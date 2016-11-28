
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


#ifndef __TCE_TIMER_H__
#define __TCE_TIMER_H__

#include "tce_singleton.h"
#include "fifo_buffer.h"
#include "tce_thread.h"
#include "tce_lock.h"
#include <map>
#include <deque>

namespace tce{


template< typename ID >
class CTimer
	: private CNonCopyAble
{
public:
	typedef CTimer this_type;
	enum TIMER_CMD{
		TC_ERROR,
		TC_ADD_TIMER,
		TC_DEL_TIMER,
		TC_RESET_TIMER,
	};
	typedef void (* CALLBACKFUNC)(void);

	struct STimerCmd{
		STimerCmd(){}
		STimerCmd(ID iID, TIMER_CMD nCmd, time_t dwWait, bool bDel, CALLBACKFUNC func=NULL)
			:nTimerCmd(nCmd),id(iID),dwWaitTime(dwWait),bAutoDel(bDel),pFunc(func)
		{

		}
		TIMER_CMD nTimerCmd;
		ID id;
		time_t dwWaitTime;
		bool bAutoDel;
		CALLBACKFUNC pFunc;
	};
private:

	struct STimerInfo 
	{
		time_t dwWaitTime;
		timeval tvStartTime;
		bool bAutoDel;
		CALLBACKFUNC pFunc;

		bool IsTimeOut() {
			timeval tvCur;
			gettimeofday(&tvCur, 0);
			uint64_t ui64CurTime = (uint64_t)tvCur.tv_sec*1000+tvCur.tv_usec/1000;
			uint64_t ui64StartTime = (uint64_t)tvStartTime.tv_sec*1000+tvStartTime.tv_usec/1000;
			if ( ui64StartTime > ui64CurTime )
			{
				tvStartTime = tvCur;
			}
			else
			{
				if ( ui64StartTime+dwWaitTime  <= ui64CurTime )
				{
					return true;
				}
				else
				{
					return false;
				}
			}

			return false;
		}	
	};
	
	typedef std::deque<STimerCmd>	DEQ_TIMER_CMD;
	typedef std::map<ID, STimerInfo> MAP_TIMER;
	typedef typename MAP_TIMER::iterator MAP_TIMER_ITERATOR;
public:
	CTimer()
		:m_oTimerThread(&TimerThread)
		,m_bThreadRun(false)
	{}

	~CTimer(){
		m_bThreadRun = false;
		m_oTimerThread.Stop();
	}

	bool Start(){
		m_oTimerThread.Start(this);
		return true;
	}

	bool Stop(){
		m_bThreadRun = false;
		m_oTimerThread.Stop();
		return true;
	}

	bool Add(CALLBACKFUNC pFunc, const int32_t iElapse, const bool bAutoDel=true){
		STimerCmd stCmd(reinterpret_cast<ID>(pFunc), TC_ADD_TIMER, iElapse, bAutoDel, pFunc);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool Del(CALLBACKFUNC pFunc){
		STimerCmd stCmd(reinterpret_cast<ID>(pFunc), TC_DEL_TIMER, 0, 0, pFunc);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool Reset(CALLBACKFUNC pFunc){
		STimerCmd stCmd(reinterpret_cast<ID>(pFunc), TC_RESET_TIMER, 0, 0, pFunc);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool Add(const ID id, const int32_t iElapse, const bool bAutoDel=true){
		STimerCmd stCmd(id, TC_ADD_TIMER, iElapse, bAutoDel);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool Del(const ID id){
		STimerCmd stCmd(id, TC_DEL_TIMER, 0, 0);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool Reset(const ID id){
		STimerCmd stCmd(id, TC_RESET_TIMER, 0, 0);
		CAutoLock oAutoLock(m_oInLock);
		m_inTimerCmdDeque.push_back(stCmd);
		return true;
	}

	bool GetTimeoutTimer(STimerCmd& stTimer){
		bool bOk = false;
		CAutoLock oAutoLock(m_oOutLock);
		if (!m_outTimerCmdDeque.empty())
		{
			stTimer = m_outTimerCmdDeque.front();
			m_outTimerCmdDeque.pop_front();
			bOk = true;
		}
		return bOk;
	}

private:
	static int TimerThread(void* pParam){
		this_type* pThis = (this_type*)pParam;
		if ( NULL != pThis )
		{
			pThis->m_bThreadRun = true;
			pThis->TimerProcess();
			pThis->m_bThreadRun = false;
		}
		return 0;
	}


	int TimerProcess(){
		STimerCmd stTimerCmd;
		STimerInfo stTimerInfo;

		while( m_bThreadRun )
		{
//			std::cout << "time=" << tce::GetTickCount() << std::endl;
			{
				CAutoLock oAutoLock(m_oInLock);
				while( !m_inTimerCmdDeque.empty() )
				{
					stTimerCmd = m_inTimerCmdDeque.front();
					m_inTimerCmdDeque.pop_front();
					if ( TC_ADD_TIMER == stTimerCmd.nTimerCmd )
					{
						stTimerInfo.pFunc  = stTimerCmd.pFunc;
						stTimerInfo.bAutoDel = stTimerCmd.bAutoDel;
						gettimeofday(&stTimerInfo.tvStartTime, 0);
//						stTimerInfo.dwStartTime = tce::GetTickCount();
						stTimerInfo.dwWaitTime = stTimerCmd.dwWaitTime;
						MAP_TIMER_ITERATOR it=m_mapTimers.insert( typename MAP_TIMER::value_type(stTimerCmd.id, stTimerInfo)).first;
						it->second.bAutoDel = stTimerCmd.bAutoDel;
						it->second.dwWaitTime = stTimerCmd.dwWaitTime;
					}
					else if ( TC_DEL_TIMER == stTimerCmd.nTimerCmd )
					{
						m_mapTimers.erase(stTimerCmd.id);
					}
					else if ( TC_RESET_TIMER == stTimerCmd.nTimerCmd )
					{
						MAP_TIMER_ITERATOR it=m_mapTimers.find(stTimerCmd.id);
						if ( it != m_mapTimers.end() )
						{
							gettimeofday(&it->second.tvStartTime, 0);
//							it->second.dwStartTime = tce::GetTickCount();
						}
					}
				}
			}

			for ( MAP_TIMER_ITERATOR it=m_mapTimers.begin(); it!=m_mapTimers.end(); )
			{
				if ( it->second.IsTimeOut() )
				{
					stTimerCmd.id = it->first;
					stTimerCmd.pFunc = it->second.pFunc;
					stTimerCmd.dwWaitTime = it->second.dwWaitTime;
					{
						CAutoLock oAutoLock(m_oOutLock);
						m_outTimerCmdDeque.push_back(stTimerCmd);
					}
					if ( it->second.bAutoDel )
					{
						m_mapTimers.erase(it++);
					}
					else
					{
						gettimeofday(&it->second.tvStartTime, 0);
//						it->second.dwStartTime += it->second.dwWaitTime;
						++it;
					}
				}
				else
				{
					++it;
				}
			}


			// sleep
			tce::xsleep(1);
		}

		return 0;
	}

private:
	typedef int32_t (* THREADFUNC)(void *);
	typedef CThread<THREADFUNC> THREAD;	
	THREAD m_oTimerThread;

private:
	DEQ_TIMER_CMD m_outTimerCmdDeque;
	DEQ_TIMER_CMD m_inTimerCmdDeque;

	MAP_TIMER m_mapTimers;

	volatile bool m_bThreadRun;

	CMutex m_oInLock;
	CMutex m_oOutLock;

};


};

#endif



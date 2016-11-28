
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


#ifndef __TCE_EVENT_H__
#define __TCE_EVENT_H__

#include <assert.h>
#include <deque>
#include "tce_lock.h"
#include "tce_thread.h"
#include "fifo_buffer.h"
#include <iostream>
using namespace std;

namespace tce{

	class CEvent
	{

	public:
		enum EVENT_TYPE
		{
			EVENT_TYPE_COMPETE,	//竞争、争抢型
			EVENT_TYPE_DISPATCH,	//主动派发
		};

		enum ERR_CODE
		{
			ERR_OK = 0,
			ERR_THREAD_NOT_FOUND,	//未找到对应的工作线程
			ERR_THREAD_QUEUE_FULL,	//工作线程队列满
			ERR_THREAD_QUEUE_ERR,
		};

	private:
		struct _SMsg 
		{
			_SMsg(const int32_t id,uint64_t ulP,void* pP)
				:iID(id)
				,ulParam(ulP)
				,pParam(pP)
			{}
			int32_t iID;
			uint64_t ulParam;
			void* pParam;
		};
		typedef std::deque<_SMsg> _MSG_DEQ;

		typedef int32_t  (*LPRouter)(void * pUser, void * pParam);
		
	public:
		struct SMessage{
			int32_t iID;
			uint64_t ulParam;
			void* pParam;
		};

		struct S_ThreadPara
		{
			void *pPara;
			uint32_t dwThreadIdx;
		};

		bool Init(const int32_t iThreadCount, const int32_t iQueueSize, const EVENT_TYPE eType=EVENT_TYPE_COMPETE)
		{
			int32_t i = 0;
			m_eEventType = eType;
			m_dwThreadCount = (uint32_t)iThreadCount;
			m_iQueueSize = iQueueSize;
			
			S_ThreadPara stPara;
			stPara.pPara = this;

			if(iThreadCount > 0)
			{
				m_vecPara.resize(iThreadCount);
				for (i=0; i<iThreadCount; ++i)
				{
					stPara.dwThreadIdx = (uint32_t)i;
					m_vecPara[i] = stPara;
				}
			}

			//2009-08-12 主动派发型
			if(eType == EVENT_TYPE_DISPATCH && iThreadCount > 0)
			{
				m_vecDispatchFIFO.resize(iThreadCount, 0);
				for (i=0; i<iThreadCount; ++i)
				{
					CFIFOBuffer* poInBuffer = new CFIFOBuffer;
					if ( NULL == poInBuffer || !poInBuffer->Init(iQueueSize*sizeof(_SMsg)) )
					{
						cout << "init dispatch buffer error: no enough memory." << endl;
						return false;
					}
					m_vecDispatchFIFO[i] = poInBuffer;
				}
			}

			for (i=0; i<iThreadCount; ++i)
			{			
				THREAD* pThread = new THREAD(&WorkThread);
				pThread->Start(&(m_vecPara[i]));
				m_vecThreads.push_back(pThread);
			}

			return true;
		}

		void SetEventID(const int32_t iID)	{	m_iID = iID;	}
		int32_t GetEventID()	const {	return m_iID;	}
		//bool IsHasData() const {		return m_msgDeqs.size() > 0 ? true : false;	}
		inline bool GetMessage(int32_t& iID, uint64_t& ulParam, void*& pParam){
			bool bOk = false;
			CAutoLock oAutoLock(m_oQueryLock);
			if ( !m_msgDeqs.empty() )
			{
				_SMsg stMsg = m_msgDeqs.front();
				iID = stMsg.iID;
				ulParam = stMsg.ulParam;
				pParam = stMsg.pParam;
				m_msgDeqs.pop_front();
				bOk = true;
			}
			return bOk;

		}
		virtual void OnMessage(const int32_t iID,uint64_t ulParam,void* pParam) = 0;

		inline bool SendMessage(const int32_t iID,uint64_t ulParam,void* pParam){
			CAutoLock oAutoLock(m_oQueryLock);
			_SMsg stMsg(iID, ulParam, pParam);
			m_msgDeqs.push_back(stMsg);
			return true;
		}

		CEvent()
			:m_bThreadRun(false)
			,m_iQueueSize(1000)
			,m_dwThreadCount(0)
			,m_eEventType(EVENT_TYPE_COMPETE)
		{}

		virtual ~CEvent()
		{
			if(m_eEventType == EVENT_TYPE_DISPATCH )
			{
				for(uint32_t i=0;i<m_dwThreadCount;++i)
				{
					if(!m_vecDispatchFIFO[i])
					{
						delete m_vecDispatchFIFO[i];
						m_vecDispatchFIFO[i] = NULL;
					}
				}
			}
		}

		bool SendWork(const int32_t iID,uint64_t ulParam,void* pParam)
		{
			assert(m_eEventType == EVENT_TYPE_COMPETE);
			
			CAutoLock oAutoLock(m_oWorkLock);
			if ( (int32_t)m_msgWorkDeqs.size() >= m_iQueueSize )
			{
				return false;
			}
			_SMsg stMsg(iID, ulParam, pParam);
			m_msgWorkDeqs.push_back(stMsg);
			return true;
		}

		//2009-08-13 按照用户指定的规则，派发给相应的工作线程
		ERR_CODE DispatchWork(const int32_t iID,uint64_t ulParam,void* pParam, LPRouter CallbackFun, void * pCallUser, void * pCallParam)
		{
			assert(m_eEventType == EVENT_TYPE_DISPATCH);
			
			int32_t iThreadIdx = CallbackFun(pCallUser, pCallParam);
			if(iThreadIdx < 0 || iThreadIdx >= (int32_t)m_dwThreadCount)
			{
				return ERR_THREAD_NOT_FOUND;
			}

			_SMsg stMsg(iID, ulParam, pParam);
			CFIFOBuffer::RETURN_TYPE nRe = m_vecDispatchFIFO[iThreadIdx]->Write((char*)&stMsg, sizeof(stMsg));
			if( nRe ==  CFIFOBuffer::BUF_OK )
			{
				return ERR_OK;
			}
			else if( nRe ==  CFIFOBuffer::BUF_FULL ) 
			{
				return ERR_THREAD_QUEUE_FULL;
			}
			else
			{
				return ERR_THREAD_QUEUE_ERR;
			}
		}

		ERR_CODE DispatchWork(const int32_t iID,uint64_t ulParam,void* pParam, uint32_t dwThreadIdx)
		{
			assert(m_eEventType == EVENT_TYPE_DISPATCH);
			
			if(dwThreadIdx >= m_dwThreadCount)
			{
				return ERR_THREAD_NOT_FOUND;
			}

			_SMsg stMsg(iID, ulParam, pParam);
			CFIFOBuffer::RETURN_TYPE nRe = m_vecDispatchFIFO[dwThreadIdx]->Write((char*)&stMsg, sizeof(stMsg));
			if( nRe ==  CFIFOBuffer::BUF_OK )
			{
				return ERR_OK;
			}
			else if( nRe ==  CFIFOBuffer::BUF_FULL ) 
			{
				return ERR_THREAD_QUEUE_FULL;
			}
			else
			{
				return ERR_THREAD_QUEUE_ERR;
			}
		}

	protected:
		size_t GetWorkQueueSize() const {	return m_msgWorkDeqs.size();	}
		size_t GetMessageSize() const {	return m_msgDeqs.size();	}

		//2009-08-10 event 多线程执行时增加传递线程序列号
		virtual int32_t OnWork(const int32_t iID,uint64_t ulParam,void* pParam, uint32_t dwThreadIdx)	= 0;
		/*{	
			cout << "ERROR: virtual function OnWork not implemented !" << endl;
			assert(false);
			return 0;
		}*/

	private:
		static int32_t WorkThread(void* pParam){
			//CEvent* pThis = (CEvent*)pParam;
			
			S_ThreadPara *pstPara = (S_ThreadPara*) pParam;
			if( NULL == pstPara)
			{
				return 0;
			}
			
			CEvent* pThis = (CEvent*)pstPara->pPara;
			if ( NULL != pThis )
			{
				//cout << "WorkThread :" << (uint64_t)(pParam) << ", idx=" << pstPara->dwThreadIdx<< endl;
				pThis->m_bThreadRun = true;
				pThis->WorkProcess(pstPara->dwThreadIdx);
				pThis->m_bThreadRun = false;
			}
			return 0;
		}
		
		int32_t WorkProcess(uint32_t dwThreadIdx){	
			
			int32_t iID = 0;
			uint64_t ulParam = 0;
			void* pParam = 0;
			bool bHasData = false;
			while (m_bThreadRun)
			{
				bHasData = false;
				
				if(m_eEventType == EVENT_TYPE_DISPATCH)	//2009-08-13 alex
				{
					if(dwThreadIdx >= m_vecDispatchFIFO.size() || NULL == m_vecDispatchFIFO[dwThreadIdx])
					{
						cout << "!!!WorkProcess: invalid thread idx=" << dwThreadIdx 
							<< ", thread_num=" << m_dwThreadCount << endl;
					}
					else
					{
						CFIFOBuffer::RETURN_TYPE nRe = m_vecDispatchFIFO[dwThreadIdx]->ReadNext();
						if ( CFIFOBuffer::BUF_OK == nRe )
						{
							if ( m_vecDispatchFIFO[dwThreadIdx]->GetCurDataLen() == (int32_t)sizeof(_SMsg) )
							{
								_SMsg * pstMsg = (_SMsg *)m_vecDispatchFIFO[dwThreadIdx]->GetCurData();
								if(NULL != pstMsg)
								{
									iID = pstMsg->iID;
									ulParam = pstMsg->ulParam;
									pParam = pstMsg->pParam;
									bHasData = true;
								}
							}

							m_vecDispatchFIFO[dwThreadIdx]->MoveNext();
						}
						else if ( CFIFOBuffer::BUF_EMPTY != nRe )
						{
							cout << "WorkProcess read FIFO error:" << m_vecDispatchFIFO[dwThreadIdx]->GetErrMsg() << endl;
						}
					}
				}
				else		//争抢型
				{
					CAutoLock oAutoLock(m_oWorkLock);
					if ( !m_msgWorkDeqs.empty() )
					{
						_SMsg stMsg = m_msgWorkDeqs.front();
						iID = stMsg.iID;
						ulParam = stMsg.ulParam;
						pParam = stMsg.pParam;
						m_msgWorkDeqs.pop_front();
						bHasData = true;
					}
				}

				if ( bHasData )
				{
					this->OnWork(iID, ulParam, pParam, dwThreadIdx);
				}
				else
				{
					tce::xsleep(10);
				}
			}
			
			return 0;		
		}

		typedef int32_t (* THREADFUNC)(void *);
		typedef CThread<THREADFUNC> THREAD;	
		typedef std::vector< THREAD* > VEC_THREAD;	
		typedef std::vector< S_ThreadPara> VEC_THREADPara;	

	private:
		int32_t m_iID;
		tce::CMutex m_oQueryLock;
		_MSG_DEQ m_msgDeqs;
		_MSG_DEQ m_msgWorkDeqs;
		tce::CMutex m_oWorkLock;
		bool m_bThreadRun;
		int32_t m_iQueueSize;
		VEC_THREAD m_vecThreads;
		VEC_THREADPara m_vecPara;

		uint32_t m_dwThreadCount;
		EVENT_TYPE m_eEventType;
		vector<CFIFOBuffer*> m_vecDispatchFIFO;//派发管道
		
	};

};


#endif



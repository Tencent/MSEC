
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


#ifndef __SIGNAL_HANDLER_H__
#define __SIGNAL_HANDLER_H__

#include <signal.h>
#include <vector>
#include <cstddef>
using namespace std;

typedef void (*HANDLESIGNAL_FUNC)(int iSignal, void *pUserInfo);

enum SIGNAL_STATUS
{
	SS_INACTIVE = 0,
	SS_ACTIVE,
};

struct SIG_INFO
{
	SIGNAL_STATUS iStatus;
	HANDLESIGNAL_FUNC pfHandleSignal;
};

class CSigHandler
{
public:
	typedef void (*sig_func)(int);

public:
	~CSigHandler(void);

	static CSigHandler& GetInstance()
	{
		if ( NULL == m_pInstance )
		{
			m_pInstance = new CSigHandler;
		}
		return *m_pInstance;
	}

public:
	bool SetSigHander(int iSignal, HANDLESIGNAL_FUNC pfHandleSignal);
	bool Check(void *pUserInfo = NULL);

private:
	bool IsValidSignal(int iSignal)
	{
		if (iSignal <= 0 || iSignal >= iMaxSignalCnt)
		{
			return false;
		}
		return true;
	}

	bool SetSigStatus(int iSignal, SIGNAL_STATUS iStatus)
	{
		if ( !IsValidSignal(iSignal) )
		{
			return false;
		}

		m_vecSigInfo[iSignal].iStatus = iStatus;
		return true;
	}

	void IncSigCnt(void)
	{
		++m_iSigCnt;
	}

	static void OnSignalSet(int iSignal)        /* 回调函数一定要是static，因为成员函数隐藏了一个this指针 */
	{
		CSigHandler::GetInstance().SetSigStatus(iSignal, SS_ACTIVE);
		CSigHandler::GetInstance().IncSigCnt();
	}

	sig_func signal(int iSignal, sig_func func)
	{
		struct sigaction act, oact;
		
		act.sa_handler = func;
		sigemptyset(&act.sa_mask);
		act.sa_flags = 0;

		if ( sigaction(iSignal, &act, &oact) < 0 )
		{
			return SIG_ERR;
		}

		return oact.sa_handler;
	}

private:
	CSigHandler();
	static CSigHandler* m_pInstance;

private:
	vector<SIG_INFO> m_vecSigInfo;
	int m_iSigCnt;                              /* 注册的信号数 */

public:
	static const int iMaxSignalCnt;
};

#endif


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


#ifndef __SET_PROC_CENTER_H__
#define __SET_PROC_CENTER_H__

#include "tce.h"
#include "tce_singleton.h"
#include "monitor.pb.h"
#include "shm_mgr.h"
#include "thread_pool.h"
#include "mysql_wrapper.h"

#include <set>
#include <tr1/memory>

extern CShmMgr ServiceShm;
extern CShmMgr DataShm;

class CSetProcCenter
	: tce::CNonCopyAble
{
public:
	DECLARE_SINGLETON_CLASS(CSetProcCenter);

public:
	bool Init(void);
	void SetTcpID(const int iID)    {   m_iTcpID = iID; }

	void OnTimer(const int iId);

	int OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize);
	void OnClose(tce::SSession& stSession);
	void OnConnect(tce::SSession& stSession, const bool bConnectOk);
	void OnError(tce::SSession& stSession, const int32_t iErrCode, const char* pszErrMsg);
	
	void PrintShmInfo();
	int Process(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqReport> Pkg);
	int ProcessReq(msec::monitor::ReqReport&  pkg, string& srcip, bool timecheck=true);
	int SendRespPkg(tce::SSession& stSession, uint32_t ret);
	int UpdateValueData(const msec::monitor::Attr& attr, const string& ip, int day, int begin, int end, int index, bool needjudge);

	void Wait() {m_pool.wait();};
private:
	enum TIMER_TYPE
	{
		TT_CHECK_SIGNAL = 1,
		TT_PRINT_SHM_INFO = 2,
	};

private:
	CSetProcCenter(void);

	time_t m_tCurTime;
	int m_iTcpID;
	wbl::thread_pool m_pool;
	mutable tce::CMutex lock_write;
};

#endif

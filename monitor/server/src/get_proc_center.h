
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


#ifndef __GET_PROC_CENTER_H__
#define __GET_PROC_CENTER_H__

#include "tce.h"
#include "tce_singleton.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include "shm_mgr.h"
#include "thread_pool.h"
#include "mysql_wrapper.h"
#include <tr1/memory>

extern CShmMgr ServiceShm;
extern CShmMgr DataShm;

class thrData: public wbl::thread_data
{
public:
	monitor::CMySql mysql_alarm;
};

class CGetProcCenter
	: tce::CNonCopyAble
{
public:
	DECLARE_SINGLETON_CLASS(CGetProcCenter);

public:
	bool Init(void);
	void SetTcpID(const int iID)    {   m_iTcpID = iID; }

	int OnRead(tce::SSession& stSession, const unsigned char* pszData, const size_t nSize);
	void OnClose(tce::SSession& stSession);
	void OnConnect(tce::SSession& stSession, const bool bConnectOk);
	void OnError(tce::SSession& stSession, const int32_t iErrCode, const char* pszErrMsg);

	int ProcService(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcServiceAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcAttrIP(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcIP(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcIPAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);

	int ProcSetAlarmAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcDelAlarmAttr(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcNewestAlarm(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	int ProcDelAlarm(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);

	int ProcTreeList(tce::SSession& stSession, std::tr1::shared_ptr<msec::monitor::ReqMonitor> Pkg);
	
	int SendRespPkg(tce::SSession& stSession, msec::monitor::RespMonitor& resp);
	int GetValueData(const string& servicename, const string& attrname, const string& ip, uint32_t day, char* valuedata);

	void Wait() {m_pool.wait();};

private:
	CGetProcCenter(void);

	time_t m_tCurTime;
	int m_iTcpID;
	wbl::thread_pool m_pool;
	mutable tce::ReadWriteLocker lock_treelist;
	mutable tce::CMutex lock_write;
};

#endif


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


#ifndef __DUMP_PROC_CENTER_H__
#define __DUMP_PROC_CENTER_H__

#include "tce.h"
#include "tce_singleton.h"
#include "tce_thread.h"
#include "monitor.pb.h"
#include "data.pb.h"
#include "shm_mgr.h"

extern CShmMgr ServiceShm;
extern CShmMgr DataShm;

class CDumpProcCenter
	: tce::CNonCopyAble
{
public:
	DECLARE_SINGLETON_CLASS(CDumpProcCenter);

public:
	~CDumpProcCenter();
	bool Init(const string& sDumpPath);
	bool Start();
	bool Stop(){
		m_bTraverseProcessRun = false;
		m_oTraverseThread.Stop();
		return true;
	}

	void SetDumpOn() { m_bDumpOn = true; }

	int LoadData();

private:
	CDumpProcCenter();
	
	static int32_t TraverseThread(void* pParam);
	int32_t TraverseProcess();

	string ReadEscapedString(string& str);
	void EscapeForBlank(string& str);
	void UnescapeForBlank(string& str);
	
private:
	typedef int32_t (* THREADFUNC)(void *);
	typedef tce::CThread<THREADFUNC> THREAD;	
	THREAD m_oTraverseThread;

	volatile bool m_bTraverseProcessRun;
	bool  m_bDumpOn;
	string m_sDumpPath;
};

#endif

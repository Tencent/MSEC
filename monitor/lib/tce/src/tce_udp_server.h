
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


#ifndef __TCE_UDP_SERVER_H__
#define __TCE_UDP_SERVER_H__

#include "fifo_buffer.h"
#include "tce_singleton.h"
#include "tce_thread.h"
#include "tce_server_base.h"
#include "tce_utils.h"
#include "tce_socket_api.h"
#include "tce_errcode.h"

namespace tce{

//template<>
class CUdpServer
	: private CNonCopyAble,public CServerBase
{
	enum{MAX_SEND_DATA_SIZE=60*1024};
public:
	CUdpServer();

	~CUdpServer();

	bool Init(const int32_t iCommID, const std::string& sBindIp,const uint16_t wPort,	const size_t nInBufferSize, const size_t nOutBufferSize);

	bool Start();

	bool Stop(){
		m_bReadProcessRun = false;
		m_bWriteProcessRun = false;
		m_oReadThread.Stop();
		m_oWriteThread.Stop();
		return true;
	}

private:
	static int32_t ReadWorkThread(void* pParam);
	static int32_t WriteWorkThread(void* pParam);

	int32_t ReadProcess();
	int32_t WriteProcess();

	bool CreateSocket(bool bBlack=true);
	
	void DoError(const SSession& stSession, const int32_t iErrCode, const char* pszErrMsg)
	{
		SSession stSession2(stSession);
		stSession2.SetDataType(DT_ERROR);		
		inner_DoError(stSession2, iErrCode, pszErrMsg);
	}

	void inner_DoError(SSession& stSession, const int32_t iErrCode, const char* pszErrMsg){
//		printf("inner_DoError: %s\n", pszErrMsg);
		char szData[4096];
		*(int32_t*)szData = iErrCode;
		size_t nErrSize = strlen(pszErrMsg);
		size_t nCopySize = nErrSize;
		if ( nErrSize > sizeof(szData)-5 )
			nCopySize = sizeof(szData)-5;

		memcpy(szData+4, pszErrMsg, nCopySize);
		szData[nCopySize+4] = 0;
		stSession.SetDataType(DT_ERROR);
		
		CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), szData, nCopySize+5);
		while ( nRe !=  CFIFOBuffer::BUF_OK )
		{
			if ( CFIFOBuffer::BUF_FULL == nRe )
			{
				xsleep(1);
				nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), szData, nCopySize+4);
			}
			else
			{
				printf("DoError<fd=%d> erorr: write buffer error:%s", stSession.GetFD(), m_poInBuffer->GetErrMsg());
//				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"DoError<fd=%d> erorr: write buffer error:%s", stSession.GetFD(),  m_poInBuffer->GetErrMsg());
				return ;
			}
		}		
	}	
private:
	typedef int32_t (* THREADFUNC)(void *);
	typedef CThread<THREADFUNC> THREAD;	
	THREAD m_oReadThread;
	THREAD m_oWriteThread;

private:
	struct sockaddr_in	 m_addrSvr;
	SOCKET m_iSockFd;

	volatile bool m_bWriteProcessRun;
	volatile bool m_bReadProcessRun;
};

};

#endif

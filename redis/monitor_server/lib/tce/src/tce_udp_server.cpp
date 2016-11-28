
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


#include "tce_udp_server.h"
namespace tce{


CUdpServer::CUdpServer()
:m_oReadThread(&ReadWorkThread)
,m_oWriteThread(&WriteWorkThread)
{
	m_bWriteProcessRun = false;
	m_bReadProcessRun = false;
	m_poInBuffer = NULL;
	m_poOutBuffer = NULL;
	m_iSockFd = -1;
}

CUdpServer::~CUdpServer()
{
	m_bReadProcessRun = false;
	m_bWriteProcessRun = false;
	m_oReadThread.Stop();
	m_oWriteThread.Stop();

	delete m_poInBuffer;
	m_poInBuffer = NULL;
	delete m_poOutBuffer;
	m_poOutBuffer = NULL;
	tce::socket_close(m_iSockFd);
	m_iSockFd = -1;
}

bool CUdpServer::Init(const int32_t iCommID, const std::string& sBindIp,const uint16_t wPort, const size_t nInBufferSize, const size_t nOutBufferSize)
{
	m_iCommID = iCommID;
	m_dwSvrIp = ntohl(inet_addr(sBindIp.c_str()));
	m_wSvrPort = wPort;

	//初始化

	//////////////////////////////////////////////////////////////////////////
	//用户增加代码

	//建立数据输入和输出buffer

	if ( NULL != m_poInBuffer )  
	{
		delete m_poInBuffer;
		m_poInBuffer = NULL;
	}
	m_poInBuffer = new CFIFOBuffer;
	if ( NULL == m_poInBuffer || !m_poInBuffer->Init(nInBufferSize) )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"init in buffer error: no enough memory.");
		return false;
	}

	if ( NULL != m_poOutBuffer )
	{
		delete m_poOutBuffer ;
		m_poOutBuffer = NULL;
	}
	m_poOutBuffer = new CFIFOBuffer;
	if ( NULL == m_poOutBuffer || !m_poOutBuffer->Init(nOutBufferSize) )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"init out buffer error: no enough memory.");
		return false;
	}

	if ( !this->CreateSocket(false) )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"CreateSocket error.");
		return false;
	}

	return true;
}

bool CUdpServer::CreateSocket(bool bBlack/*=true*/)
{
	if ( -1 != m_iSockFd )
	{
		tce::socket_close(m_iSockFd);
		m_iSockFd = -1;
	}


	/* Setup internet address information.  
	This is used with the bind() call */
	memset((char *) &m_addrSvr, 0, sizeof(m_addrSvr));
	m_addrSvr.sin_family = AF_INET;
	m_addrSvr.sin_port = htons(m_wSvrPort);
	m_addrSvr.sin_addr.s_addr = htonl(m_dwSvrIp);

	if ((m_iSockFd = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
	{
#ifdef WIN32
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "socket error:errno=%d",WSAGetLastError());
#else
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "socket error:errno=%d,errstr=%s",errno,strerror(errno));
#endif
		return false;
	}
	
	socket_setNCloseWait(m_iSockFd);

	if (bind(m_iSockFd, (struct sockaddr *) &m_addrSvr,sizeof(m_addrSvr)) < 0)
	{
#ifdef WIN32
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "bind error:errno=%d",WSAGetLastError());
#else
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "bind error:errno=%d,errstr=%s",errno,strerror(errno));
#endif
		tce::socket_close(m_iSockFd);
		return false;
	}

#ifdef WIN32
	int32_t  iVal = 512*1024; //aproximate 1 M
	int32_t  dwLen = sizeof(iVal);
#else
	int32_t  iVal = 40*1024*1024; //aproximate 1 M
	socklen_t dwLen = sizeof(iVal);
#endif
	if (setsockopt(m_iSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&iVal, dwLen) == -1)
	{
#ifdef WIN32
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[Err] set socket option error. errno = %d",WSAGetLastError());
#else
		xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[Err] set socket option error. errno = %d, err str = %s",errno,strerror(errno));
#endif
	}
	else
	{
		if (getsockopt(m_iSockFd, SOL_SOCKET, SO_RCVBUF, (char*)&iVal, &dwLen) != -1)
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[Info] sock receive buffer is %d, value len = %d", iVal, dwLen);
		}
	}


	if (!bBlack)
	{
		tce::socket_setNBlock(m_iSockFd);
	}

	return true;
}

bool CUdpServer::Start()
{
	bool bOk = true;

	m_oReadThread.Start(this);
	m_oWriteThread.Start(this);

	return bOk;
}

int32_t CUdpServer::ReadWorkThread(void* pParam)
{
	CUdpServer* poUdpSvr = (CUdpServer*)pParam;
	if ( NULL != poUdpSvr )
	{
		poUdpSvr->ReadProcess();

	}

	return 0;
}

int32_t CUdpServer::WriteWorkThread(void* pParam)
{
	CUdpServer* pThis = (CUdpServer*)pParam;
	if ( NULL != pThis )
	{
		pThis->WriteProcess();
	}
	return 0;
}

int32_t CUdpServer::ReadProcess()
{
	m_bReadProcessRun = true;
	struct sockaddr_in peerAddr;
	socklen_t nLen = sizeof(peerAddr);
	SSession* pstSession = NULL;
	char szRecvBuf[MAX_SEND_DATA_SIZE+sizeof(SSession)];
	pstSession = (SSession*)szRecvBuf;
	int32_t nRecv = 0;
	CFIFOBuffer::RETURN_TYPE nRe = CFIFOBuffer::BUF_OK;
	while( m_bReadProcessRun )
	{
		nRecv = recvfrom(m_iSockFd, szRecvBuf+sizeof(SSession), sizeof(szRecvBuf)-sizeof(SSession), 0, (struct sockaddr *) &peerAddr, &nLen);
		if (nRecv > 0)
		{
			memset(pstSession, 0, sizeof(SSession));
			pstSession->SetPeerAddr(peerAddr);
			pstSession->SetDataType(DT_UDP_DATA);
			pstSession->SetBeginTime(tce::GetTickCount());

			nRe = m_poInBuffer->Write(reinterpret_cast<const unsigned char*>(szRecvBuf), nRecv+sizeof(SSession));
			while ( nRe !=  CFIFOBuffer::BUF_OK )
			{
				if ( CFIFOBuffer::BUF_FULL == nRe )
				{
					xsleep(1);
					nRe = m_poInBuffer->Write(reinterpret_cast<const unsigned char*>(szRecvBuf), nRecv+sizeof(SSession));
				}
				else
				{
					tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ReadProcess:write buffer error:%s", m_poInBuffer->GetErrMsg());
					DoError(*pstSession, TEC_BUFFER_ERROR, m_szErrMsg);
					return false;
				}
			}		
		}
		else
		{
			tce::xsleep(1);
//#ifdef 	WIN32
//			if ( WSAECONNRESET != WSAGetLastError() )
//#else
////			if ( WSAECONNRESET != errno() )
//#endif	
//			{
//#ifdef WIN32
//				xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"recvfrom error:errno=%d",WSAGetLastError());
//				printf("recvfrom error:errno=%d",WSAGetLastError());
//#else
//				xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"recvfrom error:errno=%d,errstr=%s",errno,strerror(errno));
//				printf("recvfrom error:errno=%d,errstr=%s",errno,strerror(errno));
//#endif
//			}
		}
	}

	return 0;
}

int32_t CUdpServer::WriteProcess()
{
	m_bWriteProcessRun = true;
	SSession* pstSession=NULL;
	CFIFOBuffer::RETURN_TYPE nRe = CFIFOBuffer::BUF_OK;
	while( m_bWriteProcessRun )
	{
		nRe = m_poOutBuffer->ReadNext();
		if ( CFIFOBuffer::BUF_OK == nRe )
		{
			if ( m_poOutBuffer->GetCurDataLen() >= (int32_t)sizeof(SSession) && m_poOutBuffer->GetCurDataLen()-(int32_t)sizeof(SSession) <= MAX_SEND_DATA_SIZE)
			{
				pstSession = (SSession*)m_poOutBuffer->GetCurData();
				if (sendto(m_iSockFd, reinterpret_cast<const char*>(m_poOutBuffer->GetCurData()+sizeof(SSession)), m_poOutBuffer->GetCurDataLen()-sizeof(SSession), 0, (struct sockaddr *)&pstSession->GetPeerAddr(), sizeof(struct sockaddr)) == -1)
				{	
#ifdef WIN32
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"WriteProcess:sendto error:errno=%d",WSAGetLastError());
#else
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"WriteProcess:sendto error:%s, errno=%d",strerror(errno), errno);
#endif
					DoError(*pstSession, TEC_SYSTEM_ERROR, m_szErrMsg);
					if ( errno == 11 )
					{
						tce::xsleep(1);
					}
				}
			}
			else
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"WriteProcess:read buf error<session_size=%d>:size<%d> too large.", sizeof(SSession), m_poOutBuffer->GetCurDataLen());
				DoError(*pstSession, TEC_DATA_ERROR, m_szErrMsg);
			}

			m_poOutBuffer->MoveNext();
		}
		else if ( CFIFOBuffer::BUF_EMPTY == nRe )
		{
			tce::xsleep(1);
		}
		else
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"WriteProcess:read buffer error:%s", m_poInBuffer->GetErrMsg());
			DoError(*pstSession, TEC_BUFFER_ERROR, m_szErrMsg);
		}
	}

	return 0;
}



};



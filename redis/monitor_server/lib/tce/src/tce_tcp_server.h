
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


#ifndef __TCE_TCP_SERVER_H__ 
#define __TCE_TCP_SERVER_H__

#include "fifo_buffer.h"
#include "tce_singleton.h"
#include "tce_thread.h"
#include "tce_utils.h"
#include "tce_server_base.h"
#include "tce_session_manage.h"
#include "tce_errcode.h"
#include <time.h>
#include <linux/sockios.h>
#include <list>

namespace tce{

template
<
	class ParsePkg,
	class SessionManage=CSessionManage<CSocketSession> 
>
class CTcpServer
	: private CNonCopyAble,public CServerBase
{
	typedef CTcpServer this_type;
	enum FD_EVENT{
#ifdef WIN32
		FD_WRITE_EVENT=0x01,
		FD_READ_EVENT=0x02,
		FD_ERROR_EVENT=0x04,
#else
		FD_WRITE_EVENT=EPOLLOUT,
		FD_READ_EVENT=EPOLLIN,
		FD_ERROR_EVENT=EPOLLERR,
#endif
	};

	enum FD_EVENT_CTL{
#ifdef WIN32
		FD_CTL_ADD=0,
		FD_CTL_MOD=1,
		FD_CTL_DEL=2,
#else
		FD_CTL_ADD=EPOLL_CTL_ADD,
		FD_CTL_MOD=EPOLL_CTL_MOD,
		FD_CTL_DEL=EPOLL_CTL_DEL,
#endif
	};


	template<int32_t v> 
	struct Int2Type {
		enum {type = v};
	};

public:
	CTcpServer()
		:m_oListenThread(&ListenWorkThread)
		,m_oReadWriteThread(&ReadWriteWorkThread)
		,m_oThreadByAll(&WorkAllThread)
		,m_bThreadRun(false)
		,m_iListenFd(-1)
		,m_iMaxFd(0)
		,m_nSessionID(1)
		,m_pFDTmpBuffer(NULL)
		,m_nFDTmpBufferSize(0)
		,m_pSendTmpBuffer(NULL)
		,m_nSendTmpBufferSize(0)
		,m_bSleep(true)
	{
		m_dwCurTime = time(NULL);
		m_nLastCheckClosingTime = m_dwCurTime;
		m_nLastCheckTime = m_dwCurTime;
		m_poInBuffer = 0;
		m_poOutBuffer = 0;
#ifndef WIN32
		m_iEpollFd = -1;
#endif
	}

	~CTcpServer(){
		m_bThreadRun = false;
		m_oListenThread.Stop();
		m_oReadWriteThread.Stop();
		m_oThreadByAll.Stop();

		delete m_poInBuffer;
		m_poInBuffer = NULL;
		delete m_poOutBuffer;
		m_poOutBuffer = NULL;
		tce::socket_close(m_iListenFd);
		m_iListenFd = -1;
		
#ifndef WIN32
		tce::socket_close(m_iEpollFd);
		m_iEpollFd = -1;
#endif
	}

	bool Init(const int32_t iCommID,
		const uint32_t dwBindIp,
		const uint16_t wPort,	
		const size_t nInBufferSize,
		const size_t nOutBufferSize,
		const size_t nMaxFdInSize=1024*100,
		const size_t nMaxFdOutSize=1024*100,
		const size_t nTotalMallocMem=100*1024*1024,
		const size_t nMallocItemSize = 4*1024,
		const size_t nMaxClient=10000, 
		const time_t nOverTime=180,
		const time_t nDelayStartTime=0)
	{
		m_iCommID = iCommID;
		m_dwSvrIp = dwBindIp;
		m_wSvrPort = wPort;
		m_nDelayStartTime = nDelayStartTime;
		m_nOverTime = nOverTime;
		//初始化

#ifndef WIN32
		if ( m_iEpollFd != -1 )
			tce::socket_close(m_iEpollFd);

		m_iEpollFd = epoll_create(EPOLL_MAX_SIZE);
		if(m_iEpollFd < 0)
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg), "[error] epoll_create(%d) return %d, error:%s, %s,%d", EPOLL_MAX_SIZE, m_iEpollFd, strerror(errno), __FILE__, __LINE__);
			return false;
		}


		m_pEpollEvents = new epoll_event[EPOLL_EVENT_COUNT];
		if (NULL == m_pEpollEvents) 
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"no memory.");
			return false;
		}

#endif

		//初始化server监听socket
		if (!CreateListenSocket())
		{
			return false;
		}


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

		if ( !m_oSocketSessionMgr.Init(nMaxFdInSize,nMaxFdOutSize,nMaxClient,nTotalMallocMem,nMallocItemSize) )
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"init client socket error: no enough memory.");
			return false;
		}

		m_nFDTmpBufferSize = (nMaxFdInSize > nMaxFdOutSize ? nMaxFdInSize : nMaxFdOutSize) + 1024;
		m_pFDTmpBuffer = new char[m_nFDTmpBufferSize];
		if ( NULL == m_pFDTmpBuffer )
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"init fd tmp buffer error: no enough memory.");
			return false;
		}
		
		m_nSendTmpBufferSize = (nMaxFdInSize > nMaxFdOutSize ? nMaxFdInSize : nMaxFdOutSize) + 1024;
		m_pSendTmpBuffer = new char[m_nSendTmpBufferSize];
		if ( NULL == m_pSendTmpBuffer )
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"init send tmp buffer error: no enough memory.");
			return false;
		}		

		return true;
	}

	bool Start()
	{
		m_bThreadRun = true;
		return this->Start_imp(Int2Type<SessionManage::NEED_LOCK>());
	}

	bool Stop(){
		m_bThreadRun = false;
		m_oListenThread.Stop();
		m_oReadWriteThread.Stop();
		m_oThreadByAll.Stop();
		return true;
	}


private:

	bool Start_imp( Int2Type<true> ){
		m_oThreadByAll.Start(this);
		return true;

	}

	bool Start_imp( Int2Type<false> ){
		m_oReadWriteThread.Start(this);
		m_oListenThread.Start(this);
		return true;
	}


	bool WriteProcess(){
		SSession* pstSession = NULL;
		int32_t iReadCnt = 0;
		int32_t iDataCnt = 0;
		int32_t iCloseCnt = 0;
		int32_t iClosintCnt = 0;
		int32_t iConnectCnt = 0;
		int64_t iDataSize = 0;
		// int32_t nDataUseTime = 0;
		// int32_t nCloseUseTime = 0;
		//time_t nStartTime = GetTickCount();
		while ( m_bThreadRun )
		{
			//if ( m_iCommID == 1 ) printf("read data from buffer.\n");
			if ( ++iReadCnt > 1024 ) 
			{
//				if ( m_iCommID == 1 )
//					printf("readcnt=%d,closeCnt=%d,closing=%d,connectCnt=%d,dataCnt=%d, dataSize=%ld, costTime=%zd, nDataUseTime=%d, nCloseUseTime=%d\n", iReadCnt, iCloseCnt, iClosintCnt, iConnectCnt, iDataCnt, iDataSize, GetTickCount()-nStartTime, nDataUseTime, nCloseUseTime);
				break;			
			}
			
			CFIFOBuffer::RETURN_TYPE nRe = m_poOutBuffer->ReadNext();
			if ( CFIFOBuffer::BUF_OK == nRe )
			{
				m_bSleep = false;

				if ( m_poOutBuffer->GetCurDataLen() >= (int32_t)sizeof(SSession) )
				{
					pstSession = (SSession*)m_poOutBuffer->GetCurData();

					switch(pstSession->GetDataType())
					{
						case DT_TCP_DATA:
							{
								++iDataCnt;
								iDataSize += m_poOutBuffer->GetCurDataLen()-sizeof(SSession);
								assert(pstSession->GetFD() >= 0);
								CSocketSession* poSocket = GetSocketSession(pstSession->GetFD());
								if ( NULL != poSocket && poSocket->GetSessionID() == pstSession->GetID() )
								{
									size_t nSendDataSize = m_nSendTmpBufferSize;
									const char* pszSendData = ParsePkg::MakeSendPkg(m_pSendTmpBuffer, nSendDataSize, reinterpret_cast<const char*>(m_poOutBuffer->GetCurData()+sizeof(SSession)), m_poOutBuffer->GetCurDataLen()-sizeof(SSession));
									if ( NULL != pszSendData )
									{
										assert(nSendDataSize > 0);
										if ( !Write(poSocket, *pstSession, pszSendData, nSendDataSize) )
										{
											this->Close(poSocket);
										}
									}
									else
									{
										xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "WriteProcess: data<%lu> length large than buffer<%lu>, can't make send data.", m_poOutBuffer->GetCurDataLen()-sizeof(SSession), m_nFDTmpBufferSize);
										DoError(*pstSession, TEC_PARAM_ERROR, m_szErrMsg);
									}
								}
								else
								{
									xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "WriteProcess<comm=%d,id=%ld>: send data error:can't find socket(fd=%d,datasize=%zd).", m_iCommID, pstSession->GetID(), pstSession->GetFD(), m_poOutBuffer->GetCurDataLen()-sizeof(SSession));
									DoError(*pstSession, TEC_SOCKET_SEND_ERROR, m_szErrMsg);
									//printf("tcp_data error:can't find socket(fd=%d)\n", pstSession->GetFD());
								}
							}
							break;
						case DT_TCP_CLOSE:
							{
								
								CSocketSession* poSocket = GetSocketSession(pstSession->GetFD());
								if ( NULL != poSocket && poSocket->GetSessionID() == pstSession->GetID() && CSocketSession::SST_ESTABLISHED == poSocket->GetStatus())
								{
									poSocket->SetParam(pstSession->GetParam1(), pstSession->GetParam2());
									if ( pstSession->GetCloseWaitTime() > 0 )
									{
										++iClosintCnt;
										DoClosing(poSocket, pstSession->GetCloseWaitTime());
									}
									else
									{
										++iCloseCnt;
										Close(poSocket);
									}
								}
								

							}
							break;
						case DT_TCP_CONNECT:
							++iConnectCnt;
							if ( !Connect(*pstSession) )
							{
								this->OnConnectErr(pstSession->GetPeerAddr(), pstSession);
							}
							break;
						default:
							{
								xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "WriteProcess:data type<%d> error: unknow.", pstSession->GetDataType());
								DoError(*pstSession, TEC_PARAM_ERROR, m_szErrMsg);
							}
							break;
					}
				}
				else
				{
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "WriteProcess<session_size=%d,data_size=%d> error: data too small.", sizeof(SSession), m_poOutBuffer->GetCurDataLen());
					DoError(*pstSession, TEC_SYSTEM_ERROR, m_szErrMsg);
				}

				m_poOutBuffer->MoveNext();
			}
			else if ( CFIFOBuffer::BUF_EMPTY == nRe )
			{
				break;
			}
			else
			{
				xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"WriteProcess read buf error:%s",m_poOutBuffer->GetErrMsg());
				DoError(*pstSession, TEC_SYSTEM_ERROR, m_szErrMsg);
			}
		}
//		if ( nTmp > 0 )
//		printf("read buffer size=%d.\n", nTmp);

		return true;
	}
	
	bool Write(CSocketSession* poSocket, const SSession& stSession, const char* pszData, const size_t nDataSize)
	{
		SOCKET iFd = poSocket->GetFD();
		bool bOk = true;
		if ( CSocketSession::SST_ESTABLISHED == poSocket->GetStatus() )
		{
			poSocket->SetLastAccessTime(m_dwCurTime);
			poSocket->SetParam(stSession.GetParam1(), stSession.GetParam2());

			size_t nSendBufDataSize = poSocket->GetOutBuffer().Size();
			if ( nSendBufDataSize > 0 )
			{
				int32_t n = ::send(iFd, GetBufferDataPtr(poSocket->GetOutBuffer()), nSendBufDataSize, 0);
				if ( n > 0 )
				{
					poSocket->GetOutBuffer().Erase(n);
					nSendBufDataSize -= n;
				}
				else
				{
#ifdef WIN32
					if ( WSAEWOULDBLOCK != WSAGetLastError() )
#else
					if (errno != EAGAIN) 
#endif
					{
						return false;
					}
				}
			}


			if ( nSendBufDataSize > 0 )
			{
				if ( !poSocket->GetOutBuffer().Append(pszData, nDataSize) )
				{
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write<fd=%d,socket_allow_max_size=%lu, curdatasize=%lu,appendsize=%lu>: append error(%d).", iFd, poSocket->GetOutBuffer().MaxSize(), poSocket->GetOutBuffer().Size(), nDataSize, poSocket->GetOutBuffer().GetErrCode());
					DoError(poSocket, TEC_SOCKET_BUFFER_FULL, m_szErrMsg);
					bOk = false;
				}
				else
					SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_READ_EVENT | FD_ERROR_EVENT);
			}
			else
			{
				int32_t n = ::send(iFd, pszData, nDataSize, 0);
				if ( n > 0 )
				{
					if ( n < (int32_t)nDataSize )
					{
						if ( !poSocket->GetOutBuffer().Append(pszData+n, nDataSize-n) )
						{
							xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write<fd=%d,socket_allow_max_size=%lu, curdatasize=%lu,appendsize=%lu>: append error(%d).", iFd, poSocket->GetOutBuffer().MaxSize(), poSocket->GetOutBuffer().Size(), nDataSize-n, poSocket->GetOutBuffer().GetErrCode());
							DoError(poSocket, TEC_SOCKET_BUFFER_FULL, m_szErrMsg);
							bOk = false;
						}
						else
							SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_READ_EVENT | FD_ERROR_EVENT);
					}
					else
					{
						SetFdEvent(iFd, FD_CTL_MOD, FD_READ_EVENT | FD_ERROR_EVENT);
					}
				}
				else
				{
#ifdef WIN32
					if ( WSAEWOULDBLOCK == WSAGetLastError() )
#else
					if (errno == EAGAIN) 
#endif
					{
						if ( !poSocket->GetOutBuffer().Append(pszData, nDataSize) )
						{
							xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write<fd=%d,socket_allow_max_size=%lu, curdatasize=%lu,appendsize=%lu>: append error(%d).", iFd, poSocket->GetOutBuffer().MaxSize(), poSocket->GetOutBuffer().Size(), nDataSize, poSocket->GetOutBuffer().GetErrCode());
							DoError(poSocket, TEC_SOCKET_BUFFER_FULL, m_szErrMsg);
							bOk = false;
						}
						else
							SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_READ_EVENT | FD_ERROR_EVENT);
					}
					else
					{
#ifdef WIN32
						xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write: send data error:errno=%d", WSAGetLastError());
#else
						xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write: send data error:errno=%d,error=%s", errno, strerror(errno));
#endif
						DoError(poSocket, TEC_SOCKET_SEND_ERROR, m_szErrMsg);
						bOk = false;
					}
				}
			}
			
			// if ( CSocketSession::SST_CLOSING == poSocket->GetStatus() )
			// {
				// if ( poSocket->GetOutBuffer().Size() > 0 )
					// SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_ERROR_EVENT);
				// else
					// this->Close(poSocket);
			// }
		}
		else
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write error: socket(%d) status(%d) is close or closing.", iFd, poSocket->GetStatus());
			DoError(poSocket, TEC_SOCKET_SEND_ERROR, m_szErrMsg);
		}

		return bOk;
	}
	inline bool Write(const SOCKET iFd){
		bool bOk = false;

		CSocketSession* poSocket = GetSocketSession(iFd);
		if ( NULL != poSocket )
		{
			bOk = this->Write(poSocket);
		}
		else
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Write iSocket<%d> is error: can't find socket.", iFd);
			SSession stSession;
			DoError(stSession, TEC_UNKOWN_ERROR, m_szErrMsg);
			bOk = false;
		}

		return bOk;
	}
	bool Write(CSocketSession* poSocket){
		bool bOk = true;
		SOCKET iFd = poSocket->GetFD();
		if ( CSocketSession::SST_CONNECTTING == poSocket->GetStatus() )
		{
			this->OnConnect(poSocket);
		}
		else if ( CSocketSession::SST_ESTABLISHED == poSocket->GetStatus() 
				  || CSocketSession::SST_CLOSING == poSocket->GetStatus() )
		{
			poSocket->SetLastAccessTime(m_dwCurTime);
			int32_t n=0;
			while ( poSocket->GetOutBuffer().Size() > 0 )
			{
				n = ::send(iFd, GetBufferDataPtr(poSocket->GetOutBuffer()), poSocket->GetOutBuffer().Size(), 0);
				if (n > 0)
				{
					if ( n != (int32_t)poSocket->GetOutBuffer().Size() ) 
					{
						SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_READ_EVENT | FD_ERROR_EVENT);
					}	
					else
					{
						//注意，仅当需要写数据时才能设置这个事件　
						SetFdEvent(iFd, FD_CTL_MOD, FD_READ_EVENT | FD_ERROR_EVENT);
					}
					poSocket->GetOutBuffer().Erase(n);
					bOk = true;
				}
				else
				{
#ifdef WIN32
					if ( WSAEWOULDBLOCK != WSAGetLastError() )
#else
					if (errno == EAGAIN) 
#endif
					{
						SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_READ_EVENT | FD_ERROR_EVENT);
						bOk = true;
					}
					else
					{
						xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write: send data(%lu) error:errno=%d,error=%s", poSocket->GetOutBuffer().Size(), errno, strerror(errno));
						DoError(poSocket, TEC_SOCKET_SEND_ERROR, m_szErrMsg);
						bOk = false;
					}

					break;
				}
			}
			
			if ( bOk && CSocketSession::SST_CLOSING == poSocket->GetStatus() )
			{
				if ( poSocket->GetOutBuffer().Size() > 0 )
					SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_ERROR_EVENT);
				else if ( poSocket->GetOutBuffer().Size() <= 0 && GetSocketOutLen(iFd) <= 0 )
				{
					SetFdEvent(iFd, FD_CTL_MOD, FD_READ_EVENT | FD_ERROR_EVENT);
					shutdown(iFd, SHUT_RDWR);
				}
			}

		}
		else
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write error: socket(%d) status(%d) is close.", iFd, poSocket->GetStatus());
			DoError(poSocket, TEC_UNKOWN_ERROR, m_szErrMsg);
			bOk = false;
		}

		return bOk;
	}

	bool Read(const SOCKET iFd)
	{
		bool bOk = false;

		CSocketSession* poSocket = GetSocketSession(iFd);
		if ( NULL != poSocket )
		{
			bOk = this->Read(poSocket);
		}
		else
		{
			//tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Read iSocket<%d> is error: can't find socket.", iFd);
			//DoError(poSocket, TEC_UNKOWN_ERROR, m_szErrMsg);
			bOk = false;
		}

		return bOk;
	}

	bool Read(CSocketSession* poSocket)
	{
		SOCKET iFd = poSocket->GetFD();
		bool bOk = true;

		if ( CSocketSession::SST_ESTABLISHED == poSocket->GetStatus() )
		{
			do
			{
				char* pFreeBuf = poSocket->GetInBuffer().GetFreeBuf();
				if ( NULL == pFreeBuf || poSocket->GetInBuffer().GetFreeSize() == 0 )
				{
					tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Read error: no buffer to malloc.");
					DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
					return false;
				}

				int32_t nCount = recv(iFd, pFreeBuf, poSocket->GetInBuffer().GetFreeSize(), 0);
				if ( nCount > 0 )
				{
					poSocket->SetLastAccessTime(m_dwCurTime);
					poSocket->GetInBuffer().Append(nCount);
					const char* pInBufferData = GetBufferDataPtr(poSocket->GetInBuffer());

					if ( ParsePkg::IsSuppertProxy() && nCount>2 && poSocket->IsFirstPkg())
					{
						if ( *(uint16_t*)pInBufferData == 0 )
							poSocket->GetInBuffer().Erase(2);
					}
					
					if ( poSocket->IsFirstPkg() )
					{
						poSocket->SetFirstPkgFlag(false);
						poSocket->SetStartReadTime();
					}
					
//					if ( m_iCommID == 1 )
//						printf("fd=%d,ip=%s:%u,read count=%d, readbuffersize=%zd, data=%s\n", iFd,inet_ntoa(poSocket->GetAddr().sin_addr), ntohs(poSocket->GetAddr().sin_port), nCount, poSocket->GetInBuffer().Size(), pInBufferData);

					int32_t iWholePkgFlag = 0;
					SSession stSession;
					size_t nRealPkgLen = 0;
					size_t nPkgLen = 0;
					while ( (iWholePkgFlag = ParsePkg::HasWholePkg(poSocket->IsRequest(), pInBufferData, poSocket->GetInBuffer().Size(), nRealPkgLen, nPkgLen)) == 0 )
					{
						const char* pstRealPkgData = ParsePkg::GetRealPkgData(pInBufferData, poSocket->GetInBuffer().Size());
						stSession.SetID(poSocket->GetSessionID());
						stSession.SetDataType(DT_TCP_DATA);
						stSession.SetFD(poSocket->GetFD());
						stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
						stSession.SetPeerAddr(poSocket->GetAddr());
						stSession.SetBeginTime(tce::GetTickCount());
						stSession.SetSocketCreateTime(poSocket->GetCreateTime());
						stSession.SetSocketStartReadTime(poSocket->GetStartReadTime());

						CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), pstRealPkgData, nRealPkgLen);
						while ( nRe !=  CFIFOBuffer::BUF_OK )
						{
							if ( CFIFOBuffer::BUF_FULL == nRe )
							{
								tce::xsleep(1);
								nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), pstRealPkgData, nRealPkgLen);
							}
							else
							{
								tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"write buffer error:%s", m_poInBuffer->GetErrMsg());
								DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
								return false;
							}
						}
						poSocket->GetInBuffer().Erase(nPkgLen);
						pInBufferData = GetBufferDataPtr(poSocket->GetInBuffer());
					}

					if ( -2 == iWholePkgFlag )//非法数据包
					{
						tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"recv data is illegal.");
						DoError(poSocket, TEC_DATA_ERROR, m_szErrMsg);
						poSocket->GetInBuffer().Clear();
						return false;
					}
					
					if ( poSocket->GetOutBuffer().Size() > 0 )  
						SetFdEvent(iFd, FD_CTL_MOD, FD_READ_EVENT | FD_WRITE_EVENT | FD_ERROR_EVENT);
					else
						SetFdEvent(iFd, FD_CTL_MOD, FD_READ_EVENT | FD_ERROR_EVENT);				
				}
				else
				{
					if (0 == nCount) 
					{
						if ( ParsePkg::IsNeedReadBeforeClose() && poSocket->GetInBuffer().Size() > 0 )
						{
							SSession stSession;
							stSession.SetID(poSocket->GetSessionID());
							stSession.SetDataType(DT_TCP_DATA);
							stSession.SetFD(poSocket->GetFD());
							stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
							stSession.SetPeerAddr(poSocket->GetAddr());
							stSession.SetBeginTime(tce::GetTickCount());
							stSession.SetSocketCreateTime(poSocket->GetCreateTime());
							stSession.SetSocketStartReadTime(poSocket->GetStartReadTime());

							const char* pInBufferData = GetBufferDataPtr(poSocket->GetInBuffer());
							CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), pInBufferData, poSocket->GetInBuffer().Size());
							while ( nRe !=  CFIFOBuffer::BUF_OK )
							{
								if ( CFIFOBuffer::BUF_FULL == nRe )
								{
									tce::xsleep(1);
									nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession), pInBufferData, poSocket->GetInBuffer().Size());
								}
								else
								{
									tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"write buffer error:%s", m_poInBuffer->GetErrMsg());
									DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
									return false;
								}
							}
							poSocket->GetInBuffer().Erase(poSocket->GetInBuffer().Size());
						}

						bOk = false;
						//this->Close(poSocket);
					}
					else if (  errno != EAGAIN && errno != EWOULDBLOCK   )
					{
						bOk = false;

						//if (errno != ECONNRESET) {
						//	printf("recv<socket=%d> error:%d",iFd, errno);
						//}
	//					printf("recv<socket=%d> error:%s",iFd, strerror(errno));
					}
					break;
				}
			}while( poSocket->GetInBuffer().GetFreeSize() == 0 );
		}
		else if ( CSocketSession::SST_CONNECTTING == poSocket->GetStatus() )
		{
			int32_t iErr = -1;
			socklen_t len = sizeof(iErr);
			if (getsockopt(iFd, SOL_SOCKET, SO_ERROR,(char*)&iErr,&len)<0)
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"connecting:getsockopt error.");
				DoError(poSocket, TEC_UNKOWN_ERROR, m_szErrMsg);
				this->Close(poSocket);
			}
			else
			{
				printf("read-OnConnect(fd=%d)\n", iFd);
				this->OnConnect(poSocket);
			}
		}
		else if ( CSocketSession::SST_CLOSING == poSocket->GetStatus() )
		{
			char szTmp[128]={0};
			int32_t nCount = recv(iFd, szTmp,128, 0);
			if ( nCount <= 0 )
			{
				bOk = false;
//				if ( nCount < 0 ) printf("SST_CLOSING Read<socket=%d> error:%s",iFd, strerror(errno));
			}
			else
			{
				xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Read error: socket(%d) status is cloing and has in data.", iFd);
				DoError(poSocket, TEC_UNKOWN_ERROR, m_szErrMsg);
			}
//			SetFdEvent(iFd, FD_CTL_MOD, FD_WRITE_EVENT | FD_ERROR_EVENT);
		}
		else
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Read error: socket(%d) status is close.", iFd);
			DoError(poSocket, TEC_UNKOWN_ERROR, m_szErrMsg);
			bOk = false;
		}
		return bOk;
	}

	inline bool Close(const SOCKET iFd){
		bool bOk = false;
		CSocketSession* poSocket = GetSocketSession(iFd);
		if ( NULL != poSocket )
		{
			bOk = Close(poSocket);
		}
		else
		{
			tce::socket_close(iFd);
		}
		return bOk;
	}

	inline bool Close(CSocketSession* poSocket){
		bool bOk = true;
		SOCKET iFd = poSocket->GetFD();
		
		if ( CSocketSession::SST_CONNECTTING == poSocket->GetStatus() )
		{
			OnConnectErr(poSocket);
		}
		else
		{
			OnClose(poSocket);
		}
		EraseSocketSession(iFd);
		tce::socket_close(iFd);
		return bOk;
	}

	inline void OnClose(CSocketSession* poSocket){
		SSession stSession;
		stSession.SetID(poSocket->GetSessionID());
		stSession.SetDataType(DT_TCP_CLOSE);
		stSession.SetFD(poSocket->GetFD());
		stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
		stSession.SetPeerAddr(poSocket->GetAddr());
		stSession.SetBeginTime(tce::GetTickCount());
		stSession.SetSocketCreateTime(poSocket->GetCreateTime());
		stSession.SetSocketStartReadTime(poSocket->GetStartReadTime());

		CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
		while ( nRe !=  CFIFOBuffer::BUF_OK )
		{
			if ( CFIFOBuffer::BUF_FULL == nRe )
			{
				xsleep(1);
				nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
			}
			else
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OnClose:write buffer error:%s", m_poInBuffer->GetErrMsg());
				DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
				return ;
			}
		}		

	}

	bool Connect(const SSession& stSession){
		bool bOk = true;
		SOCKET iFd=-1;
		if ( ( iFd = ::socket(AF_INET, SOCK_STREAM, 0)) != INVALID_SOCKET )
		{
			tce::socket_setNBlock(iFd);
			tce::socket_setNCloseWait(iFd);


			int32_t iRe = ::connect(iFd, (struct sockaddr *)&stSession.GetPeerAddr(), sizeof(struct sockaddr));
#ifdef WIN32
			int32_t iErrorNo = ::WSAGetLastError();
#else
			int32_t iErrorNo = errno;
#endif
			if( iRe != INVALID_SOCKET )
			{
				CSocketSession* poSocket = m_oSocketSessionMgr.Malloc(iFd);
				if ( NULL != poSocket)
				{
					poSocket->SetFD(iFd);
					poSocket->SetReqeustFlag(false);
					poSocket->SetAddr(stSession.GetPeerAddr());
					poSocket->SetSessionID(++m_nSessionID);
					poSocket->SetStatus(CSocketSession::SST_CONNECTTING);
					poSocket->SetLastAccessTime(m_dwCurTime);
					poSocket->SetCreateTime(m_dwCurTime);
					poSocket->SetOverTime(10);
					poSocket->SetParam(stSession.GetParam1(), stSession.GetParam2());
					
					SetFdEvent(poSocket->GetFD(), FD_CTL_ADD, FD_READ_EVENT | FD_ERROR_EVENT);
					if( !this->OnConnect(poSocket) )
					{
						this->Close(poSocket);
					}			
				}
				else
				{
					tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Connect<fd=%d,maxclient=%d>:socket_session malloc error :no session to malloc.", iFd, m_oSocketSessionMgr.GetMaxClientSize());
					DoError(stSession, TEC_SOCKET_FULL, m_szErrMsg);
					tce::socket_close(iFd);
					return false;
				}

			}
			else
			{
#ifdef WIN32
				if ( iErrorNo == WSAEWOULDBLOCK )
#else
				if ( iErrorNo == EINPROGRESS )
#endif
				{
					CSocketSession* poSocket = m_oSocketSessionMgr.Malloc(iFd);
					if ( NULL != poSocket)
					{
						poSocket->SetFD(iFd);
						poSocket->SetReqeustFlag(false);
						poSocket->SetAddr(stSession.GetPeerAddr());
						poSocket->SetSessionID(++m_nSessionID);
						poSocket->SetStatus(CSocketSession::SST_CONNECTTING);
						poSocket->SetLastAccessTime(m_dwCurTime);
						poSocket->SetCreateTime(m_dwCurTime);
						poSocket->SetOverTime(10);
						poSocket->SetParam(stSession.GetParam1(), stSession.GetParam2());

						//SetFdEvent(iFd, FD_CTL_ADD, FD_READ_EVENT | FD_WRITE_EVENT | FD_ERROR_EVENT);
						SetFdEvent(iFd, FD_CTL_ADD, FD_WRITE_EVENT | FD_ERROR_EVENT);
					}
					else
					{
						tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Connect<fd=%d,maxclient=%d>:socket_session malloc error :no session to malloc.", iFd, m_oSocketSessionMgr.GetMaxClientSize());
						DoError(stSession, TEC_SOCKET_FULL, m_szErrMsg);
						tce::socket_close(iFd);
						bOk = false;
					}
				}
				else
				{
					/* 如果connect()建立连接错误，则显示出错误信息，退出*/
					tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Connect<fd=%d>:can't connect to server: errno=%d,%s..", iFd, errno, strerror(errno));
					DoError(stSession, TEC_CONNECT_ERROR, m_szErrMsg);
					tce::socket_close(iFd);
					bOk = false;
				}
			}
		}
		else
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Connect:can't create socket: errno(%d),%s.", errno, strerror(errno));
			DoError(stSession, TEC_CONNECT_ERROR, m_szErrMsg);
			bOk = false;
		}

		return bOk ;
	}

	inline bool OnConnect(CSocketSession* poSocket){
		SSession stSession;
		stSession.SetID(poSocket->GetSessionID());
		stSession.SetDataType(DT_TCP_CONNECT);
		stSession.SetFD(poSocket->GetFD());
		stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
		stSession.SetPeerAddr(poSocket->GetAddr());
		stSession.SetBeginTime(tce::GetTickCount());
		stSession.SetSocketCreateTime(poSocket->GetCreateTime());
		stSession.SetSocketStartReadTime(poSocket->GetStartReadTime());
		
		poSocket->SetStatus(CSocketSession::SST_ESTABLISHED);
		poSocket->SetOverTime(m_nOverTime);

		CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
		while ( nRe !=  CFIFOBuffer::BUF_OK )
		{
			if ( CFIFOBuffer::BUF_FULL == nRe )
			{
				xsleep(1);
				nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
			}
			else
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OnConnect:write buffer error:%s", m_poInBuffer->GetErrMsg());
				DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
				return false;
			}
		}		

		SetFdEvent(poSocket->GetFD(), FD_CTL_MOD, FD_READ_EVENT | FD_ERROR_EVENT);
		return true;
	}


	bool OnConnectErr(CSocketSession* poSocket){
		SSession stSession;
		stSession.SetID(poSocket->GetSessionID());
		stSession.SetDataType(DT_TCP_CONNECT_ERR);
		stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
		stSession.SetPeerAddr(poSocket->GetAddr());
		stSession.SetBeginTime(tce::GetTickCount());

		poSocket->SetStatus(CSocketSession::SST_CONNECT_ERR);

		CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
		while ( nRe !=  CFIFOBuffer::BUF_OK )
		{
			if ( CFIFOBuffer::BUF_FULL == nRe )
			{
				xsleep(1);
				nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
			}
			else
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OnConnectErr:write buffer error:%s", m_poInBuffer->GetErrMsg());
				DoError(poSocket, TEC_BUFFER_ERROR, m_szErrMsg);
				return false;
			}
		}		

		return true;
	}

	bool OnConnectErr(const sockaddr_in& peerAddr, SSession* pstSession){
		SSession stSession;
		stSession.SetDataType(DT_TCP_CONNECT_ERR);
		stSession.SetPeerAddr(peerAddr);
		stSession.SetParam(pstSession->GetParam1(), pstSession->GetParam2());
		stSession.SetID(0);
		stSession.SetBeginTime(tce::GetTickCount());

		CFIFOBuffer::RETURN_TYPE nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
		while ( nRe !=  CFIFOBuffer::BUF_OK )
		{
			if ( CFIFOBuffer::BUF_FULL == nRe )
			{
				xsleep(1);
				nRe = m_poInBuffer->Write((char*)&stSession, sizeof(SSession));
			}
			else
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OnConnectErr:write buffer error:%s", m_poInBuffer->GetErrMsg());
				DoError(*pstSession, TEC_BUFFER_ERROR, m_szErrMsg);
				return false;
			}
		}		

		return true;
	}

	inline CSocketSession* GetSocketSession(const SOCKET iFd){
		CSocketSession* poSocket = m_oSocketSessionMgr.Find(iFd);
		if( NULL != poSocket && CSocketSession::SST_ERR != poSocket->GetStatus() )
			assert(poSocket->GetFD() == iFd );
		return m_oSocketSessionMgr.Find(iFd);
	}
	inline void EraseSocketSession(const SOCKET iFd){
		m_oSocketSessionMgr.Erase(iFd);
	}

	inline void SetFdEvent(const SOCKET iFd, const int32_t iFlag, const int32_t iEvents){
#ifdef WIN32
	;
#else
		epoll_event ev = {0};
		//设置要处理的事件类型
		ev.events= iEvents ;
		ev.data.fd = iFd;

		//注册epoll事件
		if(epoll_ctl(m_iEpollFd, iFlag, iFd, &ev) != 0)
		{
//			SERVER_ERR_LOG2("[error] epoll_ctl fd=%d,error:%s", iSocket, strerror(errno));
			printf("[error] epoll_ctl fd=%d.", iFd);
		}
#endif

	}

	bool CreateListenSocket()
	{
		if ( m_iListenFd != -1 )
		{
			tce::socket_close(m_iListenFd);
			m_iListenFd = -1;
		}

		struct sockaddr_in address;
		memset((char *) &address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(m_wSvrPort);
		address.sin_addr.s_addr = htonl(m_dwSvrIp);

		m_iListenFd = socket(AF_INET, SOCK_STREAM, 0);
		if (m_iListenFd < 0) {
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"CreateListenSocket socket error:errno=%d,errstr=%s",errno,strerror(errno));
			return false;
		}

		if(tce::socket_setNCloseWait(m_iListenFd) < 0)
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"CreateListenSocket socket_setNCloseWait error:errno=%d,errstr=%s",errno,strerror(errno));
		}
		
		if (::bind(m_iListenFd, (struct sockaddr *) &address,sizeof(address)) < 0)
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"CreateListenSocket bind error:errno=%d,errstr=%s",errno,strerror(errno));
			tce::socket_close(m_iListenFd);
			return false;
		}


		if(listen(m_iListenFd, 1024) < 0)
		{
			xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"CreateListenSocket listen error:errno=%d,errstr=%s",errno,strerror(errno));
			tce::socket_close(m_iListenFd);
			return false;
		}

		return true;
	}


private:
	static int32_t ListenWorkThread(void* pParam){
		this_type* pThis = (this_type*)pParam;
		if ( NULL != pThis )
		{
			pThis->ListenProcess();

		}
		return 0;
	}

	static int32_t ReadWriteWorkThread(void* pParam){
		this_type* pThis = (this_type*)pParam;
		if ( NULL != pThis )
		{
			pThis->ReadWriteProcess();

		}
		return 0;
	}

	static int32_t WorkAllThread(void* pParam){
		this_type* pThis = (this_type*)pParam;
		if ( NULL != pThis )
		{
			pThis->AllProcess();

		}
		return 0;
	}

	int32_t AllProcess(){

		tce::xsleep(m_nDelayStartTime * 1000);

		// add listen socket to epoll
		SetFdEvent(m_iListenFd, FD_CTL_ADD, FD_READ_EVENT | FD_ERROR_EVENT);

		while ( m_bThreadRun )
		{
			m_dwCurTime = time(NULL);
			m_bSleep = true;

			if (!this->FdWaitByAccept())
			{
				printf("EPoll Wait error:%s", m_szErrMsg);
			}

			if (!this->WriteProcess()) 
			{
				printf("WriteProcess error:%s", m_szErrMsg);
			}

			this->CheckOvertime();
			
			if ( m_bSleep ) xsleep(1);
		}

		return 0;

	}

	int32_t ListenProcess(){

		tce::xsleep(m_nDelayStartTime * 1000);

		while ( m_bThreadRun )
		{
			if ( !this->Accept() )
			{
				tce::xsleep(1);
			}
		}

		return 0;
	}

	inline bool Accept(){
		bool bOk = true;
		sockaddr_in	addr = {0};
		socklen_t addrlen = sizeof(addr);
		SOCKET iAcceptSock = accept(m_iListenFd, (struct sockaddr *)&addr, &addrlen);
		if (INVALID_SOCKET != iAcceptSock )
		{
			if(tce::socket_setNCloseWait(iAcceptSock) < 0)
			{
				tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Accept socket_setNCloseWait error:errno=%d,errstr=%s",errno,strerror(errno));
				SSession stSession;
				DoError(stSession, TEC_UNKOWN_ERROR, m_szErrMsg);
			}

			tce::socket_setNBlock(iAcceptSock);

			if(!OnAccept(iAcceptSock,addr))
			{
				tce::socket_close(iAcceptSock);
			}

		}
		else
		{
#ifdef WIN32
			if ( WSAENOTSOCK != ::WSAGetLastError() )
			{
				printf("accept error:errno=%d.\n",WSAGetLastError());
			}
#else
			;
#endif
			bOk = false;
		}
		return bOk;
	}

	inline bool OnAccept(const SOCKET iFd, const sockaddr_in& addr)
	{
		bool bOk = false;

		CSocketSession* poSocket = m_oSocketSessionMgr.Malloc(iFd);
		if ( NULL != poSocket )
		{
			//poSocket->Reset();
			poSocket->SetFD(iFd);
			poSocket->SetReqeustFlag(true);
			poSocket->SetAddr(addr);
			poSocket->SetSessionID(++m_nSessionID);
			poSocket->SetStatus(CSocketSession::SST_ESTABLISHED);
			poSocket->SetLastAccessTime(m_dwCurTime);
			poSocket->SetCreateTime(m_dwCurTime);
			poSocket->SetOverTime(m_nOverTime);

			SetFdEvent(iFd, FD_CTL_ADD, FD_READ_EVENT | FD_ERROR_EVENT);
			bOk = true;	
		}
		else
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OnAccept<fd=%d,maxclient=%d>:socket_session malloc error :no session to malloc.", iFd, m_oSocketSessionMgr.GetMaxClientSize());
			SSession stSession;
			DoError(stSession, TEC_SOCKET_FULL, m_szErrMsg);
		}

		return bOk;
	}

	int32_t ReadWriteProcess(){

		tce::xsleep(m_nDelayStartTime * 1000);

		while ( m_bThreadRun )
		{
			m_dwCurTime = time(NULL);
			m_bSleep = true;

			if (!this->FdWait())
			{
				printf("EPoll Wait error:%s", m_szErrMsg);
			}
			
			if (!this->WriteProcess()) 
			{
				printf("WriteProcess error:%s", m_szErrMsg);
			}

			this->CheckOvertime();
			
			if ( m_bSleep ) xsleep(1);
		}

		return 0;
	}


	bool FdWaitByAccept(){

#ifdef WIN32
		struct timeval TimeVal;

		SetTriggerSocket();
		FD_SET(m_iListenFd, &m_Rfds);
		
		TimeVal.tv_sec = 0;
		TimeVal.tv_usec = 100*1000; //10ms
		int32_t nRes = ::select((int32_t)m_iMaxFd+1, &m_Rfds,&m_Wfds, &m_Efds,&TimeVal);
		if ( nRes>0 )
		{
			m_bSleep = false;
			if ( FD_ISSET(m_iListenFd, &m_Rfds) )
			{
				this->Accept();
			}

			CSocketSession* poSocket = NULL;
			for ( m_oClientFdMgr.Begin(); !m_oSocketSessionMgr.IsEnd(); )
			{
				poSocket = m_oSocketSessionMgr.Get();
				m_oSocketSessionMgr.Next();
				if ( NULL != poSocket )
				{
					if(FD_ISSET(poSocket->GetFD(), &m_Rfds))
					{
						if (!this->Read(poSocket))	
						{
							this->Close(poSocket);
							continue;
						}
					}

					if(FD_ISSET(poSocket->GetFD(), &m_Wfds))
					{
						if (!this->Write(poSocket))
						{
							this->Close(poSocket);
							continue;
						}
					}

					if(FD_ISSET(poSocket->GetFD(), &m_Efds))
					{
						this->Close(poSocket);
						continue;
					}
				}
			}
		}
		else
		{
			//			printf("select error:errno=%d.\n",WSAGetLastError());
		}

#else
		int32_t nfds=epoll_wait(m_iEpollFd, m_pEpollEvents, EPOLL_EVENT_COUNT, EPOLL_WAIT_TIMEOUT);
		if (nfds>0)
		{
			m_bSleep = false;
//			printf("nfds=%d\n", nfds);
			for (int32_t i=0; i<nfds; ++i)
			{
				epoll_event& ev = m_pEpollEvents[i];
				if(ev.events & EPOLLIN)
				{
					if ( ev.data.fd == m_iListenFd )
					{
//						printf("Accept=%d\n", nfds);
						this->Accept();
					}
					else
					{
//						printf("Read=%d\n", nfds);
						if (!this->Read(ev.data.fd)) 
						{
//							printf("Read Close=%d\n", nfds);
							this->Close(ev.data.fd);
						}
					}

				}
				else if(ev.events & EPOLLOUT)
				{  
					if (!this->Write(ev.data.fd))
					{
//						printf("Write Close=%d\n", ev.data.fd);
						this->Close(ev.data.fd);
					}
				}
				else
				{
					if(ev.events & EPOLLERR)
					{
//						printf("EPOLLERR Close=%d\n", ev.data.fd);
						this->Close(ev.data.fd);
					}
					else
					{
						if (ev.events & EPOLLHUP) {
							printf("close EPOLLHUP socket. ");
						}
					}
				}
			}
		}
		else if(nfds<0)
		{
			printf("[error] epoll_wait error:%s, %s,%d", strerror(errno), __FILE__, __LINE__);
			//			ldf::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg), "[error] epoll_wait error:%s, %s,%d", strerror(errno), __FILE__, __LINE__);
			//			SERVER_ERR_LOG1("[error] epoll_wait error:%s.", strerror(errno));
			return false;
		}

#endif
		return true;
	}



	bool FdWait(){

#ifdef WIN32
		struct timeval TimeVal;

		SetTriggerSocket();

		TimeVal.tv_sec = 0;
		TimeVal.tv_usec = 100*1000; //10ms
		int32_t nRes = ::select((int32_t)m_iMaxFd+1, &m_Rfds,&m_Wfds, &m_Efds,&TimeVal);
		if ( nRes>0 )
		{
			m_bSleep = false;
			CSocketSession* poSocket  = NULL;
			for ( m_oSocketSessionMgr.Begin(); !m_oSocketSessionMgr.IsEnd(); m_oSocketSessionMgr.Next() )
			{
				poSocket = m_oSocketSessionMgr.Get();
				if ( NULL != poSocket )
				{
					if(FD_ISSET(poSocket->GetFD(), &m_Rfds))
					{
						if (!this->Read(poSocket))	
						{
							this->Close(poSocket);
							continue;
						}
					}
					
					if(FD_ISSET(poSocket->GetFD(), &m_Wfds))
					{
						if ( poSocket->GetStatus() == CSocketSession::SST_CONNECTTING )
						{
							OnConnect(poSocket);
						}
						else
						{
							if (!this->Write(poSocket))
							{
								this->Close(poSocket);
								continue;
							}
						}

					}
					
					if(FD_ISSET(poSocket->GetFD(), &m_Efds))
					{
						this->Close(poSocket);
						continue;
					}
				}
			}
		}
		else
		{
//			printf("select error:errno=%d.\n",WSAGetLastError());
		}

#else
		int32_t nfds=epoll_wait(m_iEpollFd, m_pEpollEvents, EPOLL_EVENT_COUNT, EPOLL_WAIT_TIMEOUT);
		if (nfds>0)
		{
			int32_t iInCnt = 0;
			int32_t iOutCnt = 0;
			int32_t iErrCnt = 0;
			int32_t iConnectCnt = 0;
			int32_t iCloseCnt = 0;
			
			for (int32_t i=0; i<nfds; ++i)
			{
				epoll_event& ev = m_pEpollEvents[i];
				if(ev.events & EPOLLIN)
				{
					++iInCnt;
					
					CSocketSession* poSocket = GetSocketSession(ev.data.fd);
					if ( NULL != poSocket )
					{
						if ( CSocketSession::SST_CONNECTTING == poSocket->GetStatus() )
							++iConnectCnt ;
					}

					
					if (!this->Read(ev.data.fd)) 
					{
						++iCloseCnt;
//						printf("Read Close=%d\n", ev.data.fd);
						this->Close(ev.data.fd);
					}
				}
				else if(ev.events & EPOLLOUT)
				{  
					++iOutCnt;
					if (!this->Write(ev.data.fd))
					{
						//printf("Write Close=%d\n", ev.data.fd);
						this->Close(ev.data.fd);
					}
				}
				else
				{
					++iErrCnt;
					if(ev.events & EPOLLERR)
					{
						//printf("EPOLLERR Close=%d\n", ev.data.fd);
						fflush(stdout);
						this->Close(ev.data.fd);
					}
					else
					{
						if (ev.events & EPOLLHUP) {
							printf("close EPOLLHUP socket. ");
						}
					}
				}
			}
		}
		else if(nfds<0)
		{
//			ldf::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg), "[error] epoll_wait error:%s, %s,%d", strerror(errno), __FILE__, __LINE__);
			printf("[error] epoll_wait error:%s.\n", strerror(errno));
			return false;
		}

#endif
		return true;
	}
	
	void StatSocket(int32_t& iReadCnt, int32_t& iWriteCnt){
		CSocketSession* poSocket = NULL;
		for ( m_oSocketSessionMgr.Begin(); !m_oSocketSessionMgr.IsEnd();)
		{
			poSocket = m_oSocketSessionMgr.Get();
			m_oSocketSessionMgr.Next();
			if ( NULL != poSocket && CSocketSession::SST_ERR != poSocket->GetStatus() )
			{
				int32_t inlen = 0;
				int32_t outlen = 0;
				if ( ioctl(poSocket->GetFD(), SIOCINQ, &inlen) < 0 ) printf("ioctl SIOCINQ error\n");
				if ( ioctl(poSocket->GetFD(), SIOCOUTQ, &outlen) < 0 ) printf("ioctl error\n");
				if ( inlen > 0 ) ++iReadCnt;
				if ( outlen > 0 ) ++iWriteCnt;
			}
		}
	}
	
	int32_t GetSocketInLen(int32_t iFD)	{
		int32_t inlen = 0;
		if ( ioctl(iFD, SIOCINQ, &inlen) < 0 ) 
			inlen = -1;
		return inlen;
	}	
	int32_t GetSocketOutLen(int32_t iFD)	{
		int32_t outlen = 0;
		if ( ioctl(iFD, SIOCOUTQ, &outlen) < 0 ) 
			outlen = -1;
		return outlen;
	}
	
	void CheckClosing(){
		if ( m_dwCurTime >= m_nLastCheckClosingTime+1 )
		{
			m_dwCurTime = m_nLastCheckClosingTime;
			for ( std::list<int32_t>::iterator it=m_listClosingFds.begin(); it!=m_listClosingFds.end(); ) 
			{
				CSocketSession* poSocket = GetSocketSession(*it);
				if ( NULL != poSocket && CSocketSession::SST_CLOSING == poSocket->GetStatus() )
				{
					if ( m_dwCurTime >= poSocket->GetCloseTime()
						 ||(poSocket->GetOutBuffer().Size() == 0 && GetSocketOutLen(poSocket->GetFD()) <= 0) )
					{
						//printf("CheckClosing(comm=%d,fd=%d,id=%ld) CheckClose, outBuffer=%zd,outLen=%d\n", m_iCommID, poSocket->GetFD(), poSocket->GetSessionID(), poSocket->GetOutBuffer().Size(), GetSocketOutLen(poSocket->GetFD()));
						//this->Close(poSocket);
						m_listClosingFds.erase(it++);
						continue;
					}
				}
				else
				{
					//printf("CheckClosing(fd=%d) other close\n", *it);
					m_listClosingFds.erase(it++);
					continue;
				}
				++it;
			}			
		}
	}
	
	void CheckOvertime(){
		//check closing status
		//CheckClosing();
		
		if (m_dwCurTime > m_nLastCheckTime+5) 
		{
			m_bSleep = false;
			//检查CLient超时
			m_iMaxFd = 0;
			CSocketSession* poSocket = NULL;
			int32_t iNoReadyCnt = 0;
			for ( m_oSocketSessionMgr.Begin(); !m_oSocketSessionMgr.IsEnd();)
			{
				poSocket = m_oSocketSessionMgr.Get();
				m_oSocketSessionMgr.Next();
				if ( NULL != poSocket && CSocketSession::SST_ERR != poSocket->GetStatus() )
				{
					if ( m_iMaxFd < poSocket->GetFD()  )
						m_iMaxFd = poSocket->GetFD();

					if ( CSocketSession::SST_CLOSING == poSocket->GetStatus() )
					{	
//						printf("cloing fd=%d, closetime=%lu,curtime=%lu,writebufsize=%d\n", poSocket->GetFD(), poSocket->GetCloseTime(), m_dwCurTime, poSocket->GetOutBuffer().Size());
						// if ( m_dwCurTime >= poSocket->GetCloseTime() || poSocket->GetOutBuffer().Size() == 0 )
							// this->Close(poSocket);
					}
					
					if (CSocketSession::SST_CLOSE != poSocket->GetStatus()
						&& m_dwCurTime > poSocket->GetLastAccessTime() + poSocket->GetOverTime()) 
					{
						this->Close(poSocket);
					}
				}
				else
				{
					if ( NULL != poSocket )
					{
						if ( ++iNoReadyCnt > 10 )
							assert(false);
					}
				}
			}
			m_oSocketSessionMgr.SetMaxFD(m_iMaxFd);
			
			m_nLastCheckTime = m_dwCurTime;
		}
	}

#ifdef WIN32
	// return max value of fds been set
	void SetTriggerSocket()
	{//???Winsock2下的FD_SET效率有问题，可能需要自己重新定义

		FD_ZERO(&m_Rfds);
		FD_ZERO(&m_Wfds);
		FD_ZERO(&m_Efds);
		m_iMaxFd = 0;
		CSocketSession* poSocket = NULL;
		for ( m_oSocketSessionMgr.Begin(); !m_oSocketSessionMgr.IsEnd(); m_oSocketSessionMgr.Next() )
		{
			poSocket = m_oSocketSessionMgr.Get();
			if ( NULL != poSocket )
			{
				if ( m_iMaxFd < poSocket->GetFD()  )
					m_iMaxFd = poSocket->GetFD();

				FD_SET(poSocket->GetFD(), &m_Rfds);
				FD_SET(poSocket->GetFD(), &m_Efds);

				if ( poSocket->GetOutBuffer().Size() > 0 || poSocket->GetStatus() == CSocketSession::SST_CONNECTTING ) 
				{
					FD_SET(poSocket->iFd, &m_Wfds);
				}
			}
		}
	}
#endif

	inline void DoClosing(CSocketSession* poSocket, const time_t nCloseWaitTime)
	{
//		printf("DoClosing<comm=%d,fd=%d,id=%ld, ip=%s:%u>, outBuffer=%zd, outlen=%d\n", m_iCommID, poSocket->GetFD(), poSocket->GetSessionID(), inet_ntoa(poSocket->GetAddr().sin_addr), ntohs(poSocket->GetAddr().sin_port), poSocket->GetOutBuffer().Size(), GetSocketOutLen(poSocket->GetFD()));
		shutdown(poSocket->GetFD(), SHUT_RD);
		SetFdEvent(poSocket->GetFD(), FD_CTL_MOD, FD_WRITE_EVENT | FD_ERROR_EVENT);
		poSocket->SetStatus(CSocketSession::SST_CLOSING);
		poSocket->SetCloseTime(nCloseWaitTime+m_dwCurTime);
		if ( poSocket->GetOutBuffer().Size() == 0 && GetSocketOutLen(poSocket->GetFD()) <= 0 )
		{
			//printf("DoClosing<comm=%d,fd=%d,id=%ld>, close, outBuffer=%zd, outlen=%d\n", m_iCommID, poSocket->GetFD(), poSocket->GetSessionID(), poSocket->GetOutBuffer().Size(), GetSocketOutLen(poSocket->GetFD()));
			//this->Close(poSocket);
		}
		else
		{
			//printf("DoClosing<comm=%d,fd=%d,id=%ld>, m_listClosingFds, outBuffer=%zd, outlen=%d\n", m_iCommID, poSocket->GetFD(), poSocket->GetSessionID(), poSocket->GetOutBuffer().Size(), GetSocketOutLen(poSocket->GetFD()));
			//m_listClosingFds.push_back(poSocket->GetFD());
		}
	}
private:
	typedef int32_t (* THREADFUNC)(void *);
	typedef CThread<THREADFUNC> THREAD;	
	THREAD m_oListenThread;
	THREAD m_oReadWriteThread;
	THREAD m_oThreadByAll;
	
	void DoError(CSocketSession* poSocket, const int32_t iErrCode, const char* pszErrMsg){
		SSession stSession;
		stSession.SetID(poSocket->GetSessionID());
		stSession.SetFD(poSocket->GetFD());
		stSession.SetParam(poSocket->GetParam1(), poSocket->GetParam2());
		stSession.SetPeerAddr(poSocket->GetAddr());
		stSession.SetSocketCreateTime(poSocket->GetCreateTime());
		stSession.SetSocketStartReadTime(poSocket->GetStartReadTime());
		inner_DoError(stSession, iErrCode, pszErrMsg);
	}
	
	void DoError(const SSession& stSession, const int32_t iErrCode, const char* pszErrMsg)
	{
		SSession stSession2(stSession);
		stSession2.SetDataType(DT_ERROR);		
		inner_DoError(stSession2, iErrCode, pszErrMsg);
	}

	void inner_DoError(SSession& stSession, const int32_t iErrCode, const char* pszErrMsg){
		//printf("inner_DoError:%s\n", pszErrMsg);
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

	inline const char* GetBufferDataPtr(const CBuffer& oBuffer){
		const char* pData = NULL;
		if ( oBuffer.CanDirectCopy() )
		{
			pData = oBuffer.GetDirectData();
		}
		else
		{
			size_t nTmpSize = m_nFDTmpBufferSize;
			if ( !oBuffer.Get(m_pFDTmpBuffer, nTmpSize) )
			{
				assert(false);
				return NULL;
			}
			assert(nTmpSize==oBuffer.Size());
			pData = m_pFDTmpBuffer;
		}

		return pData;	
	}
private:

	time_t m_dwCurTime;
	volatile bool m_bThreadRun;
	SOCKET m_iListenFd;

	time_t m_nDelayStartTime;
	time_t m_nLastCheckTime;
	time_t m_nLastCheckClosingTime;
	time_t m_nOverTime;

	SessionManage m_oSocketSessionMgr;

	SOCKET m_iMaxFd;

	uint64_t m_nSessionID;

#ifdef WIN32
	fd_set m_Rfds, m_Wfds, m_Efds;
#else
	int32_t m_iEpollFd;						//epoll fd

	epoll_event* m_pEpollEvents;		//
#endif

	char* m_pFDTmpBuffer;	//大小为允许max fd data size
	size_t m_nFDTmpBufferSize;
	char* m_pSendTmpBuffer;	//大小为允许max fd data size
	size_t m_nSendTmpBufferSize;	
	
	bool  m_bSleep;

	std::list<int32_t> m_listClosingFds;
};


};//namespace tce

#endif 



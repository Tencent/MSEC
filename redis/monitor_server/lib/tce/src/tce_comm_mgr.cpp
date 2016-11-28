
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


#include "tce_comm_mgr.h"
#include "tce_tcp_server.h"
#include "tce_udp_server.h"
#include "tce_parse_pkg_by_http.h"
#include "tce_parse_pkg_by_head2longlentail3.h"
#include "tce_parse_pkg_by_head2shortlentail3.h"
#include "tce_parse_pkg_by_longlen.h"
#include "tce_parse_pkg_by_empty.h"
#include "tce_parse_tcp_pkg.h"
#include "tce_parse_pkg_by_http_response_part.h"
#include "tce_parse_pkg_asn.h"
#include "tce_socket_init.h"

#include <set>

using namespace std;

namespace tce{

CSocketInit g_oSocketInit;

CCommMgr* CCommMgr::m_pInstance = NULL;


CCommMgr::CCommMgr()
{
	m_bStart = false;
	m_iCommMaxId = MAX_COMM_SIZE+1;
	m_iUdpMaxCommID = -1;
	m_iTcpMaxCommID = 0;
	m_pOnTimerFunc = NULL;
	memset(m_szErrMsg, 0, sizeof(m_szErrMsg));
	memset(m_arComms, 0, sizeof(SCommConfig*)*MAX_COMM_SIZE);
	memset(m_arrRunComms, 0, sizeof(SRunComm)*MAX_COMM_SIZE);
	for ( int32_t i=1; i<MAX_COMM_SIZE; ++i )
	{
		m_deqIdleCommIDs.push_back(i);
	}
}

CCommMgr::~CCommMgr()
{
	m_bStart = false;
	for ( int32_t i=1; i<MAX_COMM_SIZE; ++i )
	{
		if ( NULL != m_arComms[i] )
		{
			delete m_arComms[i];
		}
	}
	m_setRunCommIDs.clear();
	
}



int32_t CCommMgr::CreateSvr(const COMM_TYPE nCommType, const std::string& sBindIp, const uint16_t wBindPort, const size_t nInBufferSize, const size_t nOutBufferSize)
{
	for ( int32_t i=1; i<MAX_COMM_SIZE; ++i )
	{
		
		if ( NULL != m_arComms[i] )
		{
			if ( m_arComms[i]->sIp == sBindIp && m_arComms[i]->wPort == wBindPort && m_arComms[i]->nCommType == nCommType )
			{
				tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "CreateSvr error: comm_type=%d,ip=%s,port=%u is exist in run list.", nCommType, sBindIp.c_str(), wBindPort);
				CallBackErrFunc(1, m_szErrMsg);
				return -1;
			}
		}
	}

	if ( m_deqIdleCommIDs.size() > 0 )
	{
		SCommConfig* pstComm = new SCommConfig;
		if ( NULL != pstComm )
		{
			pstComm->bRun = false;
			pstComm->nCommType = nCommType;
			pstComm->sIp = sBindIp;
			pstComm->wPort = wBindPort;
			pstComm->nInBufferSize = nInBufferSize;
			pstComm->nOutBufferSize = nOutBufferSize;
			int32_t iCommID = m_deqIdleCommIDs.front();
			m_arComms[iCommID] = pstComm;
			m_deqIdleCommIDs.pop_front();

			return iCommID;
		}
	}
	return -1;
}
bool CCommMgr::SetSvrDgramType(const int32_t iCommID, const TCP_DGRAM_TYPE nTcpDgramType)
{
	bool bOk = false;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDgramType[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( !pstComm->bRun )
		{
			pstComm->nTcpDgramType = nTcpDgramType;
			bOk = true;
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDgramType[comm=%d] running so that can't modify it.", iCommID);
			CallBackErrFunc(1, m_szErrMsg);
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDgramType[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;
}


bool CCommMgr::SetSvrClientOpt(const int32_t iCommID,
					const size_t nMaxClient,
					const time_t nOverTime,
					const size_t nMaxFdInSize,
					const size_t nMaxFdOutSize,
					const size_t nMaxMallocMem,
					const size_t nMallocItemSize)
{
	bool bOk = false;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrClientOpt[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( !pstComm->bRun )
		{
			pstComm->nMaxClient = nMaxClient;
			pstComm->nOverTime = nOverTime;
			pstComm->nMaxFdInSize = nMaxFdInSize;
			pstComm->nMaxFdOutSize = nMaxFdOutSize;
			pstComm->nMaxMallocMem = nMaxMallocMem;
			pstComm->nMallocItemSize = nMallocItemSize;
			bOk = true;
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrClientOpt[comm=%d] running so that can't modify it.", iCommID);
			CallBackErrFunc(1, m_szErrMsg);
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrClientOpt[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;
}

bool CCommMgr::SetSvrDelayStart(const int32_t iCommID, const size_t nDelayStartTime)
{
	bool bOk = false;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDelayStart[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( !pstComm->bRun )
		{
			pstComm->nDelayStartTime = nDelayStartTime;
			bOk = true;
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDelayStart[comm=%d] running so that can't modify it.", iCommID);
			CallBackErrFunc(1, m_szErrMsg);
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrDelayStart[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;
}

bool CCommMgr::SetSvrSockManageType(const int32_t iCommID, const SOCKET_MANAGE_TYPE nSockManageType)
{
	bool bOk = false;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrSockManageType[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( !pstComm->bRun )
		{
			pstComm->nSockManageType = nSockManageType;
			bOk = true;
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrSockManageType[comm=%d] running so that can't modify it.", iCommID);
			CallBackErrFunc(1, m_szErrMsg);
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrSockManageType[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;
}


bool CCommMgr::SetSvrCallbackFunc(const int32_t iCommID, ONREADFUNC pOnReadFunc, ONCLOSEFUNC pOnCloseFunc, ONCONNECTFUNC pOnConnectFunc, ONERRORFUNC pOnErrorFunc)
{
	bool bOk = false;
	m_pOnErrorFunc = pOnErrorFunc;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrCallbackFunc[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		pstComm->pOnReadFunc = pOnReadFunc;
		pstComm->pOnCloseFunc = pOnCloseFunc;
		pstComm->pOnConnectFunc = pOnConnectFunc;
		bOk = true;
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "SetSvrCallbackFunc[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;
}

bool CCommMgr::SetTimerCallbackFunc(ONTIMERFUNC pOnTimerFunc)
{
	m_pOnTimerFunc = pOnTimerFunc;
	return true;
}


bool CCommMgr::RunAllSvr()
{
	for ( int32_t i=1; i<MAX_COMM_SIZE; ++i )
	{
		SCommConfig* pstComm = m_arComms[i];
		if ( NULL != pstComm &&  !pstComm->bRun )
		{
			if ( !this->RunSvr( i ) )
			{
				return false;
			}
		}
	}

	return true;
}

bool CCommMgr::RunSvr(const int32_t iCommID)
{
	bool bOk = true;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( !pstComm->bRun )
		{
			switch( pstComm->nCommType ){
			case CT_UDP_SVR:
				{
					CUdpServer* poUdpSvr = new CUdpServer;
					if ( poUdpSvr->Init(iCommID, pstComm->sIp, pstComm->wPort, pstComm->nInBufferSize, pstComm->nOutBufferSize) )
					{
						pstComm->poInBuffer = poUdpSvr->GetInBuffer();
						pstComm->poOutBuffer = poUdpSvr->GetOutBuffer();
						pstComm->poSvr = poUdpSvr;

						pstComm->bRun = true;
						bOk = poUdpSvr->Start();
						m_setRunCommIDs.insert(iCommID);
					}
					else
					{
						tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d,ip=%s,port=%d]create udp error: %s.", iCommID, tce::InetNtoA(poUdpSvr->GetIp()).c_str(), poUdpSvr->GetPort(),  poUdpSvr->GetErrMsg());
						delete poUdpSvr;
						CallBackErrFunc(1, m_szErrMsg);
						bOk = false;
					}
				}
				break;
			case CT_HTTP_SVR:
			{
				if ( pstComm->nTcpDgramType != TDT_HTTP_RESPONSE_PART )
					pstComm->nTcpDgramType = TDT_HTTP;
			}
			case CT_TCP_SVR:
				{
					CServerBase* poSvr = NULL;
					switch(pstComm->nTcpDgramType){
					case TDT_H2SHORTT3:	
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHead2ShortLenTail3, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHead2ShortLenTail3>;
						}
						break;
					case TDT_H2LONGT3:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHead2LongLenTail3, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHead2LongLenTail3>;
						}
						break;
					case TDT_HTTP:	
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHttp, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHttp>;
						}
						break;
					case TDT_HTTP_RESPONSE_PART:	
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHttpResponsePart, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHttpResponsePart>;
						}
						break;						
					case TDT_LONGLEN_EXCLUDE_ITSELF:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByLongLenExc, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByLongLenExc>;
						}
						break;
					case TDT_LONGLEN_INCLUDE_ITSELF:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByLongLenInc, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByLongLenInc>;
						}
						break;
					case TDT_H5_VER_SHORT_T3:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHead5ShortVerShortLenTail3, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHead5ShortVerShortLenTail3>;
						}
						break;
					case TDT_H5SHORTT3:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByHead5ShortLenTail3, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByHead5ShortLenTail3>;
						}
						break;
					case TDT_EMPTY:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgByEmpty, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgByEmpty>;
						}
						break;
					case TDT_ASN:
						{
							if ( SMT_ARRAY_FAST == pstComm->nSockManageType )
								poSvr = new CTcpServer<CParsePkgAsn, CSessionManageByFast<CSocketSession> >;
							else
								poSvr = new CTcpServer<CParsePkgAsn>;
						}
						break;
					default:
						tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d,TcpDgramType=%d]unkown TcpDgramType.", iCommID, pstComm->nTcpDgramType);
						CallBackErrFunc(1, m_szErrMsg);
						bOk = false;
						break;
					}

					if ( NULL != poSvr )
					{
						if ( poSvr->Init(iCommID, 
											ntohl(inet_addr(pstComm->sIp.c_str())), 
											pstComm->wPort, 
											pstComm->nInBufferSize, 
											pstComm->nOutBufferSize,
											pstComm->nMaxFdInSize,
											pstComm->nMaxFdOutSize,
											pstComm->nMaxMallocMem,
											pstComm->nMallocItemSize,
											pstComm->nMaxClient,
											pstComm->nOverTime,
											pstComm->nDelayStartTime) )
						{
				
							pstComm->poInBuffer = poSvr->GetInBuffer();
							pstComm->poOutBuffer = poSvr->GetOutBuffer();
							pstComm->poSvr = poSvr;

							pstComm->bRun = true;
							bOk = poSvr->Start();
							m_setRunCommIDs.insert(iCommID);
						}
						else
						{
							tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d,ip=%s,port=%d]create tcp svr error: %s.", iCommID, tce::InetNtoA(poSvr->GetIp()).c_str(), poSvr->GetPort(),  poSvr->GetErrMsg());
							delete poSvr;
							CallBackErrFunc(1, m_szErrMsg);
							bOk = false;
						}		
					}
				
				}
				break;
			default:
				tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d,ip=%s,port=%d] unkown commtype<%d>.", iCommID, pstComm->sIp.c_str(), pstComm->wPort, pstComm->nCommType);
				bOk = false;
				CallBackErrFunc(1, m_szErrMsg);
				break;
			}
		}
	}
	else 
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "RunSvr[comm=%d]can't find svr in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		bOk = false;
	}

	return bOk;
}

bool CCommMgr::StopSvr(const int32_t iCommID)
{
	bool bOk = false;

	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "StopSvr[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		if ( pstComm->bRun )
		{
			pstComm->poSvr->Stop();
			delete pstComm->poSvr;
			pstComm->poSvr = NULL;
			pstComm->poInBuffer = NULL;
			pstComm->poOutBuffer = NULL;

			pstComm->bRun = false;
			m_setRunCommIDs.erase(iCommID);
			bOk = true;
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "StopSvr[comm=%d] no running so that can't stop it.", iCommID);
			CallBackErrFunc(1, m_szErrMsg);
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "StopSvr[comm=%d]can't find comm in no run list.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
	}

	return bOk;

}

bool CCommMgr::CloseSvr(const int32_t iCommID)
{
	if ( iCommID <= 0 || MAX_COMM_SIZE <= iCommID )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "CloseSvr[comm=%d] commID overflow error.", iCommID);
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	
	StopSvr(iCommID);
	
	SCommConfig* pstComm = m_arComms[iCommID];
	if ( NULL != pstComm)
	{
		m_deqIdleCommIDs.push_back(iCommID);
		delete pstComm;
		m_arComms[iCommID] = NULL;
		pstComm = NULL;
	}
	return true;
}

bool CCommMgr::StopAllSvr()
{
	for ( int32_t i=1; i<MAX_COMM_SIZE; ++i )
	{
		SCommConfig* pstComm = m_arComms[i];
		if ( NULL != pstComm &&  pstComm->bRun )
		{
			this->StopSvr( i );
		}
	}

	return true;
}

bool CCommMgr::StopTimer(){
	return m_oTimer.Stop();
}

void CCommMgr::Stop()
{
	m_bStart = false;
	this->StopTimer();
	this->StopAllSvr();
}

bool CCommMgr::Write(SSession& stSession,const char* pszData1, const size_t nSize1,const char* pszData2, const size_t nSize2)
{
	bool bOk = true;

	if ( stSession.GetCommID() <= 0 || MAX_COMM_SIZE <= stSession.GetCommID() )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "Write[comm=%d] commID overflow error.", stSession.GetCommID());
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[stSession.GetCommID()];
	if ( NULL != pstComm && pstComm->bRun )
	{
		CFIFOBuffer* poOutBuffer = pstComm->poOutBuffer;
		if( poOutBuffer->GetSize() >= nSize1 + nSize2 + sizeof(stSession) )
		{
			CFIFOBuffer::RETURN_TYPE nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession), pszData1, nSize1, pszData2, nSize2);
			while ( CFIFOBuffer::BUF_FULL == nRe )
			{
				tce::xsleep(1);
				nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession), pszData1, nSize1, pszData2, nSize2);
				if ( CFIFOBuffer::BUF_OK != nRe )
				{
					tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu,ip=%s,port=%d]buffer write error(%d):%s",stSession.GetCommID(), stSession.GetID(),stSession.GetIP().c_str(), stSession.GetPort(), nRe,poOutBuffer->GetErrMsg());
					CallBackErrFunc(1, m_szErrMsg);
					bOk = false;
				}	
				break;
			}
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu]buffer<datasize=%d,buffersize=%d> write error: data too large",stSession.GetCommID(), stSession.GetID(), nSize1 + nSize2, poOutBuffer->GetSize());
			CallBackErrFunc(1, m_szErrMsg);
			bOk = false;
		}

	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d]buffer write error: can't find buffer",stSession.GetCommID());
		CallBackErrFunc(1, m_szErrMsg);
		bOk = false;
	}

	return bOk;
}


bool CCommMgr::Write(SSession& stSession,const char* pszData, const size_t nSize)
{
	bool bOk = true;
	if ( stSession.GetCommID() <= 0 || MAX_COMM_SIZE <= stSession.GetCommID() )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "Write[comm=%d] commID overflow error.", stSession.GetCommID());
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[stSession.GetCommID()];
	if ( NULL != pstComm && pstComm->bRun )
	{
		CFIFOBuffer* poOutBuffer = pstComm->poOutBuffer;
		if( poOutBuffer->GetSize()  >= nSize + sizeof(stSession) )
		{
			CFIFOBuffer::RETURN_TYPE nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession), pszData, nSize);
			while ( CFIFOBuffer::BUF_FULL == nRe )
			{
				tce::xsleep(1);
				nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession), pszData, nSize);
				if ( CFIFOBuffer::BUF_OK != nRe )
				{
					tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu,ip=%s,port=%d]buffer write error(%d):%s",stSession.GetCommID(), stSession.GetID(), stSession.GetIP().c_str(), stSession.GetPort(), nRe,poOutBuffer->GetErrMsg());
					CallBackErrFunc(1, m_szErrMsg);
					bOk = false;
				}		
				break;
			}
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu]buffer<datasize=%d,buffersize=%d> write error: data too large",stSession.GetCommID(), stSession.GetID(), nSize, poOutBuffer->GetSize());
			CallBackErrFunc(1, m_szErrMsg);
			bOk = false;
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu]buffer write error: can't find buffer",stSession.GetCommID(), stSession.GetID());
		CallBackErrFunc(1, m_szErrMsg);
		bOk = false;
	}

	return bOk;
}


bool CCommMgr::WriteTo(const int32_t iCommID, const std::string& sIp, const uint16_t wPort, const unsigned char* pszData, const size_t nSize)
{
	SSession stSession;
	struct sockaddr_in peerAddr={0};
	peerAddr.sin_family = AF_INET;;
	peerAddr.sin_addr.s_addr = inet_addr(sIp.c_str());
	peerAddr.sin_port = htons(wPort);
	stSession.SetCommID(iCommID);
	stSession.SetDataType(DT_UDP_DATA);
	stSession.SetPeerAddr(peerAddr);
	return this->Write(stSession, pszData, nSize);
}


bool CCommMgr::Close(SSession& stSession, const time_t nCloseWaitTime)
{
	bool bOk = true;
	stSession.SetDataType(DT_TCP_CLOSE);
	stSession.SetCloseWaitTime(nCloseWaitTime);

	if ( stSession.GetCommID() <= 0 || MAX_COMM_SIZE <= stSession.GetCommID() )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "Close[comm=%d] commID overflow error.", stSession.GetCommID());
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[stSession.GetCommID()];
	if ( NULL != pstComm && pstComm->bRun )
	{
		CFIFOBuffer* poOutBuffer = pstComm->poOutBuffer;
		CFIFOBuffer::RETURN_TYPE nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession));
		while ( CFIFOBuffer::BUF_FULL == nRe )
		{
			tce::xsleep(1);
			nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession));
			if ( CFIFOBuffer::BUF_OK != nRe )
			{
				tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write Close[comm=%d,session=%llu,ip=%s, port=%d]buffer write error(%d):%s",stSession.GetCommID(), stSession.GetID(),stSession.GetIP().c_str(), stSession.GetPort(), nRe,poOutBuffer->GetErrMsg());
				CallBackErrFunc(1, m_szErrMsg);
				bOk = false;
			}
			break;
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu]buffer write error: can't find buffer",stSession.GetCommID(), stSession.GetID());
		CallBackErrFunc(1, m_szErrMsg);
		bOk = false;
	}

	return bOk;
}

	
bool CCommMgr::Connect(const int iCommID, const std::string& sIp, const uint16_t wPort, const int64_t nParam1, const void* pParam2)
{
	 return this->Connect(iCommID,  ntohl(inet_addr(sIp.c_str())), wPort, nParam1, pParam2);
}

bool CCommMgr::Connect(const int32_t iCommID, const uint32_t dwIp, const uint16_t wPort, const int64_t nParam1, const void* pParam2)
{
	SSession stSession;
	stSession.SetCommID(iCommID);
	stSession.SetDataType(DT_TCP_CONNECT);
	struct sockaddr_in peerAddr={0};
	peerAddr.sin_family = AF_INET;;
	peerAddr.sin_addr.s_addr = htonl(dwIp);
	peerAddr.sin_port = htons(wPort);
	stSession.SetPeerAddr(peerAddr);
	stSession.SetParam(nParam1,pParam2);

	bool bOk = true;

	if ( stSession.GetCommID() <= 0 || MAX_COMM_SIZE <= stSession.GetCommID() )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "Connect[comm=%d] commID overflow error.", stSession.GetCommID());
		CallBackErrFunc(1, m_szErrMsg);
		return false;
	}
	SCommConfig* pstComm = m_arComms[stSession.GetCommID()];
	if ( NULL != pstComm && pstComm->bRun )
	{
		CFIFOBuffer* poOutBuffer = pstComm->poOutBuffer;
		CFIFOBuffer::RETURN_TYPE nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession));
		while ( CFIFOBuffer::BUF_FULL == nRe )
		{
			tce::xsleep(1);
			nRe = poOutBuffer->Write((char*)&stSession, sizeof(stSession));
			if ( CFIFOBuffer::BUF_OK != nRe )
			{
				tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write Connect[comm=%d,session=%llu,ip=%s, port=%d]buffer write error(%d):%s",stSession.GetCommID(), stSession.GetID(), stSession.GetIP().c_str(), stSession.GetPort(), nRe,poOutBuffer->GetErrMsg());
				CallBackErrFunc(1, m_szErrMsg);
				bOk = false;
			}	
			break;	
		}
	}
	else
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"Write[comm=%d,session=%llu]buffer write error: can't find buffer",stSession.GetCommID(), stSession.GetID());
		CallBackErrFunc(1, m_szErrMsg);
		bOk = false;
	}

	return bOk;

}


bool CCommMgr::RunTimer()
{
	m_oTimer.Start();
	return true;
}

bool CCommMgr::Start()
{
	//Æô¶¯¶¨Ê±Æ÷
	this->RunTimer();
	m_bStart = true;

	SSession* pstSession = NULL;
	CTimer<int64_t>::STimerCmd stTimer;
	memset((char*)&stTimer, 0, sizeof(stTimer));
	bool bSleep = false;
	
	//add run comms
	int32_t i=0;
	for ( SET_COMMID::iterator it=m_setRunCommIDs.begin(); it!=m_setRunCommIDs.end(); ++it,++i )
	{
		m_arrRunComms[i].poInBuffer = m_arComms[*it]->poInBuffer;
		m_arrRunComms[i].iCommID = *it;
		m_arrRunComms[i].pstCommConfig = m_arComms[*it];
	}
	int32_t iRunCommSize = m_setRunCommIDs.size();

	//event param
	int32_t iMsgID = 0;
	uint64_t ulParam = 0;
	void* pParam = NULL;

	while (m_bStart)
	{
		bSleep = true;

		for ( int32_t i=0; i<iRunCommSize; ++i )
		{
			SRunComm& stRunComm = m_arrRunComms[i];
			CFIFOBuffer* poInBuffer = stRunComm.poInBuffer;
			CFIFOBuffer::RETURN_TYPE nRe = poInBuffer->ReadNext();
			if ( CFIFOBuffer::BUF_OK == nRe )
			{ 
				if ( poInBuffer->GetCurDataLen() >= (int32_t)sizeof(SSession) )
				{
					pstSession = (SSession*)poInBuffer->GetCurData();
					pstSession->SetCommID(stRunComm.iCommID);

					int32_t iRe = this->OnRead(*stRunComm.pstCommConfig, *pstSession, poInBuffer->GetCurData()+sizeof(SSession), poInBuffer->GetCurDataLen()-sizeof(SSession));
					if ( -1 == iRe && pstSession->GetDataType()!=DT_UDP_DATA)
					{
						this->Close(*pstSession);
					}
				}
				else
				{
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[comm=%lu,session_size=%d,read_size=%d]read buf size error.", stRunComm.iCommID, sizeof(SSession), poInBuffer->GetCurDataLen());
					CallBackErrFunc(1, m_szErrMsg);
				}

				poInBuffer->MoveNext();
				bSleep = false;
			}
			else if ( CFIFOBuffer::BUF_EMPTY == nRe )
			{
			}
			else
			{
				bSleep = true;
				tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[comm=%lu]read buf error:%s", stRunComm.iCommID, poInBuffer->GetErrMsg());
				CallBackErrFunc(1, m_szErrMsg);
			}
		}

		//read timer 
		if ( m_oTimer.GetTimeoutTimer(stTimer) )
		{
			if ( NULL != m_pOnTimerFunc )
			{
				if ( stTimer.pFunc != NULL )
				{
					stTimer.pFunc();
				}
				else
				{
					m_pOnTimerFunc(stTimer.id);
				}
			}
			bSleep = false;
		}
		else
		{
		}

		//read event
		for ( MAP_EVENT::iterator itEvent=m_mapEvents.begin(); itEvent!=m_mapEvents.end(); ++itEvent )
		{
			if ( itEvent->second->GetMessage(iMsgID, ulParam, pParam) )
			{
				itEvent->second->OnMessage(iMsgID, ulParam, pParam);
				bSleep = false;
			}
		}

		if ( bSleep )
		{
//			static int iCnt = 0;
//			if ( ++iCnt %1000 == 0 )
				tce::xsleep(1);
		}

	}

	return true;
}

int32_t CCommMgr::OnRead(SCommConfig& stCommConfig, SSession& stSession, const unsigned char* pszData, const size_t nSize)
{
	int32_t iOk = 0;
	switch( stSession.GetDataType() )
	{
	case DT_TCP_DATA:
	case DT_UDP_DATA:
		if ( NULL != stCommConfig.pOnReadFunc )
		{
			iOk = stCommConfig.pOnReadFunc(stSession, pszData, nSize);
		}
		break;
	case DT_TCP_CLOSE:
		if ( NULL != stCommConfig.pOnCloseFunc )
		{
			stCommConfig.pOnCloseFunc(stSession);
		}
	    break;
	case DT_TCP_CONNECT:
		if ( NULL != stCommConfig.pOnConnectFunc )
		{
			stSession.SetDataType(DT_TCP_DATA);
			stCommConfig.pOnConnectFunc(stSession, true);
		}
	    break;
	case DT_TCP_CONNECT_ERR:
		if ( NULL != stCommConfig.pOnConnectFunc )
		{
			stSession.SetDataType(DT_TCP_DATA);
			stCommConfig.pOnConnectFunc(stSession, false);
		}
		break;
	case DT_ERROR:
		if ( NULL != m_pOnErrorFunc )
		{
			m_pOnErrorFunc(stSession, *(int32_t*)pszData, (const char*)pszData+4);
		}
		else
		{
			printf("[tce_error]errcode=%d, msg=%s", *(int32_t*)pszData, (const char*)pszData+4);
		}
		break;
	default:
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "[comm=%lu]unkown stSession.nDataType<%d>", stSession.GetCommID(), stSession.GetDataType());
		CallBackErrFunc(1, m_szErrMsg);
	    break;
	}

	return iOk;
}



int CCommMgr::AddEvent(CEvent* poEvent)
{
	poEvent->SetEventID(++m_iCommMaxId);
	m_mapEvents[m_iCommMaxId] = poEvent;
	return 0;
}


};

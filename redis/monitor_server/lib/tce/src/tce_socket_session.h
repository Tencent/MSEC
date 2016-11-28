
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


#ifndef __TCE_SOCKET_SESSION_H__
#define __TCE_SOCKET_SESSION_H__

#include "tce_socket_api.h"
#include "tce_buffer.h"
#include <time.h>

namespace tce{

	class CSocketSession{

		enum {
			ALLOW_MAX_SIZE=4*1024*1024,
		};
	public:
		enum SOCKET_STATUS_TYPE{
			SST_ERR,
			SST_CLOSE,
			SST_CLOSING,
			SST_CONNECTTING,
			SST_CONNECT_ERR,
			SST_ESTABLISHED,
		};

		CSocketSession(CBufferMalloc* poMalloc, const size_t nMaxInDataSize=ALLOW_MAX_SIZE, const size_t nMaxOutDataSize=ALLOW_MAX_SIZE)
			:m_nStatus(SST_ERR)
			,m_bFirstPkg(true)
			,m_bRequest(true)
			,m_nSessionID(1)
			,m_iFD(-1)
			,m_nParam1(0)
			,m_pParam2(NULL)
			,m_nCreateTime(0)
			,m_nOverTime(180)
			,m_nLastAccessTime(0)
			,m_nCloseTime(0)
			,m_oInBuffer(poMalloc, nMaxInDataSize)
			,m_oOutBuffer(poMalloc, nMaxOutDataSize)
		{
			memset(&m_peerAddr, 0, sizeof(sockaddr_in));
			memset(&m_tvCreateTime, 0, sizeof(m_tvCreateTime));
			memset(&m_tvLastAccessTime, 0, sizeof(m_tvLastAccessTime));
			memset(&m_tvStartReadTime, 0, sizeof(m_tvStartReadTime));
		}
		~CSocketSession(){}

		void Reset(){
			m_nStatus = SST_ERR;
			m_nLastAccessTime = 0;
			m_nCreateTime = 0;
			m_nParam1 = 0;
			m_pParam2 = NULL;
			m_iFD = -1;
			m_nSessionID = 0;
			m_bFirstPkg = true;
			m_bRequest = true;
			m_nCloseTime = 0;
			m_oInBuffer.Clear();
			m_oOutBuffer.Clear();
			memset(&m_peerAddr, 0, sizeof(sockaddr_in));
			memset(&m_tvCreateTime, 0, sizeof(m_tvCreateTime));
			memset(&m_tvLastAccessTime, 0, sizeof(m_tvLastAccessTime));
			memset(&m_tvStartReadTime, 0, sizeof(m_tvStartReadTime));
			
			m_nOverTime = 180;
		}

		inline SOCKET_STATUS_TYPE GetStatus() const {	return m_nStatus;	}
		inline void SetStatus(const SOCKET_STATUS_TYPE nStatus) {	m_nStatus = nStatus;	}
		
		time_t GetCreateInterval() {
			timeval tvCurTime;
			gettimeofday(&tvCurTime, 0);
			uint64_t ui64CurTime = (uint64_t)tvCurTime.tv_sec*1000000+tvCurTime.tv_usec;
			uint64_t ui64StartTime = (uint64_t)m_tvCreateTime.tv_sec*1000000+m_tvCreateTime.tv_usec;
			if (ui64StartTime < ui64CurTime )
				return ui64CurTime-ui64StartTime;	
			return 0;
		}
		time_t GetAccess2CreateInterval() {
			uint64_t ui64CreateTime = (uint64_t)m_tvCreateTime.tv_sec*1000000+m_tvCreateTime.tv_usec;
			uint64_t ui64LastAccessTime = (uint64_t)m_tvLastAccessTime.tv_sec*1000000+m_tvLastAccessTime.tv_usec;
			if (ui64LastAccessTime > ui64CreateTime )
				return ui64LastAccessTime - ui64CreateTime;	
			return 0;
		}
		inline void SetCreateTime(const time_t nTime)	{	m_nCreateTime = nTime; gettimeofday(&m_tvCreateTime, NULL);}
		inline timeval GetCreateTime() const {	return m_tvCreateTime;	}
		inline void SetLastAccessTime(const time_t nTime) {	m_nLastAccessTime = nTime;	/*gettimeofday(&m_tvLastAccessTime, NULL);*/}
		inline time_t GetLastAccessTime() const {	return m_nLastAccessTime;	}
		inline time_t GetOverTime() const	{	return m_nOverTime;	}
		inline void SetOverTime(const time_t nTime)	{	m_nOverTime = nTime;	}

		//由上层使用，会原值返回
		inline int64_t GetParam1() const	{	return m_nParam1;	}
		inline void* GetParam2() const	{	return m_pParam2;	}
		inline void SetParam(const int64_t nParam1, void* pParam2){
			m_nParam1 = nParam1;
			m_pParam2 = pParam2;
		}

		inline void SetAddr(const sockaddr_in& addr){	m_peerAddr = addr;	}
		inline const sockaddr_in& GetAddr() const	{	return m_peerAddr;		}
		inline SOCKET GetFD()	const {	return m_iFD;	}
		inline void SetFD(const SOCKET iFD){	assert(iFD != -1 );m_iFD = iFD;	}
		inline uint64_t GetSessionID() const {	return m_nSessionID;	}
		inline void SetSessionID(const uint64_t nID) {	m_nSessionID = nID;	}

		//检查某种代理在数据开始加信息使用
		inline bool IsFirstPkg()	const {	return m_bFirstPkg;	}
		inline void SetFirstPkgFlag(const bool bFlag){	m_bFirstPkg = bFlag;	}

		inline bool IsRequest()	const {	return m_bRequest;	}
		inline void SetReqeustFlag(const bool bFlag){	m_bRequest = bFlag;	}
		
		inline CBuffer& GetInBuffer()	{	return m_oInBuffer;	}
		inline CBuffer& GetOutBuffer()	{	return m_oOutBuffer;	}
		inline const CBuffer& GetInBuffer() const	{	return m_oInBuffer;	}
		inline const CBuffer& GetOutBuffer() const	{	return m_oOutBuffer;	}

		inline time_t GetCloseTime() const {	return m_nCloseTime;	}	
		inline void SetCloseTime(const time_t nTime) {	m_nCloseTime = nTime;	}
		
		inline void SetStartReadTime()	{	gettimeofday(&m_tvStartReadTime, NULL);}
		inline timeval GetStartReadTime() const {	return m_tvStartReadTime;	}		
	private:
		CSocketSession(const CSocketSession&);
		CSocketSession& operator = (const CSocketSession&);
	private:

		volatile SOCKET_STATUS_TYPE m_nStatus;
		bool m_bFirstPkg;
		bool m_bRequest;
		volatile uint64_t m_nSessionID;
		volatile SOCKET m_iFD;
		sockaddr_in m_peerAddr;	
		
		int64_t m_nParam1;
		void* m_pParam2;
		timeval m_tvCreateTime;	//建立时间
		timeval m_tvStartReadTime;	//第一次读取数据时间
		time_t m_nCreateTime;	//建立时间
		time_t m_nOverTime;//超时时间
		timeval m_tvLastAccessTime;
		time_t m_nLastAccessTime;	//上次访问时间
		time_t m_nCloseTime;	//
		CBuffer m_oInBuffer;
		CBuffer m_oOutBuffer;
	};


};

#endif



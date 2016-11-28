
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


#ifndef __TCE_SERVER_BASE_H__
#define __TCE_SERVER_BASE_H__

#include "tce_socket_api.h"
#include "tce_utils.h"
#include "fifo_buffer.h"

namespace tce{

enum DATA_TYPE{
	DT_ERROR,
	DT_TCP_DATA,
	DT_TCP_ACCEPT,
	DT_TCP_CLOSE,			//链接关闭
	DT_TCP_CONNECT,			//连接请求
	DT_TCP_CONNECT_ERR,		//连接失败
	DT_UDP_DATA,			//UDP数据
	DT_ADD_FORBID_IP,	//添加禁止访问IP
	DT_DEL_FORBID_IP,	//删除禁止访问IP
	DT_ADD_ALLOW_IP,	//添加允许访问IP
	DT_DEL_ALLOW_IP,	//删除允许访问IP
};

struct SSession{
	friend bool operator==(const SSession& stSession1,const SSession& stSession2);

	SSession(){		memset(this,0,sizeof(SSession));	}
	
	//////////////////////////////////////
	//应用层使用的接口(业务开发)
	//ip & port接口
	inline std::string GetIPByStr()	{	return inet_ntoa(m_peerAddr.sin_addr);	}
	inline uint32_t GetIPByNum()	const {	return ntohl(m_peerAddr.sin_addr.s_addr);	}
	inline uint16_t GetPort()	{	return ntohs(m_peerAddr.sin_port);	}
	//old 将废弃
	inline std::string GetIP()	{	return inet_ntoa(m_peerAddr.sin_addr);	}
	inline uint32_t GetIP2()	const {	return ntohl(m_peerAddr.sin_addr.s_addr);	}
	
	//session 唯一标识
	inline uint64_t GetID()	{	return m_nSessionID;	}
	//comm id
	inline int32_t GetCommID() {	return m_iCommID;	}

	//user param(应用层使用，会原值返回)
	inline void SetParam(const int64_t nParam1, const void* pParam2){
		m_nParam1 = nParam1;
		m_pParam2 = (void*)pParam2;
	}
	int64_t GetParam1()	const {	return m_nParam1;	}
	void* GetParam2()	{	return m_pParam2;	}
	void* GetParam2() const	{	return m_pParam2;	}

	/////////////////////////////////////
	//底层使用的接口
	inline DATA_TYPE GetDataType() const {	return m_nDataType;	}
	inline void SetDataType(const DATA_TYPE nType) {	m_nDataType = nType;	}
	inline time_t GetCloseWaitTime() const {	return m_nCloseWaitTime;	}
	inline void SetCloseWaitTime(time_t nTime) {	m_nCloseWaitTime = nTime;	}
	inline void SetBeginTime(const time_t nTime){	m_nBeginTime = nTime;	}
	inline void SetPeerAddr(const sockaddr_in& peerAddr)	{	m_peerAddr = peerAddr;	}
	inline const sockaddr_in& GetPeerAddr() const	{	return m_peerAddr;	}
	inline void SetCommID(const int32_t iID)	{	m_iCommID = iID;	}
	inline void SetFD(const SOCKET iFD)	{	m_iFD = iFD;	}
	inline SOCKET GetFD()	 const {	return m_iFD;	}
	inline void SetID(const uint64_t nSessionID)	{	m_nSessionID = nSessionID;	}

	time_t GetDelayTime() const {		return tce::GetTickCount()-m_nBeginTime;	}
	timeval GetSocketCreateTime() const {	return m_tvSocketCreateTime;	}
	void SetSocketCreateTime(timeval time) {	m_tvSocketCreateTime = time;	}
	timeval GetSocketStartReadTime() const {	return m_tvSocketStartReadTime;	}
	void SetSocketStartReadTime(timeval time) {	m_tvSocketStartReadTime = time;	}
private:
	DATA_TYPE m_nDataType;
	time_t m_nCloseWaitTime;			//closewait等待时间秒数
	time_t m_nBeginTime;
	struct sockaddr_in m_peerAddr;
	int32_t m_iCommID;
	SOCKET m_iFD;						//tcp use
	uint64_t m_nSessionID;
	int64_t m_nParam1;		//由上层使用，会原值返回
	void* m_pParam2;		//由上层使用，会原值返回
	timeval m_tvSocketCreateTime;	//Socket建立时间	
	timeval m_tvSocketStartReadTime;		//第一次读取数据时间

	};

inline bool operator==(const SSession& stSession1,const SSession& stSession2){
	return  ( stSession1.m_nSessionID == stSession2.m_nSessionID && stSession1.m_iCommID == stSession2.m_iCommID );
}


class CServerBase{
public:
	CServerBase()
		:m_poInBuffer(NULL)
		,m_poOutBuffer(NULL)

	{
		m_iCommID = 0;
		m_wSvrPort = 0;
		m_dwSvrIp = 0;
		memset(m_szErrMsg, 0, sizeof(m_szErrMsg));
	}

	//开始服务
	virtual bool Init(const int32_t iCommID,
		const uint32_t dwBindIp,
		const uint16_t wPort,	
		const size_t nInBufferSize,
		const size_t nOutBufferSize,
		const size_t nMaxFdInSize,
		const size_t nMaxFdOutSize,
		const size_t nMaxMallocMem,
		const size_t nMallocItemSize,
		const size_t nMaxClient, 
		const time_t nOverTime,
		const time_t nDelayStartTime){	return false;}
	virtual bool Start()=0;
	virtual bool Stop()=0;
	virtual ~CServerBase(){}

	const char* GetErrMsg() {	return m_szErrMsg; }
	CFIFOBuffer* GetInBuffer() {	return m_poInBuffer;	}
	CFIFOBuffer* GetOutBuffer() {	return m_poOutBuffer;	}

	int32_t GetId() const {	return m_iCommID; }
	uint32_t GetIp() const {	return m_dwSvrIp;	}
	const char* GetIpStr() const {	return InetNtoA(m_dwSvrIp).c_str();	}
	uint16_t GetPort() const {	return m_wSvrPort;	}

protected:
	int32_t m_iCommID;
	uint32_t m_dwSvrIp;
	uint16_t m_wSvrPort;
	char m_szErrMsg[256];
	CFIFOBuffer* m_poInBuffer;
	CFIFOBuffer* m_poOutBuffer;

};

};

#endif


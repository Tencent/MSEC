
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


#ifndef __TCE_SHM_H__
#define __TCE_SHM_H__

#ifdef	WIN32
//#include <windows.h>
#else
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#endif
#include "tce_utils.h"

//#include <iostream>
//using namespace std;

namespace tce{

class CShm
{
public:
	CShm(void){
		memset(m_szErrMsg, 0,sizeof(m_szErrMsg));
		m_bCreate = false;
		m_iShmKey = 0 ;
		m_nShmSize = 0;
		m_pShmBuf = NULL;
#ifdef	WIN32
		m_hShm = NULL;
#else
		m_iShmID = 0;
#endif

	}
	~CShm(void){
		if(!Detach())
		{
			printf("%s",m_szErrMsg);
		}
	}

	inline bool Init(const int32_t iKey, const size_t nSize);
	inline bool Create(const int32_t iKey, const size_t nSize, const bool bCreate =true ,const bool bReadOnly=false);
	void* GetShmBuf() const {	return m_pShmBuf;	}
	bool IsCreate() const {	return m_bCreate;	}
	int32_t GetShmKey() const {	return m_iShmKey;	}
	size_t GetShmSize() const {	return m_nShmSize;	}
	const char* GetErrMsg() const {	return m_szErrMsg;	}

	inline bool Detach();

	inline bool Attach(const bool bReadOnly/* = false*/){
#ifdef	WIN32
		m_pShmBuf = MapViewOfFile(m_hShm, FILE_MAP_ALL_ACCESS,0,0,iSize);
		if (NULL == m_pShmBuf)
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"MapViewOfFile Failed:%s",strerror(errno));
			return false;
		}
#else
		//m_bCreate = false;
		if ((m_pShmBuf = (shmat(m_iShmID, NULL ,bReadOnly ? SHM_RDONLY : 0 ))) == (char*)-1)
		{	
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"shmat error:%s", strerror(errno));
			return false;
		}		
		return true;
#endif	
	}
	
	int32_t GetShmID() const {	
#ifdef	WIN32
		return 0;
#else
		return m_iShmID;
#endif	
	}
	CShm(const CShm& rhs){
		m_bCreate = rhs.m_bCreate;
		m_iShmKey = rhs.m_iShmKey;
		m_nShmSize = rhs.m_nShmSize;
		m_pShmBuf = rhs.m_pShmBuf;
		memcpy(m_szErrMsg, rhs.m_szErrMsg, sizeof(m_szErrMsg));
#ifdef	WIN32
		m_hShm = rhs.m_hShm;
#else
		m_iShmID = rhs.m_iShmID;
#endif
	}
	CShm& operator=(const CShm& rhs){
		if ( this != &rhs )
		{
			m_bCreate = rhs.m_bCreate;
			m_iShmKey = rhs.m_iShmKey;
			m_nShmSize = rhs.m_nShmSize;
			m_pShmBuf = rhs.m_pShmBuf;
			memcpy(m_szErrMsg, rhs.m_szErrMsg, sizeof(m_szErrMsg));
#ifdef	WIN32
			m_hShm = rhs.m_hShm;
#else
			m_iShmID = rhs.m_iShmID;
#endif
		}

		return *this;
	}
private:
	bool GetShm();
	//bool Attach();
	bool Remove();
private:
	char m_szErrMsg[256];
	bool m_bCreate;

	int32_t m_iShmKey;
	size_t m_nShmSize;	//64λ
	void* m_pShmBuf;
#ifdef	WIN32
	HANDLE m_hShm;
#else
	int32_t m_iShmID;
#endif
};

bool CShm::Detach()
{
#ifdef WIN32
	if (NULL == m_hShm)
	{
		CloseHandle(m_hShm);
	}
#else
	if ( m_pShmBuf != NULL )
	{
		if (shmdt(m_pShmBuf) < 0)
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"shmdt error:%s", strerror(errno));
			return false;
		}
		else
		{
			m_pShmBuf = NULL;
		}		
	}


//	cout << "Detach m_iShmID=" << m_iShmID << endl;
#endif

	return true;
}


bool CShm::Init(const int32_t iKey, const size_t nSize)
{
	m_bCreate = false;
#ifdef WIN32
	char szKey[20];
	tce::xsnprintf(szKey,sizeof(szKey),"%d",iKey);

	m_hShm = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0, nSize,szKey);
	if (NULL == m_hShm) 
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"CreateFileMapping Failed:%s",strerror(errno));
		return false;
	}

	if(ERROR_ALREADY_EXISTS == GetLastError())
	{
		CloseHandle(m_hShm);
		m_hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,szKey);
		if(NULL == m_hShm)
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OpenFileMapping Failed:%s",strerror(errno));
			return false;
		}
	}
	else
	{
		m_bCreate = true;
	}
#else
	int32_t iFlag = 0666 | IPC_CREAT;

	if ((m_iShmID = shmget(iKey, nSize, 0666)) < 0)
	{
		if ((m_iShmID = shmget(iKey, nSize, iFlag)) < 0)
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"shmget error:%s", strerror(errno));
			return false;
		}
		m_bCreate = true;
	}

#endif
	m_iShmKey = iKey;
	m_nShmSize = nSize;
	return true;
}


bool CShm::Create(const int32_t iKey, const size_t nSize, const bool bCreate /* =true  */ ,const bool bReadOnly/* = false*/)
{
	m_bCreate = false;
#ifdef WIN32
	char szKey[20];
	tce::xsnprintf(szKey,sizeof(szKey),"%d",iKey);
	if (bCreate)
	{
		m_hShm = CreateFileMapping((HANDLE)INVALID_HANDLE_VALUE,NULL,PAGE_READWRITE,0,nSize,szKey);
		if (NULL == m_hShm) 
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"CreateFileMapping Failed:%s",strerror(errno));
			return false;
		}

		if(ERROR_ALREADY_EXISTS == GetLastError())
		{
			CloseHandle(m_hShm);
			m_hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,szKey);
			if(NULL == m_hShm)
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OpenFileMapping Failed:%s",strerror(errno));
				return false;
			}
		}
		else
		{
			m_bCreate = true;
		}
	}
	else
	{
		m_hShm = OpenFileMapping(FILE_MAP_ALL_ACCESS,FALSE,szKey);
		if(NULL == m_hShm)
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"OpenFileMapping Failed:%s",strerror(errno));
			return false;
		}
	}

	m_pShmBuf = MapViewOfFile(m_hShm, FILE_MAP_ALL_ACCESS,0,0,nSize);
	if (NULL == m_pShmBuf)
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"MapViewOfFile Failed:%s",strerror(errno));
		return false;
	}

#else
	int32_t iFlag = 0666;
	if (bCreate)
	{
		iFlag = 0666 | IPC_CREAT;
	}
	else
	{
		iFlag = 0666;
	}

	if ((m_iShmID = shmget(iKey, nSize, 0666)) < 0)
	{
		if ((m_iShmID = shmget(iKey, nSize, iFlag)) < 0)
		{
			tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"shmget error:%s", strerror(errno));
			return false;
		}

		m_bCreate = true;
	}

	if ((m_pShmBuf = (shmat(m_iShmID, NULL ,bReadOnly ? SHM_RDONLY : 0 ))) == (char*)-1)
	{	
		tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"shmat<m_iShmID=%d> error:%s", m_iShmID, strerror(errno));
		return false;
	}
#endif
	m_iShmKey = iKey;
	m_nShmSize = nSize;
	return true;
}


};

#endif


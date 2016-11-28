
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


#ifndef __TCE_LOCK_H__
#define __TCE_LOCK_H__

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif


namespace tce{

class CLock
{
public:

	CLock(void)
	{
	}

	virtual bool Lock() = 0;
	virtual bool Unlock() = 0;

	virtual ~CLock(void)
	{
	}
};

class CMutex : public CLock
{
public:
	inline CMutex(void){
#ifdef _WIN32
		::InitializeCriticalSection(&m_lock);
#else
		::pthread_mutex_init(&m_lock,NULL);
#endif
	}
	inline ~CMutex(void){
#ifdef _WIN32
		::DeleteCriticalSection(&m_lock);
#else
		::pthread_mutex_destroy(&m_lock);
#endif
	}

	inline virtual bool Lock(){
#ifdef _WIN32
		::EnterCriticalSection(&m_lock);
#else
		::pthread_mutex_lock(&m_lock);
#endif
		return true;
	};

	inline virtual bool Unlock(){
#ifdef _WIN32
		::LeaveCriticalSection(&m_lock);
#else
		::pthread_mutex_unlock(&m_lock);
#endif
		return true;
	}
private:
	CMutex(const CMutex& rhs);
	CMutex& operator=(const CMutex& rhs);
private:
	char m_szErrMsg[1024];

#ifdef _WIN32
	CRITICAL_SECTION m_lock;
#else
	pthread_mutex_t  m_lock;
#endif

};

class CAutoLock
{
public:

	CAutoLock(CLock& lock,const bool bUseLock=true)
		:m_lock(lock)
		,m_bUse(bUseLock)
	{
		if ( m_bUse )
		{
			m_lock.Lock();
		}
	}

	virtual ~CAutoLock(void)
	{
		if ( m_bUse )
		{
			m_lock.Unlock();
		}
	}
private:
	CLock& m_lock;
	bool m_bUse;
};

template < typename ReadWriteLockerT >
class ReadWriteLockerRL
{
	ReadWriteLockerT& m_cs;

	ReadWriteLockerRL(const ReadWriteLockerRL&);
	ReadWriteLockerRL& operator=(const ReadWriteLockerRL&);
public:
	ReadWriteLockerRL(ReadWriteLockerT& cs)
		: m_cs(cs)
	{
		m_cs.ReadLock();
	}

	~ReadWriteLockerRL()
	{
		m_cs.Unlock();
	}
};

template < typename ReadWriteLockerT >
class ReadWriteLockerWL
{
	ReadWriteLockerT& m_cs;

	ReadWriteLockerWL(const ReadWriteLockerWL&);
	ReadWriteLockerWL& operator=(const ReadWriteLockerWL&);
public:
	ReadWriteLockerWL(ReadWriteLockerT& cs)
		: m_cs(cs)
	{
		m_cs.WriteLock();
	}

	~ReadWriteLockerWL()
	{
		m_cs.Unlock();
	}
};

class ReadWriteLocker
{
private:
	pthread_rwlock_t m_sect;

	ReadWriteLocker(const ReadWriteLocker&);
	ReadWriteLocker& operator=(const ReadWriteLocker&);
public:
	ReadWriteLocker()			{ ::pthread_rwlock_init(&m_sect, NULL); }
	~ReadWriteLocker()			{ ::pthread_rwlock_destroy(&m_sect); }
	bool ReadLock()				{ return 0 == ::pthread_rwlock_rdlock(&m_sect); }
	bool WriteLock()			{ return 0 == ::pthread_rwlock_wrlock(&m_sect); }
	bool TryReadLock()			{ return 0 == ::pthread_rwlock_tryrdlock(&m_sect); }
	bool TryWriteLock()			{ return 0 == ::pthread_rwlock_trywrlock(&m_sect); }
	bool Unlock()				{ return 0 == ::pthread_rwlock_unlock(&m_sect); }
#ifdef _POSIX_TIMEOUTS
#	if _POSIX_TIMEOUTS >= 0
	bool ReadLock(const struct timespec& abstime)	{ return 0 == ::pthread_rwlock_timedrdlock(&m_sect, &abstime); }
	bool WriteLock(const struct timespec& abstime)	{ return 0 == ::pthread_rwlock_timedwrlock(&m_sect, &abstime); }
#	endif
#endif
	/*
extern int pthread_rwlockattr_init (pthread_rwlockattr_t *__attr) __THROW;
extern int pthread_rwlockattr_destroy (pthread_rwlockattr_t *__attr) __THROW;
extern int pthread_rwlockattr_getpshared (__const pthread_rwlockattr_t *
					  __restrict __attr,
					  int *__restrict __pshared) __THROW;
extern int pthread_rwlockattr_setpshared (pthread_rwlockattr_t *__attr,
					  int __pshared) __THROW;
extern int pthread_rwlockattr_getkind_np (__const pthread_rwlockattr_t *__attr,
					  int *__pref) __THROW;
extern int pthread_rwlockattr_setkind_np (pthread_rwlockattr_t *__attr,
					  int __pref) __THROW;
	*/
};

typedef ReadWriteLockerRL<ReadWriteLocker>	ReadLocker;
typedef ReadWriteLockerWL<ReadWriteLocker>	WriteLocker;



};

#endif



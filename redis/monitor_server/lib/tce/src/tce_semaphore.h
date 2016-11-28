
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


#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#ifdef WIN32
#else
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#endif

namespace tce{

class CSemaphore
{
public:
	CSemaphore(void){
		m_bCreate = false;
		m_iSemId = -1;
	}

	~CSemaphore(void){}

	bool Create(const int32_t iSemKey,const int32_t iSems=1){
#if defined(__GNU_LIBRARY__) && !defined(_SEM_SEMUN_UNDEFINED)
		/* union semun is defined by including <sys/sem.h> */
#else
		/* according to X/OPEN we have to define it ourselves */
		union semun {
			int32_t val;                  /* value for SETVAL */
			struct semid_ds *buf;     /* buffer for IPC_STAT, IPC_SET */
			unsigned short *array;    /* array for GETALL, SETALL */
			/* Linux specific part: */
			struct seminfo *__buf;    /* buffer for IPC_INFO */
		};
#endif

#ifdef WIN32
#else
		if ((m_iSemId = semget(iSemKey,iSems,00666))<0)
		{
			if ((m_iSemId = semget(iSemKey,iSems,IPC_CREAT | IPC_EXCL | 00666))<0)
			{
				switch(errno)
				{
				case EACCES:
					sprintf(m_szErrMsg,"A semaphore set exists for key, but the calling process does not have permission to access the  set.\n");
					break;
				case EEXIST:
					sprintf(m_szErrMsg,"A semaphore set exists for key and semflg was asserting both IPC_CREAT and IPC_EXCL.\n");
					break;
				case ENOENT:
					sprintf(m_szErrMsg,"No semaphore set exists for key and semflg wasn't asserting IPC_CREAT.");
					break;
				case EINVAL:
					sprintf(m_szErrMsg,"nsems  is  less  than  0  or  greater  than  the limit on the number of semaphores per semaphore set(SEMMSL), or a semaphore set corresponding to key already exists, and nsems is larger than the  num	ber of semaphores in that set.\n");
					break;
				case ENOMEM:
					sprintf(m_szErrMsg,"A semaphore set has to be created but the system has not enough memory for the new data structure.\n");
					break;
				case ENOSPC:
					sprintf(m_szErrMsg,"A  semaphore  set  has  to  be created but the system limit for the maximum number of semaphore sets(SEMMNI), or the system wide maximum number of semaphores (SEMMNS), would be exceeded.\n");
					break;
				default:
					sprintf(m_szErrMsg,"unkown error.\n");
					break;
				}

				return false;
			}

			m_bCreate = true;
			if (m_bCreate)
			{
				union semun arg;
				arg.val=1;
				if(semctl(m_iSemId,0,SETVAL,arg)==-1) 
				{
					sprintf(m_szErrMsg,"semctl setval error");
					return false;
				}
			}

		}
#endif


		return true;
	}
	bool Lock(const bool bWait=true){
#ifdef WIN32
#else

		struct sembuf stSemBuf[2] = {{0, -1, SEM_UNDO},{0, -1,IPC_NOWAIT|SEM_UNDO}};

		if (m_iSemId==-1)
		{
			sprintf(m_szErrMsg,"no create sem.");
			return false;
		}


		if ( semop(m_iSemId, &stSemBuf[bWait?0:1], 1) < 0)
		{
			sprintf(m_szErrMsg,"semop error:(errno=%d)",errno);
			return false;
		}
#endif
		return true;	
	}

	bool Unlock(){
#ifdef WIN32
#else
		struct sembuf stSemBuf[1] = {{0, 1, SEM_UNDO}};

		if (m_iSemId==-1)
		{
			sprintf(m_szErrMsg,"no create sem.");
			return false;
		}


		if ( semop(m_iSemId, &stSemBuf[0], 1) < 0)
		{
			sprintf(m_szErrMsg,"semop error:(errno=%d)",errno);
			return false;
		}
#endif

		return true;	
	}

	bool Delete(){
#ifdef WIN32
#else
		if(semctl(m_iSemId, 0, IPC_RMID)==-1)
		{
			sprintf(m_szErrMsg,"semctl IPC_RMID error:(errno=%d)",errno);
			return false;
		}
#endif

		return true;
	}


	const char* GetErrMsg() const { return m_szErrMsg; }
private:
	char m_szErrMsg[1024];

	int32_t m_iSemId;

	bool m_bCreate;	//是否创建信号量(true:是; false:否)
};

};

#endif


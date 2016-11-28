
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


#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include "shm_adapter.h"

/*
 * 默认构造函数
 */
shm_adapter::shm_adapter()
{
	m_bIsNewShm = false;
	m_ddwShmKey = 0;
	m_ddwShmSize = 0;
	m_bIsValid = false;
	m_pShm = NULL;
	m_nShmId = 0;
}

/*
 * 构造函数
 */
shm_adapter::shm_adapter(uint64_t ddwShmKey, uint64_t ddwShmSize, bool bCreate, bool bReadOnly, bool bLock, bool bHugePage, bool bInitClear)
{
	int iRet;
	m_bIsNewShm = false;
	m_ddwShmKey = ddwShmKey;
	m_ddwShmSize = ddwShmSize;
	m_bIsValid = false;
	m_pShm = NULL;
	int nFlag = 0666;	//默认标志
	if(bCreate)
	{
		//增加创建标志
		nFlag |= IPC_CREAT;
	}
	else if(bReadOnly)
	{
		//非创建, 并且只读, 去除写标志
		nFlag &= 077777444; 
	}
	if(bHugePage)
	{
		//增加大页标志
		nFlag |= SHM_HUGETLB;
	}
	//获取ShmId
	int iShmId = shmget(ddwShmKey, ddwShmSize, nFlag & (~IPC_CREAT));
	if(iShmId < 0)
	{
		//连接失败, 并且是非创建的, 返回失败
		if(! (nFlag & IPC_CREAT))
		{
			m_strErrMsg = "shmget without create failed - "; m_strErrMsg += strerror(errno);
			return;
		}
		//连接失败, 创建该共享内存
		m_bIsNewShm = true;
		iShmId = shmget(ddwShmKey, ddwShmSize , nFlag);
		if(iShmId < 0)
		{
			m_strErrMsg = "shmget with create failed - "; m_strErrMsg += strerror(errno);
			return;
		}
		//Attach
		m_pShm = shmat(iShmId, NULL, 0);
		if(m_pShm == (void *)-1)
		{
			m_strErrMsg = "shmat with create failed - "; m_strErrMsg += strerror(errno);
			m_pShm = NULL;
			return;
		}
		//创建成功后初始化为0
		if(bInitClear)
		{
			memset(m_pShm, 0, ddwShmSize);
		}
	}
	else 
	{
		//共享内存存在, Attach
		m_pShm = shmat(iShmId, NULL, 0);
		if(m_pShm == (void *)-1)
		{
			m_strErrMsg = "shmat with exist shm failed - "; m_strErrMsg += strerror(errno);
			m_pShm = NULL;
			return;
		}
	}

	m_nShmId = iShmId;
	
	//锁住内存
	if(bLock)
	{
		iRet = mlock(m_pShm, m_ddwShmSize);
		if(iRet != 0)
		{
			m_strErrMsg = "mlock failed - "; m_strErrMsg += strerror(errno);
			//操作失败, dettach内存
			shmdt(m_pShm);
			m_pShm = NULL;
			return;
		}
	}
	m_bIsValid = true;
}

/*
 * 析构函数
 */
shm_adapter::~shm_adapter()
{
	if(m_pShm)
	{
		shmdt(m_pShm);
	}
}

/*
 * 刷新共享内存
 */
bool shm_adapter::refresh()
{
	if(! m_pShm)
	{
		m_strErrMsg = "shm is not init yet";
		return false;
	}

	struct shmid_ds stShmStat;
	//重新连接共享内存
	int nShmId = shmget(m_ddwShmKey, 0, 0); 
	if(nShmId < 0)
	{
		m_strErrMsg = "shmget failed - "; m_strErrMsg += strerror(errno);
		return false;
	}
	//ShmId一样, 共享内存没有发生变化
	if(nShmId == m_nShmId)
	{
		return true;
	}
	//获取共享内存大小
	int iRet = shmctl(nShmId, IPC_STAT, &stShmStat);
	if(iRet < 0)
	{
		m_strErrMsg = "shmctl failed - "; m_strErrMsg += strerror(errno);
		return false;
	}
	//大小一样, 没有发生变化
	if(m_ddwShmSize == stShmStat.shm_segsz)
	{
		return true;
	}

	//重新Attach共享内存
	void *pShm = shmat(nShmId, NULL, 0);
	if(pShm == (void *)-1)
	{
		m_strErrMsg = "shmat failed - "; m_strErrMsg += strerror(errno);
		return false;
	}

	//释放旧的共享内存连接
	shmdt(m_pShm);

	m_pShm = pShm;
	m_nShmId = nShmId;
	m_ddwShmSize = stShmStat.shm_segsz;

	return true;
}

/*
 * 打开共享内存
 */
bool shm_adapter::open(uint64_t ddwShmKey, bool bReadOnly)
{
	if(m_pShm)
	{
		m_strErrMsg = "Shm is NOT null, you have opened one already";
		return false;
	}
	struct shmid_ds stShmStat;
	//连接共享内存
	int nShmId = shmget(ddwShmKey, 0, 0);
	if(nShmId < 0)
	{
		m_strErrMsg = "shmget failed - "; m_strErrMsg += strerror(errno);
		return false;
	}
	//获取共享内存大小
	int iRet = shmctl(nShmId, IPC_STAT, &stShmStat);
	if(iRet < 0)
	{
		m_strErrMsg = "shmctl to get size failed - "; m_strErrMsg += strerror(errno);
		return false;
	}
	//Attach共享内存
	if(bReadOnly)
	{
		m_pShm = shmat(nShmId, NULL, SHM_RDONLY);
	}
	else
	{
		m_pShm = shmat(nShmId, NULL, 0);
	}
	if(m_pShm == (void *)-1)
	{
		m_strErrMsg = "shmat with exist shm failed - "; m_strErrMsg += strerror(errno);
		m_pShm = NULL;
		return false;
	}

	m_nShmId = nShmId;
	m_ddwShmSize = stShmStat.shm_segsz;
	m_ddwShmKey = ddwShmKey;
	
	m_bIsValid = true;
	return true;
}

/*
 * 创建共享内存
 */
bool shm_adapter::create(uint64_t ddwShmKey, uint64_t ddwShmSize, bool bHugePage, bool bInitClear)
{
	if(m_pShm)
	{
		m_strErrMsg = "Shm is NOT null, you have opened one already";
		return false;
	}
	//设置创建标志
	int nFlag = 0666 | IPC_CREAT;
	if(bHugePage)
	{
		nFlag |= SHM_HUGETLB;
	}
	//尝试连接共享内存
	int nShmId = shmget(ddwShmKey, 0, 0);
	if(nShmId < 0)
	{
		//连接失败, 创建共享内存
		nShmId = shmget(ddwShmKey, ddwShmSize, nFlag);
		if(nShmId < 0)
		{
			m_strErrMsg = "shmget with create failed - "; m_strErrMsg += strerror(errno);
			return false;
		}
		//Attach共享内存
		m_pShm = shmat(nShmId, NULL, 0);
		if(m_pShm == (void *)-1)
		{
			m_strErrMsg = "shmat failed - "; m_strErrMsg += strerror(errno);
			m_pShm = NULL;
			return false;
		}
		if(bInitClear)
		{
			memset(m_pShm, 0, ddwShmSize);
		}
		m_bIsNewShm = true;
	}
	else
	{
		//共享内存已经存在, 判断大小是否一致
		struct shmid_ds stShmStat;
		int iRet = shmctl(nShmId, IPC_STAT, &stShmStat);
		if(iRet < 0)
		{
			m_strErrMsg = "shmctl with size failed - "; m_strErrMsg += strerror(errno);
			return false;
		}
		//大小不一致
		if(stShmStat.shm_segsz != ddwShmSize)
		{
			m_strErrMsg = "shm exist, but size is not the same";
			return false;
		}
		//Attach共享内存
		m_pShm = shmat(nShmId, NULL, 0);
		if(m_pShm == (void *)-1)
		{
			m_strErrMsg = "shmat failed - "; m_strErrMsg += strerror(errno);
			m_pShm = NULL;
			return false;
		}
		m_bIsNewShm = false;
	}
	m_nShmId = nShmId;
	m_ddwShmKey = ddwShmKey;
	m_ddwShmSize = ddwShmSize;
	m_bIsValid = true;
	return true;
}

/*
 * 关闭共享内存
 */
bool shm_adapter::close()
{
	if(m_pShm)
	{
		//Dettach共享内存
		int iRet = shmdt(m_pShm);
		if(iRet < 0)
		{
			m_strErrMsg = "shmdt shm failed - "; m_strErrMsg += strerror(errno);
			return false;
		}
		m_pShm = NULL;
	}
	m_bIsValid = false;
	return true;
}

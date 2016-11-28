
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


#pragma once

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/shm.h>

//共享内存类
class shm_adapter
{
	public:
		/*
		 * 默认构造函数
		 * 需要使用create/open函数创建或打开共享内存	
		 */
		shm_adapter();

		/*
		 * 构造函数
		 * 需要使用is_valid函数判断是否成功
		 * 注: bLock锁住内存, 只有root用户可以使用
		 */
		shm_adapter(uint64_t ddwShmKey, uint64_t ddwShmSize, bool bCreate = true, bool bReadOnly = false, bool bLock = true, bool bHugePage = false, bool bInitClear = true);

		/*
		 * 析构函数
		 */
		~shm_adapter();

		/*
		 * 判断初始化是否成功
		 */
		bool is_valid() { return m_bIsValid; }

		/*
		 * 获取创建/连接的共享内存
		 */
		void *get_shm() { return m_pShm; }

		/*
		 * 刷新共享内存
		 * 注:	调用该函数会使用shmdt, 反复调用会影响性能
		 *		如果共享内存不会在程序运行期间被另外的程序重新创建/修改大小, 则不需要调用该函数
		 */
		bool refresh();

		/*
		 * 取出失败信息
		 */
		const std::string &get_err_msg() { return m_strErrMsg; }

		/*
		 * 打开存在的共享内存
		 */
		bool open(uint64_t ddwShmKey, bool bReadOnly = false);

		/*
		 * 只读打开存在的共享内存
		 */
		bool open_readonly(uint64_t ddwShmKey) { return open(ddwShmKey, true); }

		/*
		 * 创建共享内存
		 */
		bool create(uint64_t ddwShmKey, uint64_t ddwShmSize, bool bHugePage = false, bool bInitClear = true);

		/*
		 * 使用大页创建共享内存
		 */
		bool create_hugepage(uint64_t ddwShmKey, uint64_t ddwShmSize, bool bInitClear = true) { return create(ddwShmKey, ddwShmSize, true, bInitClear); }

		/*
		 * 获取共享内存的大小
		 */
		uint64_t get_size() { return m_ddwShmSize; }
		
		/*
		 * 获取共享内存的Key
		 */
		uint64_t get_key() { return m_ddwShmKey; }

		/*
		 * 判断是否为新的共享内存
		 */
		bool is_new() { return m_bIsNewShm; }

		/*
		 * 关闭共享内存
		 */
		bool close();

	private:

	public:

	private:

		bool m_bIsNewShm;		//是否是新创建的共享内存

		uint64_t m_ddwShmKey;	//共享内存的Key
		uint64_t m_ddwShmSize;	//共享内存的大小
		int m_nShmId;			//共享内存的ID

		bool m_bIsValid;		//共享内存是否可用

		void *m_pShm;			//共享内存的指针

		std::string m_strErrMsg;	//错误信息
};

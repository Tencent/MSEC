
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


#ifndef __TCE_SHM_ARRAY_H__
#define __TCE_SHM_ARRAY_H__

#include "tce_def.h"
#include "tce_shm.h"
#include <stdexcept>
#include <assert.h>


namespace tce{



namespace shm{

	template <class _Tp, class _Head=SEmptyHead>
	class CArray
	{
		typedef _Head head_type;
		typedef _Tp value_type;
		typedef size_t size_type;
		typedef value_type node_type;
	public:
		CArray()
			:m_bInit(false),
			m_pHead(NULL)
			,m_pNodes(NULL)
		{

		}
		~CArray(){

		}
		bool init(const int iShmKey, const size_t nArraySize, const bool bReadOnly=false);
		inline value_type& operator[](const size_t nIndex);
		inline const value_type& operator[](const size_t nIndex) const;
		size_t size() const	{	return m_nArraySize;	}
		const char* err_msg() const {	return m_szErrMsg;	}
		head_type& head(){	return *m_pHead;	}
		const head_type& head() const {	return *m_pHead;	}
	private:
		bool m_bInit;
		head_type* m_pHead;
		node_type* m_pNodes;
		size_type m_nArraySize;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
	};

	template <class _Tp, class _Head>
	typename CArray<_Tp,_Head>::value_type& CArray<_Tp,_Head>::operator[](const size_t nIndex)
	{
		assert( nIndex < m_nArraySize );
		if ( nIndex >= m_nArraySize )
			throw std::runtime_error("operator[] index error");
		return *(m_pNodes+nIndex);
	}

	template <class _Tp, class _Head>
	const typename CArray<_Tp,_Head>::value_type& CArray<_Tp,_Head>::operator[](const size_t nIndex) const
	{
		assert(nIndex < m_nArraySize);
		if ( nIndex >= m_nArraySize )
			throw std::runtime_error("operator[] index error");
		
		return *(m_pNodes+nIndex);
	}


	template <class _Tp, class _Head>
	bool CArray<_Tp,_Head>::init(const int32_t iShmKey, const size_t nArraySize, const bool bReadOnly /*=false*/)
	{
		if ( m_bInit )
			return true;

		m_nArraySize = nArraySize;
		size_type nShmMaxSize = nArraySize*sizeof(value_type)+sizeof(_Head);
		if ( !m_oShm.Init( iShmKey, nShmMaxSize ) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Array Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}

		//根据是否只读方式绑定共享内存
		if ( !m_oShm.Attach(bReadOnly) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"Array Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}

		m_pHead = (head_type*)reinterpret_cast<char*>(m_oShm.GetShmBuf());
		m_pNodes  = (node_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
		
		m_bInit = true;
		return true;
	}


};

};


#endif


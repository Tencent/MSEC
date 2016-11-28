
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


#ifndef __TCE_SHM_ARRAY_VAR_H__
#define __TCE_SHM_ARRAY_VAR_H__

#include "tce_utils.h"
#include "tce_shm.h"
#include "tce_def.h"
#include <stdexcept>
#include <assert.h>
using namespace std;

namespace tce{

namespace shm{

	template <class _Head=SEmptyHead>
	class CArrayVar
	{
	public:
		typedef size_t size_type;
	private:
		typedef _Head user_head_type;
#pragma pack(1)

		struct SArrayHead{
			size_type nArraySize;
			size_type nMaxCacheDataSize;
			size_type nItemDataSize;
			size_type nTotalItemCount;	
			size_type nEmptyItemCount;
			size_type nEmptyHead;
			bool bCRC32Check;
			char szReserve[999];
		};


		struct SArrayNode{
			size_type nCRC32;
			size_type nDataSize;
			size_type nDataPtr;
		};
		struct SItemDataHead{
			size_type nNextPtr;
		};
#pragma pack()

		typedef SArrayNode node_type;
		typedef SItemDataHead idh_type;
		typedef SArrayHead head_type;

	public:
		CArrayVar()
			:m_bInit(false)
			,m_pUserDefineHead(NULL)
			,m_pHead(NULL)
			,m_pNodes(NULL)
			,m_pDatas(NULL)
		{

		}
		~CArrayVar(){

		}
		bool init(const int iShmKey, 
			const size_type nArraySize, 
			const size_type nMaxCacheDataSize, 
			const size_type nItemDataSize,
			const bool bCRC32Check=false,
			const bool bReadOnly=false);

		inline bool set(const size_type nIndex, const char* pData, const size_type nSize);
		inline bool set(const size_type nIndex, const std::string& sData){
			return this->set(nIndex, sData.data(), sData.size());
		}

		inline std::string get(const size_type dwIndex);
		inline bool get(const size_type dwIndex, char* pBuf, size_type& nBufSize);
	
		size_type total_item_count() {	return m_pHead->nTotalItemCount;	}
		size_type empty_item_count() {	return m_pHead->nEmptyItemCount;	}
		size_type empty_real_item_count() {	
			if ( 0 == m_pHead->nEmptyHead )
				return 0;

			size_type nCur = m_pHead->nEmptyHead;
			size_type nCount = 1;
			while(_get_next_ptr(nCur))
			{
				++nCount;
				nCur = _get_next_ptr(nCur);
			}		
			return nCount;	
		}

		size_type size() const	{	return m_pHead->m_nArraySize;	}
		const char* err_msg() const {	return m_szErrMsg;	}
		user_head_type& head(){	return *m_pUserDefineHead;	}
		const user_head_type& head() const {	return *m_pUserDefineHead;	}

	private:
		inline size_type _new_and_copy(const char* pData, const size_type nSize);
		inline bool _delete(const size_type nDeletePtr);
		inline size_type _get_next_ptr(const size_type nPtr){
			assert(0!=nPtr);
			return _get_data_head(nPtr)->nNextPtr;
		}
		inline idh_type* _get_data_head(const size_type nPtr){
			assert(0!=nPtr);
			return  (idh_type*)(m_pDatas+((nPtr-1)*m_pHead->nItemDataSize));
		}
		inline char* _get_data(const size_type nPtr){
			assert(0!=nPtr);
			return  m_pDatas+((nPtr-1)*m_pHead->nItemDataSize) + sizeof(idh_type);
		}

	private:
		bool m_bInit;
		user_head_type* m_pUserDefineHead;
		head_type* m_pHead;
		node_type* m_pNodes;
		char* m_pDatas;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
	};

	template<class _Head>
	bool CArrayVar<_Head>::_delete(const size_type nDeletePtr)
	{
		if ( nDeletePtr == 0 )
			return false;

		size_type nCur = nDeletePtr;
		size_type nCount = 1;
		while(_get_next_ptr(nCur))
		{
			++nCount;
//			cout << "delete nCur=" << nCur << endl;
			nCur = _get_next_ptr(nCur);
		}
		
		_get_data_head(nCur)->nNextPtr = m_pHead->nEmptyHead;
		m_pHead->nEmptyHead = nDeletePtr;
		m_pHead->nEmptyItemCount += nCount;

		return true;
	}

	//return:>0 item data index; 0:error.
	template<class _Head>
	typename CArrayVar<_Head>::size_type CArrayVar<_Head>::_new_and_copy(const char* pData, const size_type nSize)
	{
		size_type nNeedItemCount = (nSize+m_pHead->nItemDataSize-sizeof(idh_type)-1)/(m_pHead->nItemDataSize-sizeof(idh_type));

		if ( m_pHead->nEmptyItemCount < nNeedItemCount )
		{//没有足够的空间
			return 0;
		}
		
		size_type nCopySize = 0;
		size_type nBeginItem = m_pHead->nEmptyHead;
		size_type nEndItem = 0;
		size_type nCurItem = nBeginItem;
//		cout << "nNeedItemCount=" << nNeedItemCount << endl;
		for ( size_type i=0; i<nNeedItemCount; ++i )
		{
			assert(0 != nCurItem);
			if ( 0 != nCurItem )
			{
				char* pBuf = _get_data(nCurItem);
//				cout << "i=" << i << "; nCurItem=" << nCurItem << "; pBuf="<< (int)pBuf <<  endl;
				if ( nSize - nCopySize > m_pHead->nItemDataSize-sizeof(idh_type) )
				{	
					memcpy(pBuf, pData+nCopySize, m_pHead->nItemDataSize-sizeof(idh_type));
					nCurItem = _get_data_head(nCurItem)->nNextPtr;
					nCopySize += m_pHead->nItemDataSize-sizeof(idh_type);
				}
				else
				{
					assert(i == nNeedItemCount-1);
					if ( i != nNeedItemCount-1 )//分配错误。。
						return 0;

					memcpy(pBuf, pData+nCopySize, nSize - nCopySize);
					nEndItem = _get_data_head(nCurItem)->nNextPtr;

					m_pHead->nEmptyHead = nEndItem;
					_get_data_head(nCurItem)->nNextPtr = 0;

					nCopySize = nSize;
					
					assert(m_pHead->nEmptyItemCount>=nNeedItemCount);
					m_pHead->nEmptyItemCount -= nNeedItemCount;
				}
	
			}
			else
			{
				//没有足够的空间
				return 0;
			}
		}

		return nBeginItem;
	}


	template<class _Head>
	std::string CArrayVar<_Head>::get(const size_type nIndex)
	{
		std::string sData;
		assert(nIndex < m_pHead->nArraySize);
		if ( nIndex >= m_pHead->nArraySize )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: index<%lu> over arraysize<%lu>", nIndex,m_pHead->nArraySize);
			return sData;
		}

		node_type* pNode = m_pNodes+nIndex;
		size_type nCur = pNode->nDataPtr;
		size_type nNeedSize = pNode->nDataSize;
		while( 0 != nCur && 0 != nNeedSize )
		{
			char* pData = _get_data(nCur);
			if ( nNeedSize >= m_pHead->nItemDataSize-sizeof(idh_type) )
			{
				sData.append(pData, m_pHead->nItemDataSize-sizeof(idh_type));
				nNeedSize -= m_pHead->nItemDataSize-sizeof(idh_type);
				nCur = _get_data_head(nCur)->nNextPtr;
			}
			else
			{
				sData.append(pData, nNeedSize);
				nNeedSize = 0;
			}
//			cout << "nCur=" << nCur << "; pData=" << (int)pData <<  endl;
		}

		if ( m_pHead->bCRC32Check )
		{
			if ( pNode->nCRC32 != tce::CRC32(sData.data(), sData.size()) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: crc32 check error.");
				assert(pNode->dwCRC32 == tce::CRC32(sData.data(), sData.size()));
				return "";
			}
		}

		return sData;
	}

	template<class _Head>
	bool CArrayVar<_Head>::get(const size_type nIndex, char* pBuf, size_type& dwBufSize)
	{
		std::string sData;
		assert(nIndex < m_pHead->m_nArraySize);
		if ( nIndex >= m_pHead->nArraySize )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: index<%lu> over arraysize<%lu>", nIndex,m_pHead->nArraySize);
			return false;
		}

		node_type* pNode = m_pNodes+nIndex;

		if ( pNode->nDataSize > dwBufSize )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: datasize<%lu> > dwBufSize<%lu>", pNode->nDataSize, dwBufSize);
			return false;
		}

		size_type nNeedSize = pNode->nDataSize;
		size_type nCur = pNode->nDataPtr;
		while( 0 != nCur && 0 != nNeedSize )
		{
			char* pData = _get_data(nCur);
			if ( nNeedSize >= m_pHead->nItemDataSize-sizeof(idh_type) )
			{
				memcpy(pBuf+pNode->nDataSize-nNeedSize, pData, m_pHead->nItemDataSize-sizeof(idh_type));
				nNeedSize -= m_pHead->nItemDataSize-sizeof(idh_type);
				nCur = _get_data_head(nCur)->nNextPtr;
			}
			else
			{
				memcpy(pBuf+pNode->nDataSize-nNeedSize, pData, nNeedSize);
				nNeedSize = 0;
			}
		}

		if ( m_pHead->bCRC32Check )
		{
			if ( pNode->nCRC32 != tce::CRC32(sData.data(), sData.size()) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: crc32 check error.");
				assert(pNode->dwCRC32 == tce::CRC32(sData.data(), sData.size()));
				return false;
			}
		}


		return true;
	}


 

	template<class _Head>
	bool CArrayVar<_Head>::set(const size_type nIndex, const char* pData, const size_type nSize)
	{
		assert(nIndex < m_pHead->m_nArraySize);
		if ( nIndex >= m_pHead->nArraySize )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: index<%lu> over arraysize<%lu>", nIndex,m_pHead->nArraySize);
			return false;
		}

		size_type nDataPtr = _new_and_copy(pData, nSize);
		if ( nDataPtr != 0 )
		{
			node_type* pNode = m_pNodes+nIndex;
			size_type nTmpPtr = pNode->nDataPtr;
			pNode->nDataPtr = nDataPtr;
			pNode->nDataSize = nSize;
			if ( m_pHead->bCRC32Check )
				pNode->nCRC32 = tce::CRC32(pData, nSize);
			_delete(nTmpPtr);
		}
		else
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar set error: no enough memory(emptysize=%lu,datasize=%lu).", m_pHead->nEmptyItemCount*(m_pHead->nItemDataSize-sizeof(idh_type)), nSize);
			return false;
		}
		
		return true;
	}

	
	template<class _Head>
	bool CArrayVar<_Head>::init(const int iShmKey, const size_t nArraySize, const size_type nMaxCacheDataSize, const size_type nItemDataSize, const bool bCRC32Check/*=false*/, const bool bReadOnly /*=false*/)
	{
		if ( m_bInit )
			return true;

		size_type nShmMaxSize =  sizeof(user_head_type) + sizeof(head_type) + sizeof(node_type)*nArraySize + nMaxCacheDataSize;

		//初始化共享内存
		if ( !m_oShm.Init( iShmKey, nShmMaxSize ) )
		{
			xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:%s", m_oShm.GetErrMsg());
			return false;
		}

		//是否是第一次创建共享内存
		if ( m_oShm.IsCreate() )
		{
			if ( !m_oShm.Attach(false) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}

			m_pUserDefineHead = (user_head_type*)reinterpret_cast<char*>(m_oShm.GetShmBuf());
			m_pHead = (head_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type));
			m_pNodes = (node_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type)+sizeof(head_type));
			m_pDatas = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type)+sizeof(head_type)+sizeof(node_type)*nArraySize);

			m_pHead->nArraySize = nArraySize;
			m_pHead->nMaxCacheDataSize = nMaxCacheDataSize;
			m_pHead->nItemDataSize = nItemDataSize;
			m_pHead->nTotalItemCount = nMaxCacheDataSize/nItemDataSize;
			m_pHead->nEmptyItemCount = m_pHead->nTotalItemCount;
			m_pHead->bCRC32Check = bCRC32Check;
			for ( size_type i=1; i<=m_pHead->nTotalItemCount; ++i )
			{
				if ( 1 == i )
				{
					m_pHead->nEmptyHead = 1;
					_get_data_head(i)->nNextPtr = 0;
				}
				else
				{
					_get_data_head(i-1)->nNextPtr = i;
					_get_data_head(i)->nNextPtr = 0;
				}
			}

			//根据是否只读方式重新绑定共享内存
			if ( !m_oShm.Detach() )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}
			if ( !m_oShm.Attach(bReadOnly) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}
		}
		else
		{
			//根据是否只读方式绑定共享内存
			if ( !m_oShm.Attach(bReadOnly) )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:%s", m_oShm.GetErrMsg());
				return false;
			}

			m_pUserDefineHead = (user_head_type*)reinterpret_cast<char*>(m_oShm.GetShmBuf());
			m_pHead = (head_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type));
			m_pNodes = (node_type*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type)+sizeof(head_type));
			m_pDatas = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(user_head_type)+sizeof(head_type)+sizeof(node_type)*nArraySize);

			if ( nArraySize != m_pHead->nArraySize
				|| nMaxCacheDataSize != m_pHead->nMaxCacheDataSize
				|| nItemDataSize != m_pHead->nItemDataSize
				|| nMaxCacheDataSize/nItemDataSize != m_pHead->nTotalItemCount 
				|| bCRC32Check != m_pHead->bCRC32Check )
			{
				xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"ArrayByVar Init Failed:head data error: no equal.");
				return false;
			}

		}
	
		m_bInit = true;
		return true;
	}


};

};


#endif



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


#ifndef __TCE_SESSION_MANAGE_H__
#define __TCE_SESSION_MANAGE_H__

#include "tce_singleton.h"
#include "tce_socket_api.h"
#include "tce_socket_session.h"

namespace tce{

	template<typename T>
	class  CSessionManage:private CNonCopyAble
	{
		typedef T ITEM_TYPE;
		enum{
			MAX_SOCKET = 500000,
		};

	public:
		enum {NEED_LOCK=true};

		CSessionManage()
			:m_nMaxClient(1024)
			,m_nClientSize(0)
			,m_nCur(0)
			,m_poBufferMalloc(NULL)
			,m_pIdles(NULL)
			,m_iMaxFd(-1)
		{
			memset(m_arrSockets, 0, sizeof(ITEM_TYPE*)*MAX_SOCKET);
		}
		~CSessionManage(){}

		bool Init(const size_t nMaxInDataSize, const size_t nMaxOutDataSize,const size_t nMaxClient, const size_t nTotalMallocSize, const size_t nMallocItemSize){
			m_nMaxClient = nMaxClient;

			m_poBufferMalloc = new CBufferMalloc;
			if ( NULL == m_poBufferMalloc || !m_poBufferMalloc->Init(nTotalMallocSize, nMallocItemSize) )
			{
				return false;
			}

			char* pTmp = new char[sizeof(ITEM_TYPE*)*m_nMaxClient];
			m_pIdles =  (ITEM_TYPE**)pTmp;
			for ( size_t i=0; i<m_nMaxClient; ++i )
			{
				m_pIdles[i] = new CSocketSession(m_poBufferMalloc,nMaxInDataSize, nMaxOutDataSize);
				assert(NULL != m_pIdles[i]);
				if ( NULL == m_pIdles[i] )
				{
					return false;
				}
			}
			m_nIdleEndIndex = (int32_t)m_nMaxClient-1;

			return true;
		}

		inline void Begin()
		{
			m_nCur = 0;
		}
		inline bool IsEnd(){
			return (int32_t)m_nCur >= m_iMaxFd;
		}
		inline void Next(){
			while ( (int32_t)(++m_nCur) <= m_iMaxFd && NULL == m_arrSockets[m_nCur] );
		}
		inline void EraseAndNext(){
			Erase(m_nCur++);
		}
		ITEM_TYPE* Get(){
			if ( NULL != m_arrSockets[m_nCur] )
				return m_arrSockets[m_nCur];
			else
				return NULL;
		}

		inline ITEM_TYPE* Malloc(const SOCKET iFD){
			if ( iFD >= 0 && (int)iFD < MAX_SOCKET )
			{
				if ( m_iMaxFd < iFD )
					m_iMaxFd = iFD;
					
				return  m_arrSockets[iFD] = _new_Item();
			}
			return NULL;
		}

		inline ITEM_TYPE* Find(const SOCKET iFD){
			if ( iFD >= 0 && iFD < MAX_SOCKET && NULL != m_arrSockets[iFD])
			{
				return m_arrSockets[iFD];
			}
			return NULL;
		}

		inline void Erase(const SOCKET iFD){
			assert(iFD >= 0 && iFD < MAX_SOCKET && NULL != m_arrSockets[iFD]);
			if ( iFD >= 0 && iFD < MAX_SOCKET && NULL != m_arrSockets[iFD])
			{
				m_arrSockets[iFD]->Reset();
				_delete_Item(m_arrSockets[iFD]);
				m_arrSockets[iFD] = NULL;
			}
		}
		
		void SetMaxFD(const SOCKET iFD) {	m_iMaxFd = iFD;	}

		size_t GetMaxClientSize() const {	return m_nMaxClient;	}
		size_t GetClientSize() const {	return m_nClientSize;	}

	private:
		ITEM_TYPE*  _new_Item(){
			if ( m_nIdleEndIndex < 0 )
				return NULL;
			
			++m_nClientSize;
			ITEM_TYPE* pItem = m_pIdles[m_nIdleEndIndex];
			m_pIdles[m_nIdleEndIndex--] = NULL;
			assert(NULL != pItem);
			return pItem;
		}
		void _delete_Item(ITEM_TYPE* pItem){
			assert(NULL != pItem);
			assert(m_nIdleEndIndex<(int32_t)m_nMaxClient-1);
			assert(NULL == m_pIdles[1+m_nIdleEndIndex]);
			m_pIdles[++m_nIdleEndIndex] = pItem;
			assert(m_nClientSize>0);
			--m_nClientSize;
		}
	private:
		ITEM_TYPE* m_arrSockets[MAX_SOCKET];
		size_t m_nMaxClient;
		size_t m_nClientSize;
		size_t m_nCur;
		CBufferMalloc* m_poBufferMalloc;

		ITEM_TYPE** m_pIdles;
		int32_t m_nIdleEndIndex;
		SOCKET m_iMaxFd;
	};


	template<typename T>
	class  CSessionManageByFast:private CNonCopyAble
	{
		typedef T ITEM_TYPE;
		struct SItem 
		{
			SItem(CBufferMalloc* poBufferMalloc, const size_t nMaxInDataSize, const size_t nMaxOutDataSize)
				:bUse(false)
				,stCltFd(poBufferMalloc, nMaxInDataSize, nMaxOutDataSize)
			{}
			SItem()
				:bUse(false)
			{}
			bool bUse;
	#ifdef WIN32
			typename ITEM_TYPE stCltFd;
	#else
			ITEM_TYPE stCltFd;
	#endif
		};	
	
		enum{
			MAX_SOCKET = 500000,
		};

	public:
		enum {NEED_LOCK=false};

		CSessionManageByFast()
			:m_nMaxClient(1024)
			,m_nFreeClientSize(0)
			,m_nNewClientSize(0)
			,m_nCur(0)
			,m_poBufferMalloc(NULL)
			,m_iMaxFd(-1)
		{
			memset(m_arrSockets, 0, sizeof(ITEM_TYPE*)*MAX_SOCKET);
		}
		~CSessionManageByFast(){}

		bool Init(const size_t nMaxInDataSize, const size_t nMaxOutDataSize,const size_t nMaxClient, const size_t nTotalMallocSize, const size_t nMallocItemSize){
			m_nMaxClient = nMaxClient;

			m_poBufferMalloc = new CBufferMalloc;
			if ( NULL == m_poBufferMalloc || !m_poBufferMalloc->Init(nTotalMallocSize, nMallocItemSize) )
			{
				return false;
			}
			
			for ( size_t i=0; i<MAX_SOCKET; ++i )
			{
				m_arrSockets[i] = new SItem(m_poBufferMalloc,nMaxInDataSize, nMaxOutDataSize);
				if ( NULL == m_arrSockets[i] )
				{
					return false;
				}
			}
			

			// char* pTmp = new char[sizeof(ITEM_TYPE*)*m_nMaxClient];
			// m_pIdles =  (ITEM_TYPE**)pTmp;
			// for ( size_t i=0; i<m_nMaxClient; ++i )
			// {
				// m_pIdles[i] = new CSocketSession(m_poBufferMalloc,nMaxInDataSize, nMaxOutDataSize);
				// assert(NULL != m_pIdles[i]);
				// if ( NULL == m_pIdles[i] )
				// {
					// return false;
				// }
			// }
			// m_nIdleEndIndex = (int32_t)m_nMaxClient-1;

			return true;
		}

		inline void Begin()
		{
			m_nCur = 0;
		}
		inline bool IsEnd(){
			return (int32_t)m_nCur >= m_iMaxFd;
		}
		inline void Next(){
			while ( (int32_t)(++m_nCur) <= m_iMaxFd && !m_arrSockets[m_nCur]->bUse );
		}
		inline void EraseAndNext(){
			Erase(m_nCur++);
		}
		ITEM_TYPE* Get(){
			if ( m_arrSockets[m_nCur]->bUse )
				return &(m_arrSockets[m_nCur]->stCltFd);
			else
				return NULL;
		}

		inline ITEM_TYPE* Malloc(const SOCKET iFD){
			if ( iFD >= 0 && (int)iFD < MAX_SOCKET &&  m_nNewClientSize < m_nMaxClient + m_nFreeClientSize )
			{
				if ( m_iMaxFd < iFD )
					m_iMaxFd = iFD;
			
				assert(m_arrSockets[iFD]->bUse == false);
				m_arrSockets[iFD]->bUse = true;
				++m_nNewClientSize;
				return &(m_arrSockets[iFD]->stCltFd);
			}
			return NULL;
		}

		inline ITEM_TYPE* Find(const SOCKET iFD){
			if ( iFD >= 0 && iFD < MAX_SOCKET && m_arrSockets[iFD]->bUse )
			{
				return &(m_arrSockets[iFD]->stCltFd);
			}
			return NULL;
		}

		inline void Erase(const SOCKET iFD){
			assert(iFD >= 0 && iFD < MAX_SOCKET && m_arrSockets[iFD]->bUse);
			if ( iFD >= 0 && iFD < MAX_SOCKET && m_arrSockets[iFD]->bUse )
			{
				m_arrSockets[iFD]->bUse = false;
				assert(m_nNewClientSize>m_nFreeClientSize);
				++m_nFreeClientSize;
				m_arrSockets[iFD]->stCltFd.Reset();
			}
		}
		void SetMaxFD(const SOCKET iFD) {	m_iMaxFd = iFD;	}
		size_t GetMaxClientSize() const {	return m_nMaxClient;	}
		size_t GetClientSize() const {	return m_nNewClientSize-m_nFreeClientSize;	}
	private:
		SItem* m_arrSockets[MAX_SOCKET];
		size_t m_nMaxClient;
//		size_t m_nClientSize;
		volatile uint64_t m_nFreeClientSize;
		volatile uint64_t m_nNewClientSize;
		size_t m_nCur;
		CBufferMalloc* m_poBufferMalloc;
		SOCKET m_iMaxFd;
	};	
	
};

#endif


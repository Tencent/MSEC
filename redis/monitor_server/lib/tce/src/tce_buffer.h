
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


#ifndef __TCE_BUFFER_H__
#define __TCE_BUFFER_H__

#include <deque>
#include "tce_buffer_malloc.h"


namespace tce{

	class CBuffer
	{
		struct SItemDataHead{
			char* pNextPtr;
		};	
	public:
		CBuffer(CBufferMalloc* poMalloc, const size_t nMaxDataSize)
			:m_poMalloc(poMalloc)
			,m_nItemSize(poMalloc->GetItemBufSize())
			,m_pDataHead(NULL)
			,m_pDataBeg(NULL)
			,m_pDataEnd(NULL)
			,m_nDataSize(0)
			,m_nFreeBufSize(0)
			,m_nMaxDataSize(nMaxDataSize)
			,m_nErrCode(0)
			,m_nItemCount(0)
		{}
		~CBuffer()
		{
			
		}
	public:
		size_t MaxSize()	{	return m_nMaxDataSize;	}
		bool CanDirectCopy() const {
			return  ( NULL != GetDataBegItem() && GetDataBegItem() == GetDataEndItem() ) ? true :false;
		}
		const char* GetDirectData() const {	
			assert(m_nDataSize == (size_t)(m_pDataEnd-m_pDataBeg));	
			return m_pDataBeg;	
		}
		size_t Size() const {	return  m_nDataSize;	}

		char* GetFreeBuf() {
			NewBuffer();
			return m_pDataEnd;
		}
		size_t GetFreeSize() {
			return m_nFreeBufSize;
		}
		bool Append(const size_t nSize){
			assert(nSize<=m_nFreeBufSize);
			assert((m_pDataEnd - m_poMalloc->GetBufferBegin())%m_nItemSize != 0);
			assert((m_pDataEnd - m_poMalloc->GetBufferBegin())%m_nItemSize + nSize <= m_nItemSize);
			m_pDataEnd += nSize;
			m_nDataSize += nSize;
			m_nFreeBufSize -= nSize;
			return true;
		}

		bool Get(char* pBuf, size_t& nSize) const {
			if ( m_nDataSize > nSize )
			{
				m_nErrCode = -3;
				return false;
			}

			SItemDataHead* pCur = (SItemDataHead*)m_pDataHead;
			size_t nNeedSize = m_nDataSize;
			int32_t nTmp = 0;
			const char* pReadPos = m_pDataBeg;
			while (nNeedSize > 0 && NULL != pCur )
			{
				assert( pReadPos > (char*)pCur && pReadPos < (char*)pCur+m_nItemSize );
				size_t nTmpSize = (char*)pCur + m_nItemSize - pReadPos;
				++nTmp;
//				printf("nTmpSize=%d, nNeedSize=%d, pCur=%x, pcur-next=%x, pReadPos=%x\n", nTmpSize, nNeedSize, pCur, (SItemDataHead*)pCur->pNextPtr, pReadPos);
				if ( nNeedSize >= nTmpSize )
				{
					memcpy(pBuf+m_nDataSize-nNeedSize, pReadPos, nTmpSize);
					nNeedSize -= nTmpSize;
					pCur = (SItemDataHead*)pCur->pNextPtr;
					if(NULL != pCur)
						pReadPos = GetDataPtr((char*)pCur);
					else
					{
//						printf("m_nDataSize=%d, m_nItemCount=%d,nTmp=%d, databeg=%x,dataend=%x, pReadPos=%x\n", m_nDataSize, m_nItemCount, nTmp, m_pDataBeg, m_pDataEnd, pReadPos);
						assert(nNeedSize == 0);
						assert(NULL == pCur);
						pReadPos += nTmpSize;
						assert(pReadPos == m_pDataEnd);
					}
				}
				else
				{
					memcpy(pBuf+m_nDataSize-nNeedSize, pReadPos, nNeedSize);
					pReadPos += nNeedSize;
					nNeedSize = 0;
				}
			}
			nSize = m_nDataSize;
//			printf("m_nDataSize=%d, m_nItemCount=%d,nTmp=%d, databeg=%x,dataend=%x, pReadPos=%x\n", m_nDataSize, m_nItemCount, nTmp, m_pDataBeg, m_pDataEnd, pReadPos);
			assert(pReadPos == m_pDataEnd ||  ( GetDataEndItem()+sizeof(SItemDataHead) == m_pDataEnd && 0 == (pReadPos-m_poMalloc->GetBufferBegin())%m_nItemSize ));
			assert(nTmp == m_nItemCount || (nTmp +1 == m_nItemCount && GetDataEndItem()+sizeof(SItemDataHead) == m_pDataEnd) );
			assert(NULL == pCur || pCur->pNextPtr == NULL);

			return true;
		}

		bool Erase(const size_t nSize){
//			printf("Erase m_nDataSize=%d,nEraseSize=%d\n", m_nDataSize, nSize);
			assert(m_nDataSize >= nSize );
			if (m_nDataSize < nSize)
			{
				m_nErrCode = -3;
				return false;
			}

			size_t nNeedEraseSize = nSize;
			while ( nNeedEraseSize>0 )
			{
				assert( NULL != m_pDataHead);
				assert(m_pDataHead == GetDataBegItem() );
				assert(m_pDataHead + m_nItemSize > m_pDataBeg );
				size_t nTmp = m_nItemSize + m_pDataHead - m_pDataBeg;
				assert ( nTmp <= m_nItemSize-sizeof(SItemDataHead) );
//				printf("Erase nNeedEraseSize=%d, nTmp=%d,m_pDataBeg=%x, m_pDataEnd=%x\n", nNeedEraseSize, nTmp, m_pDataBeg, m_pDataEnd);
				if ( nNeedEraseSize >= nTmp )
				{
					nNeedEraseSize -= nTmp;
					//回收
					char* pDelete = m_pDataHead;
					m_pDataHead = GetNextPtr(m_pDataHead);
					DelBuffer(pDelete);
					
					if ( NULL == m_pDataHead )
					{
						m_pDataBeg = NULL;
						m_pDataEnd = NULL;
					}
					else
					{
						m_pDataBeg = GetDataPtr(m_pDataHead);
					}
				}
				else
				{
					m_pDataBeg += nNeedEraseSize;
					nNeedEraseSize  = 0;
				}
			}
			m_nDataSize -= nSize;

			return true;
		}

		bool Append(const char* pData, const size_t nSize){
			if ( m_poMalloc->GetIdleBufSize()+m_nFreeBufSize < nSize )
			{
				m_nErrCode = -1;
				return false;
			}

			if ( m_nDataSize+nSize > m_nMaxDataSize )
			{
				m_nErrCode = -2;
				return false;
			}

			size_t nNeedWriteSize = nSize;
			while ( nNeedWriteSize>0 )
			{
				if ( m_nFreeBufSize == 0 )
				{
					if ( !NewBuffer() )
					{
						m_nErrCode = -1;
						assert(false);
						return false;
					}
				}
				else
				{
					assert(m_nFreeBufSize<=m_nItemSize-sizeof(SItemDataHead));
					if ( nNeedWriteSize >= m_nFreeBufSize )
					{
						memcpy(m_pDataEnd, pData+nSize-nNeedWriteSize, m_nFreeBufSize);
						m_pDataEnd += m_nFreeBufSize;
						nNeedWriteSize -= m_nFreeBufSize;
						m_nDataSize += m_nFreeBufSize;
						m_nFreeBufSize = 0;
					}
					else
					{
						memcpy(m_pDataEnd, pData+nSize-nNeedWriteSize, nNeedWriteSize);
						m_pDataEnd += nNeedWriteSize;
						m_nDataSize += nNeedWriteSize;
						m_nFreeBufSize -= nNeedWriteSize;
						nNeedWriteSize = 0;
					}
				}
			}

			return true;
		}

		void Clear(){

			//回收
			char* pDelete = NULL;
			while ( NULL != m_pDataHead && m_nItemCount > 0)
			{
//				printf("Clear m_pDataHead=%x\n", m_pDataHead);
				pDelete = m_pDataHead;
				m_pDataHead = GetNextPtr(m_pDataHead);
				DelBuffer(pDelete);
			}
			assert(NULL == m_pDataHead && m_nItemCount == 0);

			m_pDataHead = NULL;
			m_pDataBeg = NULL;
			m_pDataEnd = NULL;
			m_nDataSize = 0;
			m_nFreeBufSize = 0;
		}

		int16_t GetErrCode() const {	return m_nErrCode;	}

	private:
		void DelBuffer(const char* pDel){
			m_poMalloc->Delete(pDel);
			--m_nItemCount;
			assert(m_nItemCount>=0);
		}
	
		bool NewBuffer(){
			if ( m_nFreeBufSize > 0 )
				return true;
				
			//printf("m_nFreeBufSize=%d,m_pDataHead=%x\n", m_nFreeBufSize, m_pDataHead);

			char* pNew = m_poMalloc->New();
			if ( NULL != pNew )
			{
				++m_nItemCount;
				assert(GetNextPtr(pNew) == NULL);
				SetNextPtr(pNew, NULL);
				if ( NULL == m_pDataHead )
				{
					m_pDataHead = pNew;
					m_pDataBeg = m_pDataEnd = GetDataPtr(m_pDataHead);
					m_nFreeBufSize = m_nItemSize-sizeof(SItemDataHead);
				}
				else
				{
					assert(m_pDataEnd > m_poMalloc->GetBufferBegin());
					if ( m_pDataEnd > m_poMalloc->GetBufferBegin() )
					{
						char* pEndItem = GetDataEndItem();

						//check
						char* pTmp = m_pDataHead;
						while(GetNextPtr(pTmp)){ pTmp = GetNextPtr(pTmp);	}
//						printf("bufferBegin=%x, pEndItem=%x, pTmp=%x, m_pDataEnd=%x\n", m_poMalloc->GetBufferBegin(), pEndItem, pTmp, m_pDataEnd);
						assert(pEndItem == pTmp);

						SetNextPtr(pEndItem, pNew);
						assert(0 == (m_pDataEnd-m_poMalloc->GetBufferBegin())%m_nItemSize);
						m_pDataEnd = GetDataPtr(pNew);
						m_nFreeBufSize = m_nItemSize-sizeof(SItemDataHead);
					}
					else
					{
						return false;
					}
				}
			}
			else
			{
				return false;
			}
			
			return true; 
		}

		inline char* GetNextPtr(const char* pData)	{	return ((SItemDataHead*)(pData))->pNextPtr;	}
		inline void SetNextPtr(char* pData, char* pNextData){	
			((SItemDataHead*)(pData))->pNextPtr = pNextData;
		}
		inline char* GetDataPtr(char* pData){	
			if ( NULL == pData )	return NULL;
			return pData+sizeof(SItemDataHead);	
		}
		inline const char* GetDataPtr(char* pData) const {	
			if ( NULL == pData )	return NULL;
			return pData+sizeof(SItemDataHead);	
		}
		inline char* GetDataEndItem() {	
			assert(NULL != m_pDataEnd);
			if ( NULL == m_pDataEnd )	return NULL;
			return (char*)m_poMalloc->GetBufferBegin() + m_nItemSize*((m_pDataEnd - (char*)m_poMalloc->GetBufferBegin() -1)/m_nItemSize);
		}
		inline const char* GetDataEndItem() const {	
			assert(NULL != m_pDataEnd);
			if ( NULL == m_pDataEnd )	return NULL;
			return m_poMalloc->GetBufferBegin() + m_nItemSize*((m_pDataEnd - m_poMalloc->GetBufferBegin() -1)/m_nItemSize);
		}		
		inline char* GetDataBegItem() {
			//assert(NULL != m_pDataBeg);
			if ( NULL == m_pDataBeg )	return NULL;
			return (char*)m_poMalloc->GetBufferBegin() + m_nItemSize*((m_pDataBeg - (char*)m_poMalloc->GetBufferBegin() -1)/m_nItemSize);
		}		
		inline const char* GetDataBegItem() const {
			//assert(NULL != m_pDataBeg);
			if ( NULL == m_pDataBeg )	return NULL;
			return m_poMalloc->GetBufferBegin() + m_nItemSize*((m_pDataBeg - m_poMalloc->GetBufferBegin() -1)/m_nItemSize);
		}		
	private:
		CBuffer(const CBuffer&);
		CBuffer& operator = (const CBuffer&);
	private:
		CBufferMalloc* m_poMalloc;
		const size_t m_nItemSize;
		char* m_pDataHead;
		char* m_pDataBeg;
		char* m_pDataEnd;
		size_t m_nDataSize;
		size_t m_nFreeBufSize;
		size_t m_nMaxDataSize;
		mutable int16_t m_nErrCode;
		int32_t m_nItemCount;
	};

};

#endif




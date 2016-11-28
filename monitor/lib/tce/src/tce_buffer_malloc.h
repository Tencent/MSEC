
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


#ifndef __TCE_BUFFER_MALLOC_H__
#define __TCE_BUFFER_MALLOC_H__

#include <assert.h>


namespace tce{

class CBufferMalloc
{
	struct SItemDataHead{
		char* pNextPtr;
	};
public:
	CBufferMalloc()
		:m_pData(NULL)
		,m_pIdles(NULL)
		,m_nTotalSize(0)
		,m_nItemSize(0)
		,m_nIdleItemSize(0)
	{}
	~CBufferMalloc()
	{
		if ( NULL != m_pData )
			delete[] m_pData;
	}


	bool Init(const size_t nTotalSize=50*1024*1024, const size_t nItemSize = 4*1024)
	{
		if ( nItemSize <= sizeof(SItemDataHead) )
		{
			return false;
		}
		
		m_pData = new char[nTotalSize];
		if ( NULL == m_pData )
		{
			return false;
		}

		for ( size_t i=0; i<nTotalSize/nItemSize; ++i)
		{
			SItemDataHead* pItemHead = (SItemDataHead*)(m_pData + (i*nItemSize));
			pItemHead->pNextPtr = m_pIdles;
			m_pIdles = (char*)pItemHead;
			++m_nIdleItemSize;
		}
//		printf("Init m_pIdles=0x%x, nItemSize=%d, nTotalSize=%d\n", m_pIdles, nItemSize, nTotalSize);


		m_nTotalSize = nTotalSize;
		m_nItemSize = nItemSize;
		return true;
	}

	inline size_t GetItemBufSize() const {	return m_nItemSize;	}

	inline char* New()
	{
		char* pNew = NULL;
		if ( NULL != m_pIdles )
		{
			pNew = m_pIdles;
			m_pIdles = ((SItemDataHead*)m_pIdles)->pNextPtr;
			((SItemDataHead*)pNew)->pNextPtr = NULL;
			assert(m_nIdleItemSize>0);
			--m_nIdleItemSize;
		}
		return pNew;
	}

	inline void Delete(const char* pDelete){
		assert(pDelete>=m_pData);
		assert( (pDelete-m_pData)%m_nItemSize == 0);
		//assert( memset((char*)pDelete,'1',m_nItemSize));//check;

		SItemDataHead* pItemHead = (SItemDataHead*)(pDelete);
		pItemHead->pNextPtr = m_pIdles;
		m_pIdles = (char*)pItemHead;		
		++m_nIdleItemSize;
	}

	size_t GetIdleBufSize() const {	return m_nIdleItemSize*(m_nItemSize-sizeof(SItemDataHead));	}
	const char* GetBufferBegin() const {	return m_pData;	}
private:
	CBufferMalloc(const CBufferMalloc&);
	CBufferMalloc& operator = (const CBufferMalloc&);
private:
	char* m_pData;
	char* m_pIdles;
	size_t m_nTotalSize;
	size_t m_nItemSize;
	size_t m_nIdleItemSize;
};


};

#endif





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


#ifndef __TCE_SHM_BITMAP_H__
#define __TCE_SHM_BITMAP_H__

#include "tce_def.h"
#include "tce_shm.h"

namespace tce{


namespace shm{

	template<size_t _bit>
	class CBitmap{};

	//0-1
	__TCE_TEMPLATE_NULL class CBitmap<1>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap()
			:m_bInit(false)
		{}
		~CBitmap(){}
		bool init(const int iShmKey, const size_t nBitmapSize,  const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = (nBitmapSize+7)/8 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
			
		}

		bool get(const size_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			char cFlag;
			cFlag = *(m_pDataPtr+nIndex/ 8) >> (nIndex % 8);
			cFlag = cFlag & 0x1;
			return (cFlag ==1 ? true:false);
		}

		bool set(const size_t nIndex, const bool bVal){
			int  iIndexBitPos;
			static unsigned char caUinFlag[] = {0xfe, 0xfd, 0xfb, 0xf7,0xef,0xdf,0xbf,0x7f};
			char cFlag = bVal ? 1 : 0;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex % 8;


			cFlag = cFlag << (iIndexBitPos);

			*(m_pDataPtr+nIndex/8) &= caUinFlag[iIndexBitPos];		//清位为0
			*(m_pDataPtr+nIndex/8) |= cFlag;//

			return true;
		}

		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}

		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};


	//0-3
	__TCE_TEMPLATE_NULL class CBitmap<2>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;
	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*2 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;

		}

		int get(const size_t nIndex)  const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			unsigned char ucFlag;
			ucFlag = *(m_pDataPtr+nIndex/ 4) >> (nIndex%4*2);
			ucFlag = ucFlag & 0x3;
			return ucFlag;
		}

		bool set(const size_t nIndex, const unsigned char ucVal){
			int  iIndexBitPos;
			static unsigned char caUinFlag[] = {0xfc, 0xf3, 0xcf, 0x3f};
			uint32_t ucFlag = ucVal;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex%4;


			ucFlag = ucFlag << (iIndexBitPos*2);

			*(m_pDataPtr+nIndex/4) &= caUinFlag[iIndexBitPos];		//清位为0
			*(m_pDataPtr+nIndex/4) |= ucFlag;//

			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};


	//0-7
	__TCE_TEMPLATE_NULL class CBitmap<3>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*3 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
		}

		int get(const size_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			uint32_t dwTmpValue = *(uint32_t*)(m_pDataPtr+(nIndex/8)*3) >> (nIndex%8*3 );
			dwTmpValue = dwTmpValue & 0x7;
			return dwTmpValue;
		}

		bool set(const size_t nIndex, const unsigned char ucVal){
			int  iIndexBitPos;
			static uint32_t caUinFlag[] = {0xFFFFFFF8, 0xFFFFFFC7, 0xFFFFFE3F, 0xFFFFF1FF, 0xFFFF8FFF, 0xFFFC7FFF, 0xFFE3FFFF, 0xFF1FFFFF};
			uint32_t dwFlag = ucVal;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex%8;

			dwFlag = dwFlag << (iIndexBitPos*3);

			*(uint32_t*)(m_pDataPtr+(nIndex/8)*3) &= caUinFlag[iIndexBitPos];		//清位为0
			*(uint32_t*)(m_pDataPtr+(nIndex/8)*3) |= dwFlag;//

			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};


	//0-15
	__TCE_TEMPLATE_NULL class CBitmap<4>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*4 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
		}

		int get(const uint32_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			unsigned char ucFlag;
			ucFlag = *(m_pDataPtr+nIndex/ 2) >> (nIndex%2*4);
			ucFlag = ucFlag & 0x0F;
			return ucFlag;
		}

		bool set(const uint32_t nIndex, const unsigned char ucVal){
			int  iIndexBitPos;
			static unsigned char caUinFlag[] = {0xf0, 0x0f};
			unsigned char ucFlag = ucVal;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex%2;


			ucFlag = ucFlag << (iIndexBitPos*4);

			*(m_pDataPtr+nIndex/2) &= caUinFlag[iIndexBitPos];		//清位为0
			*(m_pDataPtr+nIndex/2) |= ucFlag;//

			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};


	//0-31
	__TCE_TEMPLATE_NULL class CBitmap<5>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*5 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
		}

		int get(const size_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			uint64_t ui64TmpValue= *(int64_t*)(m_pDataPtr+(nIndex/8)*5) >> (nIndex%8*5 );
			ui64TmpValue = ui64TmpValue & 0x1F;
			return (int)ui64TmpValue;
		}

		bool set(const size_t nIndex, const unsigned char ucVal){
			int  iIndexBitPos;
			static uint64_t caUinFlag[] = {0xFFFFFFFFFFFFFFE0LL, 0xFFFFFFFFFFFFFC1FLL, 0xFFFFFFFFFFFF83FFLL, 0xFFFFFFFFFFF07FFFLL, 0xFFFFFFFFFE0FFFFFLL, 0xFFFFFFFFC1FFFFFFLL, 0xFFFFFFF83FFFFFFFLL, 0xFFFFFF07FFFFFFFFLL};
			uint64_t ui64Flag = ucVal;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex%8;

			ui64Flag = ui64Flag << (iIndexBitPos*5);

			*(uint64_t*)(m_pDataPtr+(nIndex/8)*5) &= caUinFlag[iIndexBitPos];		//清位为0
			*(uint64_t*)(m_pDataPtr+(nIndex/8)*5) |= ui64Flag;//

			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};




	//0-63
	__TCE_TEMPLATE_NULL class CBitmap<6>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*6 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
		}

		int get(const size_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			uint32_t dwTmpValue = *(uint32_t*)(m_pDataPtr+(nIndex/4)*3) >> (nIndex%4*6 );
			dwTmpValue = dwTmpValue & 0x3F;
			return dwTmpValue;
		}

		bool set(const size_t nIndex, const unsigned char ucVal){
			int  iIndexBitPos;
			static uint32_t caUinFlag[] = {0xFFFFFFC0, 0xFFFFF03F, 0xFFFC0FFF, 0xFF03FFFF};
			uint32_t dwFlag = ucVal;

			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			iIndexBitPos = nIndex%4;

			dwFlag = dwFlag << (iIndexBitPos*6);

			*(uint32_t*)(m_pDataPtr+(nIndex/4)*3) &= caUinFlag[iIndexBitPos];		//清位为0
			*(uint32_t*)(m_pDataPtr+(nIndex/4)*3) |= dwFlag;//

			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};




	__TCE_TEMPLATE_NULL class CBitmap<8>
	{
		typedef size_t size_type;
		struct SHead{
			char szReserve[1024];
		};
		typedef SHead head_type;

	public:
		CBitmap(){}
		~CBitmap(){}

		bool init(const int iShmKey, const size_t nBitmapSize, const bool bReadOnly=false){
			if ( m_bInit )
				return true;

			m_nShmSize = ((nBitmapSize+7)/8)*8 + sizeof(SHead);
			m_nMaxIndex = nBitmapSize;

			if ( !m_oShm.Init( iShmKey, m_nShmSize ) )
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
			m_pDataPtr  = (unsigned char*)(reinterpret_cast<char*>(m_oShm.GetShmBuf())+sizeof(head_type));
			
			m_bInit = true;
			return true;
		}

		int get(const size_t nIndex) const {
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			return *(m_pDataPtr+nIndex);
		}

		bool set(const size_t nIndex, const unsigned char ucVal){
			if ( nIndex > m_nMaxIndex )
			{
				return false;
			}

			*(m_pDataPtr+nIndex) = ucVal;
			return true;
		}
		void clear()	{	memset(m_pDataPtr, 0, m_nShmSize-sizeof(SHead));	}
		const char* data() const {	return (char*)m_pDataPtr;	}
		size_t size() const {	return m_nMaxIndex;	}
	protected:
	private:
		SHead* m_pHead;
		unsigned char* m_pDataPtr;
		size_t m_nMaxIndex;
		tce::CShm m_oShm;
		char m_szErrMsg[1024];
		bool m_bInit;
		size_t m_nShmSize;
	};




	//1bit的bitmap
	typedef CBitmap<1> BITMAP;	//1bit的bitmap
	typedef CBitmap<1> BITMAP1;

	typedef CBitmap<2> BITMAP2;	//2bit的bitmap
	
	typedef CBitmap<3> BITMAP3;	//3bit的bitmap
	
	typedef CBitmap<4> BITMAP4;	//4bit的bitmap
	
	typedef CBitmap<5> BITMAP5;	//5bit的bitmap
	
	typedef CBitmap<6> BITMAP6;	//6bit的bitmap
	
	typedef CBitmap<8> BITMAP8;	//8bit的bitmap


};

};

#endif




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


#ifndef __TCE_PARSE_TCP_PKG_H__
#define __TCE_PARSE_TCP_PKG_H__


namespace tce{


	class CParsePkgByHead5ShortLenTail3{
		enum {
			PKG_HEAD_MARK=0x05,
			PKG_TAIL_MARK=0x03,
		};
		enum {
			MAX_PKG_SIZE=(unsigned short)0xffff,
		};
	public:
		CParsePkgByHead5ShortLenTail3(){}
		~CParsePkgByHead5ShortLenTail3(){}

		inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize){
			return pszData+3;
		}

		// return value: -2:非法包; -1:不完整包; 0:完整包
		static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen){
			int iRe = -2;
			if (nDataSize <= 0)
			{
				return -1;
			}

			if (nDataSize <= 3)
			{
				iRe = -1;
				if ( *pszData != PKG_HEAD_MARK )
				{
					iRe = -2;
				}
			}
			else
			{
				nPkgLen = ntohs(*((unsigned short*)(pszData+1)));
				if ( nDataSize >= nPkgLen && nPkgLen >= 4  )
				{

					if ( *(pszData+nPkgLen-1) == PKG_TAIL_MARK )
					{
						nRealPkgLen = nPkgLen - 4;
						iRe = 0;
					}
					else
					{
						iRe = -2;
					}
				}
				else
				{
					iRe = -1;
					if ( *pszData != PKG_HEAD_MARK || nPkgLen < 4 )
					{
						iRe = -2;
					}
				}
			}
			return iRe;
		}

		static inline const char* MakeSendPkg(char* pSendBuf, size_t& nSendDataSize, const char* pszData, const size_t nDataSize){
			if ( nDataSize <= 0 && nSendDataSize < nDataSize+4 )
				return NULL;

			pSendBuf[0] = PKG_HEAD_MARK;
			pSendBuf[nDataSize+3] = PKG_TAIL_MARK;
			*((uint16_t*)(pSendBuf)) = htons(nDataSize+4);
			memcpy(pSendBuf+3, pszData, nDataSize);
			nSendDataSize = nDataSize+4;
			return pSendBuf;
		}	

		//支持某种奇怪的代理（前两个字节为0）
		static inline bool IsSuppertProxy(){
			return true;
		}

		static inline bool IsNeedReadBeforeClose(){
			return false;
		}

	};


class CParsePkgByHead5ShortVerShortLenTail3{
	enum {
		PKG_HEAD_MARK=0x05,
		PKG_TAIL_MARK=0x03,
	};
	enum {
		MAX_PKG_SIZE=(unsigned short)0xffff,
	};
public:
	CParsePkgByHead5ShortVerShortLenTail3(){}
	~CParsePkgByHead5ShortVerShortLenTail3(){}

	inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize){
		return pszData+5;
	}

	// return value: -2:非法包; -1:不完整包; 0:完整包
	static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen){
		int iRe = -2;
		if (nDataSize <= 0)
		{
			return -1;
		}

		if (nDataSize <= 5)
		{
			iRe = -1;
			if ( *pszData != PKG_HEAD_MARK )
			{
				iRe = -2;
			}
		}
		else
		{
			nPkgLen = ntohs(*((unsigned short*)(pszData+3)));
			if ( nDataSize >= nPkgLen && nPkgLen >= 6  )
			{

				if ( *(pszData+nPkgLen-1) == PKG_TAIL_MARK )
				{
					nRealPkgLen = nPkgLen - 6;
					iRe = 0;
				}
				else
				{
					iRe = -2;
				}
			}
			else
			{
				iRe = -1;
				if ( *pszData != PKG_HEAD_MARK || nPkgLen < 6 )
				{
					iRe = -2;
				}
			}
		}
		return iRe;
	}

	static inline const char* MakeSendPkg(char* pSendBuf, size_t& nSendDataSize, const char* pszData, const size_t nDataSize){
		if ( nDataSize <= 0 && nSendDataSize < nDataSize+6 )
			return NULL;

		pSendBuf[0] = PKG_HEAD_MARK;
		pSendBuf[nDataSize+5] = PKG_TAIL_MARK;
		*((uint16_t*)(pSendBuf+5)) = htonl(nDataSize+6);
		memcpy(pSendBuf+5, pszData, nDataSize);
		nSendDataSize = nDataSize+6;
		return pSendBuf;
	}		

	//支持某种奇怪的代理（前两个字节为0）
	static inline bool IsSuppertProxy(){
		return true;
	}

	static inline bool IsNeedReadBeforeClose(){
		return false;
	}

};

};


#endif


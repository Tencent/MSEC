
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


#ifndef __TCE_PARSE_PKG_ASN_H_2013_06_20_kylehuang__
#define __TCE_PARSE_PKG_ASN_H_2013_06_20_kylehuang__

namespace tce{


class CParsePkgAsn{
public:
	CParsePkgAsn(){}
	~CParsePkgAsn(){}

	inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize)
	{
		return pszData;
		#if 0
		if ( (unsigned)pszData[1] > 128 )
		{
			unsigned len_len = (unsigned)(pszData[1]&127);
			return pszData + len_len + 2;
		}
		else
		{
			return pszData + 2;
		}
		#endif
	}

	// return value: -2:非法包; -1:不完整包; 0:完整包
	static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen)
	{
		int iRe = -2;
		const unsigned MAX_LEN = 1024*1024*128;
		unsigned head_len = 0;
		unsigned cont_len = 0;
		if (nDataSize <= 0)
		{
			return -1;
		}

		if (nDataSize < 4)
		{
			iRe = -1;
			if ( *pszData != '0' )
			{
				printf("%s:%d\n", __func__, __LINE__);
				iRe = -2;
			}
		}
		else
		{
			if ((unsigned)pszData[1] > 128)
			{
				unsigned len_len = (unsigned)(pszData[1]&127);
				char *addr_dest = (char*)(&cont_len)+len_len-1;
				const char *addr_src = pszData + 2;

				head_len = 2 + len_len;

				while (addr_dest >= (char*)(&cont_len))
				{
					memcpy(addr_dest--, addr_src++, 1);
				}
			}
			else
			{
				head_len = 2;
				cont_len = (unsigned)pszData[1];
			}

			nPkgLen = head_len + cont_len;
			if ( MAX_LEN < nPkgLen )
			{
				printf("%s:%d\n", __func__, __LINE__);
				return -2;	
			}

			if ( nDataSize >= nPkgLen && nPkgLen >= 4 )
			{
				#if 0
				if ( pszData[nPkgLen] != '0' )
				{
					printf("%s:%d\n", __func__, __LINE__);
					iRe = -2;	
				}
				else
				{
				#endif
					iRe = 0;
//					nRealPkgLen = nPkgLen - head_len - 1;
					nRealPkgLen = nPkgLen;
				//}
			}
			else
			{
				iRe = -1;	
				if ( *pszData != '0' )
				{
					printf("%s:%d\n", __func__, __LINE__);
					iRe = -2;
				}
			}
		}
		return iRe;
	}

	static inline const char* MakeSendPkg(char* pSendBuf, size_t& nSendDataSize, const char* pszData, const size_t nDataSize)
	{
		if ( nDataSize <= 0 )
			return NULL;

		nSendDataSize = nDataSize;
		return pszData;

		#if 0
		pSendBuf[0] = '0';
		if ( nDataSize + 1 < 128 )
		{
			pSendBuf[nDataSize+2] = '0';
			*((char*)pSendBuf+1) = (unsigned char)(nDataSize+1);
			memcpy( pSendBuf+2, pszData, nDataSize );
			nSendDataSize = nDataSize + 3;
			return pSendBuf;
		}
		else if ( nDataSize + 1 < 256 )
		{
			pSendBuf[nDataSize+3] = '0';
			*((char*)pSendBuf+1) = 0x81;
			*((char*)pSendBuf+2) = (unsigned char)(nDataSize+1);
			memcpy( pSendBuf+3, pszData, nDataSize );
			nSendDataSize = nDataSize + 4;
			return pSendBuf;
		}
		else if ( nDataSize + 1 < 65536 )
		{
			pSendBuf[nDataSize+4] = '0';
			*((char*)pSendBuf+1) = 0x82;
			*((char*)pSendBuf+2) = (unsigned char)((nDataSize+1) >> 8);
			*((char*)pSendBuf+3) = (unsigned char)(nDataSize+1);
			memcpy( pSendBuf+4, pszData, nDataSize );
			nSendDataSize = nDataSize + 5;
			return pSendBuf;
		}
		else if ( nDataSize + 1 < 16777126 )
		{
			pSendBuf[nDataSize+5] = '0';
			*((char*)pSendBuf+1) = 0x83;
			*((char*)pSendBuf+2) = (unsigned char)((nDataSize+1) >> 16);
			*((char*)pSendBuf+3) = (unsigned char)((nDataSize+1) >> 8);
			*((char*)pSendBuf+4) = (unsigned char)(nDataSize+1);
			memcpy( pSendBuf+5, pszData, nDataSize );
			nSendDataSize = nDataSize + 6;
			return pSendBuf;
		}
		else
		{
			pSendBuf[nDataSize+6] = '0';
			*((char*)pSendBuf+1) = 0x84;
			*((char*)pSendBuf+2) = (unsigned char)((nDataSize+1) >> 24);
			*((char*)pSendBuf+3) = (unsigned char)((nDataSize+1) >> 16);
			*((char*)pSendBuf+4) = (unsigned char)((nDataSize+1) >> 8);
			*((char*)pSendBuf+5) = (unsigned char)(nDataSize+1);
			memcpy( pSendBuf+6, pszData, nDataSize );
			nSendDataSize = nDataSize + 7;
			return pSendBuf;
		}
		return NULL;
		#endif
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


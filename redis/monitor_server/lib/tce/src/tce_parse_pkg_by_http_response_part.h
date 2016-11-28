
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


#ifndef __TCE_PARSE_PKG_BY_HTTP_RESPONSE_PART_H__
#define __TCE_PARSE_PKG_BY_HTTP_RESPONSE_PART_H__

#include <stdlib.h>

namespace tce{


class CParsePkgByHttpResponsePart{
	enum {
		MAX_PKG_SIZE=(unsigned int)-1,
	};
public:
	CParsePkgByHttpResponsePart(){}
	~CParsePkgByHttpResponsePart(){}

	inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize){
		return pszData;
	}

	// return value: -2:非法包; -1:不完整包; 0:完整包
	static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen){
		int iRe = -1;
		
		if ( nDataSize <= 0 )
			return iRe;

		if ( bRequest )
		{
			const char* pEnd = ::strstr(pszData, "\r\n\r\n");
			if ( NULL != pEnd && pEnd+4 <= pszData+nDataSize )
			{
				const char* pContentLenPos = ::strstr(pszData, "Content-Length: ");
				if ( NULL != pContentLenPos )
				{
					if ( pEnd > pContentLenPos )
					{
						size_t nTmpSize = atoi(pContentLenPos+15) + (pEnd-pszData) + 4;
						if ( nTmpSize <= nDataSize)
						{
							iRe = 0;
							nRealPkgLen = nPkgLen = nTmpSize;	
						}
					}
					else
					{
						iRe = 0;
						nRealPkgLen = nPkgLen = nDataSize;				
					}
				}
				else
				{
					iRe = 0;
					nRealPkgLen = nPkgLen = nDataSize;				
				}
			}
			else if ( nDataSize > 1024*1024 )
			{
				iRe = -2;
			}
		}
		else
		{
			if(*(uint32_t *)pszData != *(uint32_t *)"HTTP" ||
				pszData[4] != '/' || !isdigit(pszData[5]) ||
				pszData[6] != '.' || !isdigit(pszData[7]) || 
				pszData[8] != ' ' || nDataSize < 10) 
			{
				//非包头数据
				iRe = 0;
				nRealPkgLen = nPkgLen = nDataSize;
			}
			else
			{//包头数据，需要收到完整的包头再往应用层传递
				char * pEnd = (char*)::strstr(pszData, "\r\n\r\n");
				if ( NULL != pEnd && pEnd < nDataSize + pszData )
				{
					iRe = 0;
					nRealPkgLen = nPkgLen = nDataSize;
				}
			}
		}
			

		return iRe;
	}

	static inline const char* MakeSendPkg(char* pSendBuf, size_t& nSendDataSize, const char* pszData, const size_t nDataSize){
		if ( nDataSize <= 0 )
			return NULL;

		nSendDataSize = nDataSize;
		return pszData;
	}
	

	//支持某种奇怪的代理（前两个字节为0）
	static inline bool IsSuppertProxy(){
		return false;
	}

	static inline bool IsNeedReadBeforeClose(){
		return true;
	}

};

};


#endif


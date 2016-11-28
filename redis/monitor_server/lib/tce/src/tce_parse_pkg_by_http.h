
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


#ifndef __TCE_PARSE_PKG_BY_HTTP_H__
#define __TCE_PARSE_PKG_BY_HTTP_H__

#include <stdlib.h>

namespace tce{


class CParsePkgByHttp{
	enum {
		MAX_PKG_SIZE=(unsigned int)-1,
	};
public:
	CParsePkgByHttp(){}
	~CParsePkgByHttp(){}

	inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize){
		return pszData;
	}

	// return value: -2:非法包; -1:不完整包; 0:完整包
	static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen){
		int32_t iRe = -1;
		const char* pFirstLine = ::strstr(pszData, "\r\n");
		if ( NULL != pFirstLine )
		{
			const char* pEnd = ::strstr(pszData, "\r\n\r\n")+sizeof("\r\n\r\n")-1;
			if ( NULL != pEnd )
			{
				size_t iHeadLen = (size_t)(pEnd-pszData);
				size_t iHTMLLen = iHeadLen;

				if(*(uint32_t *)pszData == *(uint32_t *)"HTTP") 	//接收请求返回的数据
				{
					const char* pContentLenPos = ::strstr(pszData, "Content-Length: ");
					if ( NULL != pContentLenPos )
					{
						if ( pEnd > pContentLenPos )
						{
							const char* pContentLenEndPos = strstr(pContentLenPos, "\r\n");
							if ( NULL != pContentLenEndPos && pContentLenPos < pEnd )
							{
								//bug modified by alex 2008-11-15
								//int iContentLen = atoi(pContentLenPos);+strlen("Content-Length: ")
								int iContentLen = atoi(pContentLenPos+sizeof("Content-Length: ")-1);
								iHTMLLen += iContentLen;
							}
						}

						if ( iHTMLLen <= nDataSize && iHTMLLen > iHeadLen )
						{
							//alex 2008-11-15
							//nRealPkgLen = nPkgLen = nDataSize;
							nRealPkgLen = nPkgLen = iHTMLLen;
							//printf("接收请求返回的数据 nDataSize=%d, iHTMLLen=%d\n", nDataSize, iHTMLLen);
							iRe = 0;
						}
					}
					if ( iHTMLLen <= nDataSize && iHTMLLen > 0 )
					{
						//nRealPkgLen = nPkgLen = nDataSize;
						nRealPkgLen = nPkgLen = iHTMLLen;
						//printf("接入http请求 nDataSize=%d\n", nDataSize);
						//printf("*****http_pkg=%s*****\n", pszData);
						iRe = 0;
					}
				}
				else	//接入http请求
				{
					const char* pContentLenPos = ::strstr(pszData, "Content-Length: ");
					if ( NULL != pContentLenPos )
					{
						if ( pEnd > pContentLenPos )
						{
							const char* pContentLenEndPos = strstr(pContentLenPos, "\r\n");
							if ( NULL != pContentLenEndPos && pContentLenPos < pEnd )
							{
								//bug modified by alex 2008-11-15
								//int iContentLen = atoi(pContentLenPos);
								int iContentLen = atoi(pContentLenPos+sizeof("Content-Length: ")-1);
								iHTMLLen += iContentLen;
							}
						}
					}
					if ( iHTMLLen <= nDataSize && iHTMLLen > 0 )
					{
						//alex 2008-11-15
						//nRealPkgLen = nPkgLen = nDataSize;
						nRealPkgLen = nPkgLen = iHTMLLen;
						//printf("接入http请求 nDataSize=%d\n", nDataSize);
						iRe = 0;
					}
				}
			}
		}
		return iRe;
	}

	// static inline const char* MakeSendPkg(std::string& sSendBuf, const char* pszData, const int nDataSize, int& iSendDataSize){
		// iSendDataSize = nDataSize;
		// return pszData;
	// }

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


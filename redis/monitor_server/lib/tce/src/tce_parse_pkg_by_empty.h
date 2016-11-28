
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


#ifndef __TCE_PARSE_PKG_BY_EMPTY_H__
#define __TCE_PARSE_PKG_BY_EMPTY_H__

namespace tce{


class CParsePkgByEmpty{

public:
	CParsePkgByEmpty(){}
	~CParsePkgByEmpty(){}

	inline static const char* GetRealPkgData(const char* pszData, const size_t nDataSize){
		return pszData;
	}

	// return value: -2:非法包; -1:不完整包; 0:完整包
	static inline int32_t HasWholePkg(const bool bRequest, const char* pszData, const size_t nDataSize, size_t& nRealPkgLen, size_t& nPkgLen){
		int iRe = -2;
		if (nDataSize <= 0)
		{
			return -1;
		}
		else
		{
			nRealPkgLen = nPkgLen = nDataSize;
			iRe = 0;
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


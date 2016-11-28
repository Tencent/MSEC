
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


#ifndef __TCE_HTTP_PARSER_H__
#define __TCE_HTTP_PARSER_H__

#include <map>
#include <string>
#include "tce_utils.h"
#include "tce_value.h"
#include <assert.h>
using std::map;
using std::string;

namespace tce{

enum HTTP_METHOD{
	METHOD_GET = 0,
	METHOD_POST = 1,
	METHOD_HEAD = 2
};


typedef map<string, string> MAP_COOKIE;
typedef map<string, string> MAP_VALUE;

class CHttpParser
{
	enum{
		HTTP_MAX_URI_LEN				= 4096,
		HTTP_MAX_COOKIE_LEN				= 4096,
		HTTP_MAX_REFERER_LEN			= 4096,
		HTTP_MAX_USERAGENT_LEN			= 4096,
		HTTP_OTHER_HEAD_CNT				= 100,
		HTTP_OTHER_HEAD_NAME_LEN		= 128,
		HTTP_OTHER_HEAD_DATA_LEN		= 4096,
		MAX_ALLOW_CONTENT_LEN 			= 1024*1024,
		TAG_WIDTH = 10,
		TAG_LENGHT = 20,	
	};

private:
	typedef bool (*DECODE_FUNC)(CHttpParser& oInstance, const char* pLine, const char* pLineEnd);
public:
	CHttpParser(void);
	~CHttpParser(void);

	bool Decode(const char* pszData, const size_t nSize);
	bool DecodeRequest(const char* pszData, const size_t nSize);
	bool DecodeResponse(const char* pszData, const size_t nSize);
	
	const int32_t GetMethod() const {	return m_nMethod;	}
	const char* GetURI() const {	return m_szURI;	}
	const char* GetHost() const {	return m_szHost;	}
	const int32_t GetStatusCode() const {	return m_nStatusCode;	}
	const bool GetKeepAlive() const {	return m_bKeepAlive;	}
	
	inline tce::CValue GetValue(const std::string& sName, const tce::CValue& vDefValue="")
	{
		if ( m_bDecodeValueFlag == 0 ) DecodeValues();
		
		MAP_VALUE::iterator it = m_mapValues.find(sName);
		if (it != m_mapValues.end())
		{
			return tce::CValue(it->second);
		}
		return vDefValue;
	}
	inline bool HasValue(const std::string& sName)
	{
		if ( m_bDecodeValueFlag == 0 ) DecodeValues();
		MAP_VALUE::iterator it = m_mapValues.find(sName);
		if (it != m_mapValues.end())
		{
			return true;
		}
		return false;
	}	

	inline tce::CValue GetCookie(const std::string& sName, const tce::CValue& vDefValue="")
	{
		if ( m_bDecodeCookieFlag == 0 ) DecodeCookies();
		MAP_COOKIE::iterator it = m_mapCookies.find(sName);
		if (it != m_mapCookies.end())
		{
			return tce::CValue(it->second);
		}
		return vDefValue;
	}
	
	inline bool HasCookie(const std::string& sName)
	{
		if ( m_bDecodeCookieFlag == 0 ) DecodeCookies();
		MAP_COOKIE::iterator it = m_mapCookies.find(sName);
		if (it != m_mapCookies.end())
		{
			return true;
		}
		return false;
	}	
		
	const char* GetErrMsg() const {	return m_szErrMsg;	}
	void Clear(){
		m_ucOtherHeadCnt = 0;
		m_nMethod = 0;
		m_nStatusCode = 200;	
		m_mapCookies.clear();
		m_mapValues.clear();
		m_nFlagValue = 0;
		m_szURIByAll[0] = 0;
		m_szURI[0] = 0;
		m_szContentType[0] = 0;
		m_szHost[0] = 0;
		m_szReferer[0] = 0;
		m_szCookie[0] = 0;
		m_szUserAgent[0] = 0;
		m_szAcceptLanguage[0] = 0;
		m_szErrMsg[0] = 0;
		m_nHeadDataSize = 0;
		m_sValues.clear();
	}
private:

	CHttpParser(const CHttpParser& rhs);
	CHttpParser& operator=(const CHttpParser& rhs);

	bool DecodeValues();
	bool DecodeCookies();
	
private:
	static char m_arrFirstCharIndex[255];
	static bool m_bInitStaticData;
	static DECODE_FUNC m_arrTagFuncs[TAG_WIDTH*TAG_LENGHT];
	char m_szNULL[2];
	char m_szErrMsg[1024];
	size_t m_nHeadDataSize;
	char m_nMethod;
	int32_t m_nStatusCode;
	char m_szURIByAll[HTTP_MAX_URI_LEN];
	char m_szURI[HTTP_MAX_URI_LEN];
	string m_sValues;
	char m_szContentType[128];
	char m_szHost[128];
	char m_szReferer[HTTP_MAX_REFERER_LEN];
	char m_szCookie[HTTP_MAX_COOKIE_LEN];
	char m_szUserAgent[HTTP_MAX_USERAGENT_LEN];
	char m_szAcceptLanguage[128];
	uint64_t m_nContentLength;
	uint32_t m_nClientIP;
	union{
		struct{
			unsigned char m_bRequest:1;
			unsigned char m_bNoHeaderFlag:1;
			unsigned char m_bHttpV11:1;
			unsigned char m_bKeepAlive:1;
			unsigned char m_bHasContentType:1;
			unsigned char m_bHasContentLength:1;
			unsigned char m_bHasChunked:1;
			unsigned char m_bCookieLenOverflow:1;
			unsigned char m_bResponseComplete:1;		//数据接收完成
			unsigned char m_bHeadFormatOk:1;	//头部解析正确标志位
			unsigned char m_bBodyFormatErr:1;	//Body解析错误标志位
			unsigned char m_bDecodeValueFlag:1;		//是否解析了参数内容（1：是，0：否）
			unsigned char m_bDecodeCookieFlag:1;	//是否解析了cookie内容	（1：是，0：否）
		};
		uint64_t m_nFlagValue;
	};

	unsigned char m_ucOtherHeadCnt;
	char m_arrOtherHeadName[HTTP_OTHER_HEAD_CNT][HTTP_OTHER_HEAD_NAME_LEN];
	char m_arrOtherHeadValue[HTTP_OTHER_HEAD_CNT][HTTP_OTHER_HEAD_DATA_LEN];
	
	MAP_COOKIE m_mapCookies;
	MAP_VALUE  m_mapValues;
	
private:
	static inline bool decode_connection(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Connection:", 11))
		{
			pLine += 11;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			if(!strncasecmp(pLine, "close", 5)) 
			{
				oInstance.m_bKeepAlive = false;
			} 
			else if( 0 == strncasecmp(pLine, "keep-alive", 10))
			{
				oInstance.m_bKeepAlive = true;
			}
		}

		return true;
	}

	static inline bool decode_content_type(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Content-type:", 13) ) 
		{
			pLine += 13;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			oInstance.m_bHasContentType = true;
			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen > 128 )
			{
				memcpy(oInstance.m_szContentType, pLine, 128);
				oInstance.m_szContentType[127] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szContentType, pLine, nValueLen);
				oInstance.m_szContentType[nValueLen] = 0;
			}
		}

		return true;
	}
	static inline bool decode_content_length(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Content-length:", 15) ) 
		{
			if(oInstance.m_nMethod != METHOD_POST) {
				oInstance.m_bKeepAlive = false;
			}
			oInstance.m_bHasContentLength = true;
			oInstance.m_bHasChunked = false;
			
			oInstance.m_nContentLength = atoi(pLine+15);
			if(oInstance.m_nContentLength < 0 || oInstance.m_nContentLength > MAX_ALLOW_CONTENT_LEN ) 
			{
				oInstance.m_nStatusCode = 413;
				return false;
			}
		}

		return true;
	}
	static inline bool decode_cookie(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Cookie:", 7) )
		{
			pLine += 7;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen >= HTTP_MAX_COOKIE_LEN )
			{
				oInstance.m_bCookieLenOverflow = true;
				memcpy(oInstance.m_szCookie, pLine, HTTP_MAX_COOKIE_LEN);
				oInstance.m_szCookie[HTTP_MAX_COOKIE_LEN-1] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szCookie, pLine, nValueLen);
				oInstance.m_szCookie[nValueLen] = 0;
			}
		}

		return true;
	}

	static inline bool decode_accept_language(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Accept-Language:", 16) )
		{
			pLine += 16;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen > 128 )
			{
				memcpy(oInstance.m_szAcceptLanguage, pLine, 128);
				oInstance.m_szAcceptLanguage[127] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szAcceptLanguage, pLine, nValueLen);
				oInstance.m_szAcceptLanguage[nValueLen] = 0;
			}
		}

		return true;
	}

	static inline bool decode_user_agent(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "User-Agent:", 11) )
		{
			pLine += 11;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen >= HTTP_MAX_USERAGENT_LEN )
			{
				memcpy(oInstance.m_szUserAgent, pLine, HTTP_MAX_USERAGENT_LEN);
				oInstance.m_szUserAgent[HTTP_MAX_USERAGENT_LEN-1] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szUserAgent, pLine, nValueLen);
				oInstance.m_szUserAgent[nValueLen] = 0;
			}
		}
		return true;		
	}

	static inline bool decode_host(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Host:", 5) )
		{
			pLine += 5;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen > 128 )
			{
				memcpy(oInstance.m_szHost, pLine, 128);
				oInstance.m_szHost[127] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szHost, pLine, nValueLen);
				oInstance.m_szHost[nValueLen] = 0;
			}
		}
		return true;
	}
	static inline bool decode_if_modified_since(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_if_unmodified_since(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_if_range(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_request_range(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_referer(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Referer:", 8) )
		{
			pLine += 8;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			int32_t nValueLen = pLineEnd-pLine;
			if ( nValueLen >= HTTP_MAX_REFERER_LEN )
			{
				memcpy(oInstance.m_szReferer, pLine, HTTP_MAX_REFERER_LEN);
				oInstance.m_szReferer[HTTP_MAX_REFERER_LEN-1] = 0;
			}
			else if ( nValueLen >= 0)
			{
				memcpy(oInstance.m_szReferer, pLine, nValueLen);
				oInstance.m_szReferer[nValueLen] = 0;
			}
			
		}

		return true;
	}
	static inline bool decode_accept_encoding(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_expect(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_transfer_encoding(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		if( 0 == strncasecmp(pLine, "Transfer-Encoding:", 18))
		{
			pLine += 18;
			while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

			if( 0 == strncasecmp(pLine, "chunked", 7)) 
			{
				oInstance.m_bHasContentLength = false;
				oInstance.m_bHasChunked = true;
			} 
		}
		return true;
	}

	static inline bool decode_qvia(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
		// if( 0 == strncasecmp(pLine, "Qvia:", 5))
		// {
			// pLine += 5;
			// while(*pLine==' '||*pLine=='\t'||*pLine=='\r') pLine++;

		// }

		return true;
	}
	static inline bool decode_range_if(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	static inline bool decode_range(CHttpParser& oInstance, const char* pLine, const char* pLineEnd)
	{
		return false;
	}
	
};

};

#endif

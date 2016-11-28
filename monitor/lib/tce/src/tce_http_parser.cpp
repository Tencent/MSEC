
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


#include "tce_http_parser.h"
#include <iostream>
using namespace std;

namespace tce{

char CHttpParser::m_arrFirstCharIndex[255]={0};
bool CHttpParser::m_bInitStaticData = false;
CHttpParser::DECODE_FUNC CHttpParser::m_arrTagFuncs[TAG_WIDTH*TAG_LENGHT] = {0};

CHttpParser::CHttpParser(void)
{
	memset(m_szNULL, 0, sizeof(m_szNULL));
	if ( !m_bInitStaticData ){
		m_bInitStaticData = true;
		m_arrFirstCharIndex[(int32_t)'c'] = 1;
		m_arrFirstCharIndex[(int32_t)'C'] = 1;
		m_arrFirstCharIndex[(int32_t)'h'] = 2;
		m_arrFirstCharIndex[(int32_t)'H'] = 2;
		m_arrFirstCharIndex[(int32_t)'i'] = 3;
		m_arrFirstCharIndex[(int32_t)'I'] = 3;
		m_arrFirstCharIndex[(int32_t)'k'] = 4;
		m_arrFirstCharIndex[(int32_t)'K'] = 4;
		m_arrFirstCharIndex[(int32_t)'r'] = 5;
		m_arrFirstCharIndex[(int32_t)'R'] = 5;
		m_arrFirstCharIndex[(int32_t)'a'] = 6;
		m_arrFirstCharIndex[(int32_t)'A'] = 6;
		m_arrFirstCharIndex[(int32_t)'e'] = 7;
		m_arrFirstCharIndex[(int32_t)'E'] = 7;
		m_arrFirstCharIndex[(int32_t)'t'] = 8;
		m_arrFirstCharIndex[(int32_t)'T'] = 8;
		m_arrFirstCharIndex[(int32_t)'q'] = 9;
		m_arrFirstCharIndex[(int32_t)'Q'] = 9;
		m_arrFirstCharIndex[(int32_t)'u'] = 10;
		m_arrFirstCharIndex[(int32_t)'U'] = 10;

		m_arrTagFuncs[10*TAG_WIDTH+1] = &decode_connection;
		m_arrTagFuncs[12*TAG_WIDTH+1] = &decode_content_type;
		m_arrTagFuncs[14*TAG_WIDTH+1] = &decode_content_length;
		m_arrTagFuncs[6*TAG_WIDTH+1] = &decode_cookie;
		m_arrTagFuncs[4*TAG_WIDTH+2] = &decode_host;
		m_arrTagFuncs[17*TAG_WIDTH+3] = &decode_if_modified_since;
		m_arrTagFuncs[19*TAG_WIDTH+3] = &decode_if_unmodified_since;
		m_arrTagFuncs[8*TAG_WIDTH+3] = &decode_if_range;
		m_arrTagFuncs[8*TAG_WIDTH+5] = &decode_range_if;
		m_arrTagFuncs[5*TAG_WIDTH+5] = &decode_range;
		m_arrTagFuncs[14*TAG_WIDTH+5] = &decode_request_range;
		m_arrTagFuncs[7*TAG_WIDTH+5] = &decode_referer;
		m_arrTagFuncs[15*TAG_WIDTH+6] = &decode_accept_language;
		m_arrTagFuncs[6*TAG_WIDTH+7] = &decode_expect;
		m_arrTagFuncs[17*TAG_WIDTH+8] = &decode_transfer_encoding;
		m_arrTagFuncs[4*TAG_WIDTH+9] = &decode_qvia;
		m_arrTagFuncs[10*TAG_WIDTH+10] = &decode_user_agent;
	}

	this->Clear();	
}

CHttpParser::~CHttpParser(void)
{

}

bool CHttpParser::DecodeValues()
{
	m_bDecodeValueFlag = true;
	
	string::size_type pos1 = 0;
	string::size_type pos2 = 0;
	string::size_type equalPos = 0;
	string sTmp;

	if ( m_nMethod == METHOD_GET )
	{	
		char* pTmp = strchr(m_szURIByAll, '?');
		if ( NULL != pTmp )
		{
			m_sValues = pTmp;
		}
	}		
	if (m_sValues.empty())
		return true;	
		
	while ( pos2 != string::npos )
	{
		pos2 = m_sValues.find('&',pos1);
		sTmp = m_sValues.substr(pos1, pos2-pos1);
		pos1 = pos2+1;
		equalPos = sTmp.find('=');
		if (equalPos == string::npos)
			continue;
		m_mapValues[sTmp.substr(0, equalPos)] = tce::FormUrlDecode(sTmp.substr(1+equalPos));
	}	
	
	return true;
}

bool CHttpParser::DecodeCookies()
{
	m_bDecodeCookieFlag = true;
	
	std::string::size_type pos1 = 0;
	std::string::size_type pos2 = 0;
	std::string::size_type equalPos = 0;
	std::string sTmp;
	std::string sCookies = m_szCookie;
	if (sCookies.empty())
		return true;
		
	while ( pos2 != string::npos )
	{
		pos2 = sCookies.find(';',pos1);

		sTmp = sCookies.substr(pos1, pos2-pos1);
		pos1 = pos2+1;
		equalPos = sTmp.find('=');
		if (equalPos == string::npos)
			continue;

		// skip leading whitespace - " \f\n\r\t\v"
		std::string::size_type wscount = 0;
		std::string::const_iterator data_iter;
		for(data_iter = sTmp.begin(); data_iter != sTmp.end(); ++data_iter,++wscount)
			if(isspace(*data_iter) == 0)
				break;			

		m_mapCookies[sTmp.substr(wscount, equalPos - wscount)] = tce::FormUrlDecode(sTmp.substr(1+equalPos));
	}
	
	return true;
}

bool CHttpParser::Decode(const char* pszData, const size_t nSize)
{
	bool bOk = false;
	if(*(uint32_t *)pszData == *(uint32_t *)"HTTP" )
		bOk = DecodeResponse(pszData, nSize);
	else
		bOk = DecodeRequest(pszData, nSize);
		
	return bOk;
}

bool CHttpParser::DecodeRequest(const char* pszData, const size_t nSize)
{
	m_bRequest = true;
	//method
	const char* pLine = pszData;
	if( 0 == strncmp(pLine, "GET ", 4) ) {
		pLine += 4;
		m_nMethod = METHOD_GET;
	} else if( 0 == strncmp(pLine, "HEAD ", 5) ) {
		pLine += 5;
		m_nMethod = METHOD_HEAD;
	} else if( 0 == strncmp(pLine, "POST ", 5) ) {
		pLine += 5;
		m_nMethod = METHOD_POST;
	} else {
		m_nStatusCode = 501;
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest error: no suppert method.");
		return false;
	}
	if( pLine[0] != '/' ) {
		m_nStatusCode = 400;
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest error: 400, '/' error.");
		return false;
	}

	//Http°æ±¾
	int32_t n = strcspn(pLine, " \t\r");
	const char* pVersion = pLine+n;
	if(*pVersion=='\0') 
	{
		m_bNoHeaderFlag = true;
	} 
	else 
	{
		while(*pVersion==' '||*pVersion=='\t'||*pVersion=='\r') pVersion++;

		if(pVersion[0]=='\0')
		{
			m_bNoHeaderFlag = true;
		}
		else if(*(uint32_t *)pVersion != *(uint32_t *)"HTTP" ||
			pVersion[4] != '/' || !isdigit(pVersion[5]) ||
			pVersion[6] != '.' || !isdigit(pVersion[7]) || pVersion[8] != '\r') 
		{
			m_nStatusCode = 400;
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest error: version error.");
			return false;
		}

		if(pVersion[5]>'1' || (pVersion[5]=='1' && pVersion[7]>='1'))
		{	
			m_bHttpV11 = true;
			m_bKeepAlive = true;
		}
		else
		{
			m_bHttpV11 = false;
			m_bKeepAlive = false;
		}
		
	}

	//uri
	n = strcspn(pLine, " \t\r");
	if( n >= HTTP_MAX_URI_LEN ) {
		m_nStatusCode = 400;
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest error: uri<%lu> over max len.", n);
		return false;
	}
	memcpy(m_szURIByAll, pLine, n);
	m_szURIByAll[n] = '\0';
	n = strcspn(m_szURIByAll, "?#");
	if ( n < HTTP_MAX_URI_LEN )
	{
		memcpy(m_szURI, m_szURIByAll, n);
		m_szURI[n] = '\0';
	}

	char* pLineEnd = NULL;
	char* pTmpLineEnd = NULL;
	pLineEnd = (char*)strchr(pLine, '\n');
	if ( NULL == pLineEnd )
	{
		m_nStatusCode = 400;
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest error: no complete line.");
		return false;
	}
	pLine = pLineEnd + 1;

	char* pTagEnd = NULL;
	m_ucOtherHeadCnt = 0;
	while ( true )
	{
		pLineEnd = (char*)strchr(pLine, '\n');
		if ( NULL == pLineEnd )
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest parse line error:can't find '\\n',line=%s.", pLine);
			m_nStatusCode = 400;
			return false;
		}
		if ( pLine[0] == '\r' && pLine[1] == '\n' )
		{
			//head end
			break;
		}

		const char* pValueStart = NULL;
		bool bOtherHead = true;
		/* parse tags */
		if( (pTagEnd = (char*)strchr(pLine, ':')) != NULL && pTagEnd < pLineEnd ) 
		{
			if ( pLineEnd[-1]=='\r') 
				pTmpLineEnd = pLineEnd-1; 
			else
				pTmpLineEnd = pLineEnd;

			n = pTagEnd - pLine;
			DECODE_FUNC pFun = m_arrTagFuncs[n*TAG_WIDTH + m_arrFirstCharIndex[*(unsigned char *)pLine]];
			if ( NULL != pFun )
			{
				bOtherHead = !(*pFun)(*this, pLine, pTmpLineEnd);
			}

			if ( bOtherHead && m_ucOtherHeadCnt<HTTP_OTHER_HEAD_CNT)
			{
				
				//copy name
				if ( n < HTTP_OTHER_HEAD_NAME_LEN )
				{
					memcpy(m_arrOtherHeadName[m_ucOtherHeadCnt], pLine, n);
					m_arrOtherHeadName[m_ucOtherHeadCnt][n] = 0;
				}
				else
				{
					memcpy(m_arrOtherHeadName[m_ucOtherHeadCnt], pTagEnd+1, HTTP_OTHER_HEAD_NAME_LEN);
					m_arrOtherHeadName[m_ucOtherHeadCnt][HTTP_OTHER_HEAD_NAME_LEN-1] = 0;
				}

				//copy value
				pValueStart = pTagEnd+1;
				while(*pValueStart==' '||*pValueStart=='\t'||*pValueStart=='\r') pValueStart++;

				int nCnt = pTmpLineEnd - pValueStart;
				if ( nCnt > HTTP_OTHER_HEAD_DATA_LEN )
				{
					memcpy(m_arrOtherHeadValue[m_ucOtherHeadCnt], pValueStart, HTTP_OTHER_HEAD_DATA_LEN);
					m_arrOtherHeadValue[m_ucOtherHeadCnt][HTTP_OTHER_HEAD_DATA_LEN-1] = 0;
				}
				else if ( nCnt >= 0 )
				{
					memcpy(m_arrOtherHeadValue[m_ucOtherHeadCnt], pValueStart, nCnt);
					m_arrOtherHeadValue[m_ucOtherHeadCnt][nCnt] = 0;
				}
				++m_ucOtherHeadCnt;
			}
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeRequest parse tags error:%s.", pLine);
			m_nStatusCode = 400;
			return false;
		}

		pLine = pLineEnd + 1;
	}
	m_bHeadFormatOk = true;
	m_nHeadDataSize = pLine - pszData + 2;
	assert(m_nHeadDataSize <= (size_t)nSize);
	
	if ( METHOD_POST == m_nMethod ) 
		m_sValues.assign(pLine+2, nSize-m_nHeadDataSize);

	return true;
}

bool CHttpParser::DecodeResponse(const char* pszData, const size_t nSize)
{
	m_bRequest = false;
	const char* pLine = pszData;
	if(*(uint32_t *)pLine != *(uint32_t *)"HTTP" ||
		pLine[4] != '/' || !isdigit(pLine[5]) ||
		pLine[6] != '.' || !isdigit(pLine[7]) || pLine[8] != ' ') 
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeResponse error: version error, %s.", string(pLine, 20).c_str());
		return false;
	}

	//http version
	if(pLine[5]>'1' || (pLine[5]=='1' && pLine[7]>='1'))
	{	
		m_bHttpV11 = true;
		m_bKeepAlive = true;
	}
	else
	{
		m_bKeepAlive = false;
		m_bHttpV11 = false;
	}

	pLine += 9;
	//http code
	if ( (pLine[0] != '1' && pLine[0] != '2' && pLine[0] != '3' && pLine[0] != '4' && pLine[0] != '5') || !isdigit(pLine[1]) || !isdigit(pLine[2]) || isdigit(pLine[3]) )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeResponse error: httpcode error, %s.", string(pLine, 5).c_str());
		return false;
	}
	m_nStatusCode = atoi(pLine);

	char* pLineEnd = NULL;
	char* pTmpLineEnd = NULL;
	pLineEnd = (char*)strchr(pLine, '\n');
	if ( NULL == pLineEnd )
	{
		tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeResponse error: no complete line, %s.", pszData);
		return false;
	}
	pLine = pLineEnd + 1;

	char* pTagEnd = NULL;
	m_ucOtherHeadCnt = 0;
	int32_t n=0;
	while ( true )
	{
		pLineEnd = (char*)strchr(pLine, '\n');
		if ( NULL == pLineEnd )
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeResponse parse line error:can't find '\\n',line=%s.", pLine);
			return false;
		}
		if ( pLine[0] == '\r' && pLine[1] == '\n' )
		{
			//head end
			break;
		}

		const char* pValueStart = NULL;
		bool bOtherHead = true;
		/* parse tags */
		if( (pTagEnd = (char*)strchr(pLine, ':')) != NULL && pTagEnd < pLineEnd ) 
		{
			if ( pLineEnd[-1]=='\r') 
				pTmpLineEnd = pLineEnd-1; 
			else
				pTmpLineEnd = pLineEnd;

			n = pTagEnd - pLine;
			DECODE_FUNC pFun = m_arrTagFuncs[n*TAG_WIDTH + m_arrFirstCharIndex[*(unsigned char *)pLine]];
			if ( NULL != pFun )
			{
				bOtherHead = !(*pFun)(*this, pLine, pTmpLineEnd);
			}

			if ( bOtherHead && m_ucOtherHeadCnt < HTTP_OTHER_HEAD_CNT )
			{
				//copy name
				if ( n < HTTP_OTHER_HEAD_NAME_LEN )
				{
					memcpy(m_arrOtherHeadName[m_ucOtherHeadCnt], pLine, n);
					m_arrOtherHeadName[m_ucOtherHeadCnt][n] = 0;
				}
				else
				{
					memcpy(m_arrOtherHeadName[m_ucOtherHeadCnt], pTagEnd+1, HTTP_OTHER_HEAD_NAME_LEN);
					m_arrOtherHeadName[m_ucOtherHeadCnt][HTTP_OTHER_HEAD_NAME_LEN-1] = 0;
				}

				//copy value
				pValueStart = pTagEnd+1;
				while(*pValueStart==' '||*pValueStart=='\t'||*pValueStart=='\r') pValueStart++;
				int32_t nCnt = pTmpLineEnd - pValueStart;
				if ( nCnt > HTTP_OTHER_HEAD_DATA_LEN )
				{
					memcpy(m_arrOtherHeadValue[m_ucOtherHeadCnt], pValueStart, HTTP_OTHER_HEAD_DATA_LEN);
					m_arrOtherHeadValue[m_ucOtherHeadCnt][HTTP_OTHER_HEAD_DATA_LEN-1] = 0;
				}
				else if ( nCnt >= 0 )
				{
					memcpy(m_arrOtherHeadValue[m_ucOtherHeadCnt], pValueStart, nCnt);
					m_arrOtherHeadValue[m_ucOtherHeadCnt][nCnt] = 0;
				}
				++m_ucOtherHeadCnt;
			}
		}
		else
		{
			tce::xsnprintf(m_szErrMsg, sizeof(m_szErrMsg), "AnalyzeResponse parse tags error:%s.", pLine);
			m_nStatusCode = 400;
			return false;
		}

		pLine = pLineEnd + 1;
	}

	m_nHeadDataSize = pLine - pszData + 2;
	assert(m_nHeadDataSize <= nSize);
	m_bHeadFormatOk = true;
	return true;
}




};

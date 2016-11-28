
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


#ifndef __HTTPRESPONSE_H__
#define __HTTPRESPONSE_H__

#include <map>
#include <string>
#include <sstream>
#include "tce_utils.h"
#include <map>

using std::string;

namespace tce{


/*
	StatusCode
      o 1xx: Informational - Not used, but reserved for future use

      o 2xx: Success - The action was successfully received,
             understood, and accepted.

      o 3xx: Redirection - Further action must be taken in order to
             complete the request

      o 4xx: Client Error - The request contains bad syntax or cannot
             be fulfilled

      o 5xx: Server Error - The server failed to fulfill an apparently
             valid request
*/

struct SSession;

class CHttpResponse
{
	typedef CHttpResponse this_type;
	struct SCookieInfo{
		SCookieInfo(){}
		string sValue;
		string sDomain;
		string sPath;
		time_t dwCookieTime;
		SCookieInfo& operator=(const SCookieInfo& rhs){
			if (&rhs != this)
				assign(rhs);

			return *this;
		}
		SCookieInfo(const SCookieInfo& rhs)	{	assign(rhs);	}
	private:
		void assign(const SCookieInfo& rhs){
			sValue = rhs.sValue;
			sDomain = rhs.sDomain;
			sPath = rhs.sPath;
			dwCookieTime = rhs.dwCookieTime;
		}
	};
	typedef std::map<string, SCookieInfo> MAP_COOKIE;

public:
	CHttpResponse(void)
	{
		m_dwSetBodyLen = 0;
		m_dwLastModified = 0;
	}
	~CHttpResponse(void)
	{
	}

	inline void Begin();

	template<typename T>
	void SetCookie(const string& sName, const T& tValue,const string& sDomain="",const string& sPath="",const time_t dwCookieTime=0)
	{
		std::stringstream sstr;
		sstr << tValue;
		SCookieInfo stCookieInfo;
		stCookieInfo.dwCookieTime = dwCookieTime;
		stCookieInfo.sDomain = sDomain;
		stCookieInfo.sPath = sPath;
		stCookieInfo.sValue = sstr.str();
		m_mapCookie.insert(MAP_COOKIE::value_type(sName,stCookieInfo));
	}
	inline void SetCookie(const string& sName,const string& sValue,const string& sDomain="",const string& sPath="",const time_t dwCookieTime=0);
	void SetStatusCode(const int iStatus) {	m_iStatusCode = iStatus;	}
	void SetConnection(const string& sConn="close")	{	m_sConnection = sConn; }
	void SetContentType(const string& sData="text/html"){	m_sContentType=sData;	}
	void SetCacheControl(const string& sCacheCtl) {	m_sCacheControl = sCacheCtl;	}
	void SetBodyLen(const unsigned long dwLen) { m_dwSetBodyLen = dwLen;	}
	void SetLocation(const string& sLocation) {	m_sLocation = sLocation;	}
	void SetLastModified(const time_t dwTime) {	m_dwLastModified = dwTime;	}
	void SetExpires(const time_t dwTime)	{	m_dwExpiresTime = dwTime;	}
	void SetETag(const std::string& sETag)	{	m_sETag = sETag;	}
	inline void End();

	const char* data() const {	return m_sSendData.c_str();	}
	size_t size() const {	return static_cast<unsigned long>(m_sSendData.size());	}
	const char* GetData() const {	return m_sSendData.c_str();	}
	int GetDataLen() const {	return static_cast<unsigned long>(m_sSendData.size());	}

	template<typename TVal>
	this_type& operator << (const TVal& tVal){
		std::stringstream sstr;
		sstr << tVal;
		m_sBodyContent.append(sstr.str());
		return *this;
	}

	void Write(const char* pData, const size_t dwSize){
		m_sBodyContent.append(pData, dwSize);
	}

	this_type& operator << (const char* pszVal){
		m_sBodyContent.append(pszVal);
		return *this;
	}

	this_type& operator << (const string& sVal){
		m_sBodyContent += sVal;
		return * this;
	}

	bool EndAndSend(SSession& stSession);

private:
	MAP_COOKIE m_mapCookie;
	string m_sBodyContent;
	string m_sSendData;
	int m_iStatusCode;
	string m_sConnection;
	string m_sContentType;
	string m_sCacheControl;
	string m_sLocation;
	time_t m_dwLastModified;
	time_t m_dwExpiresTime;
	std::string m_sETag;
	unsigned long m_dwSetBodyLen;
};



 void CHttpResponse::SetCookie(const string& sName,const string& sValue,const string& sDomain,const string& sPath,const time_t dwCookieTime)
 {

	SCookieInfo stCookieInfo;
	stCookieInfo.dwCookieTime = dwCookieTime;
	stCookieInfo.sDomain = sDomain;
	stCookieInfo.sPath = sPath;
	stCookieInfo.sValue = sValue;
	m_mapCookie.insert(MAP_COOKIE::value_type(sName,stCookieInfo));
 }



void CHttpResponse::End()
{
	char szTmp[1024]={0};
	m_sSendData.erase();

	//
	xsnprintf(szTmp, sizeof(szTmp), "HTTP/1.1 %d OK\r\n", m_iStatusCode);
	m_sSendData = szTmp;

//	sprintf(szTmp, "Keep-Alive: timeout=300000, max=10000000\r\n");
//	m_sSendData += szTmp;

	xsnprintf(szTmp, sizeof(szTmp),"Connection: %s\r\n", m_sConnection.c_str());
	m_sSendData += szTmp;

	//Date: Wed, 22 Mar 2006 07:47:52 GMT\r\n
	xsnprintf(szTmp, sizeof(szTmp), "Date: %s\r\n", getGMTDate(time(NULL)).c_str());
	m_sSendData += szTmp;

	if (m_dwLastModified > 0)
	{
		xsnprintf(szTmp, sizeof(szTmp), "Last-Modified: %s\r\n", getGMTDate(m_dwLastModified).c_str());
		m_sSendData += szTmp;
	}

	if ( m_dwExpiresTime > 0 )
	{
		xsnprintf(szTmp, sizeof(szTmp), "Expires: %s\r\n", getGMTDate(m_dwExpiresTime).c_str());
		m_sSendData += szTmp;
	}

	//Server: Apache/1.3.29 (Unix)\r\n
	//	m_sSendData += "Server: qqlive web server1.0\r\n";

	if (!m_sCacheControl.empty())
	{
		xsnprintf(szTmp, sizeof(szTmp),"Cache-Control: %s\r\n", m_sCacheControl.c_str());
		m_sSendData += szTmp;
	}

	if ( !m_sETag.empty() )
	{
		xsnprintf(szTmp, sizeof(szTmp),"ETag: %s\r\n", m_sETag.c_str());
		m_sSendData += szTmp;
	}

	//length
	if (m_dwSetBodyLen != 0)
	{
		xsnprintf(szTmp, sizeof(szTmp), "Content-Length: %lu\r\n", m_dwSetBodyLen);
		m_sSendData += szTmp;
	}
	else
	{
		if ( !m_sBodyContent.empty() )
		{
			xsnprintf(szTmp, sizeof(szTmp), "Content-Length: %d\r\n", m_sBodyContent.size());
			m_sSendData += szTmp;
		}
	}	
	//set cookie
	for (MAP_COOKIE::const_iterator it=m_mapCookie.begin(); it!=m_mapCookie.end(); ++it)
	{
		m_sSendData += "Set-Cookie:"  + it->first + "=" + it->second.sValue;
			if ( !it->second.sDomain.empty() )
				m_sSendData += "; Domain=" + it->second.sDomain;
			if ( it->second.dwCookieTime != 0 )
				m_sSendData += "; Expires=" + getGMTDate(it->second.dwCookieTime+time(NULL));
			if ( !it->second.sPath.empty() )
				m_sSendData += "; Path=" + it->second.sPath;
			m_sSendData += "\r\n";
	}

	if ( !m_sLocation.empty() )
	{
		m_sSendData += "Location:" + m_sLocation + "\r\n";
	}

	//html head end
	xsnprintf(szTmp, sizeof(szTmp), "Content-Type: %s\r\n\r\n", m_sContentType.c_str());
	m_sSendData += szTmp;
	m_sSendData += m_sBodyContent;
}

void CHttpResponse::Begin()
{
	m_mapCookie.clear();
	m_sBodyContent.erase();
	m_iStatusCode = 200;
	m_sConnection = "close";
	m_sContentType = "text/html";
	m_dwSetBodyLen = 0;
	m_sCacheControl.erase();
	m_sLocation.erase();
	m_dwLastModified = 0;
	m_sETag = "";
	m_dwExpiresTime = 0;
}

};

#endif



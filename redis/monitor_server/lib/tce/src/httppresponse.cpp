
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


#include "httpresponse.h"
#include "tce_comm_mgr.h"

namespace tce{

bool CHttpResponse::EndAndSend(SSession& stSession)
{
	//	 m_sSendData.erase();
	char szTmp[1024]={0};
	std::string sHead;

	//Date: Wed, 22 Mar 2006 07:47:52 GMT\r\n
	xsnprintf(szTmp, sizeof(szTmp), "HTTP/1.1 %d OK\r\nConnection: %s\r\nDate: %s\r\n", m_iStatusCode,m_sConnection.c_str(),getGMTDate(time(NULL)).c_str());
	sHead = szTmp;

	if (m_dwLastModified > 0)
	{
		xsnprintf(szTmp, sizeof(szTmp), "Last-Modified: %s\r\n", getGMTDate(m_dwLastModified).c_str());
		sHead += szTmp;
	}

	if ( m_dwExpiresTime > 0 )
	{
		xsnprintf(szTmp, sizeof(szTmp), "Expires: %s\r\n", getGMTDate(m_dwExpiresTime).c_str());
		sHead += szTmp;
	}

	if (!m_sCacheControl.empty())
	{
		xsnprintf(szTmp, sizeof(szTmp),"Cache-Control: %s\r\n", m_sCacheControl.c_str());
		sHead += szTmp;
	}

	if ( !m_sETag.empty() )
	{
		xsnprintf(szTmp, sizeof(szTmp),"ETag: %s\r\n", m_sETag.c_str());
		sHead += szTmp;
	}

	//length
	if (m_dwSetBodyLen != 0)
	{
		xsnprintf(szTmp, sizeof(szTmp), "Content-Length: %lu\r\n", m_dwSetBodyLen);
		sHead += szTmp;
	}
	else
	{
		if ( !m_sBodyContent.empty() )
		{
			xsnprintf(szTmp, sizeof(szTmp), "Content-Length: %d\r\n", m_sBodyContent.size());
			sHead += szTmp;
		}
	}	
	//set cookie
	for (MAP_COOKIE::const_iterator it=m_mapCookie.begin(); it!=m_mapCookie.end(); ++it)
	{
		sHead += "Set-Cookie:"  + it->first + "=" + it->second.sValue;
		if ( !it->second.sDomain.empty() )
			sHead += "; Domain=" + it->second.sDomain;
		if ( it->second.dwCookieTime != 0 )
			sHead += "; Expires=" + getGMTDate(it->second.dwCookieTime+time(NULL));
		if ( !it->second.sPath.empty() )
			sHead += "; Path=" + it->second.sPath;
		sHead += "\r\n";
	}

	if ( !m_sLocation.empty() )
	{
		sHead += "Location:" + m_sLocation + "\r\n";
	}

	//html head end
	xsnprintf(szTmp, sizeof(szTmp), "Content-Type: %s\r\n\r\n", m_sContentType.c_str());
	sHead += szTmp;

	return CCommMgr::GetInstance().Write(stSession, sHead.data(), sHead.size(), m_sBodyContent.data(), m_sBodyContent.size());
}

}


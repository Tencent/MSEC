
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


#include "file_log.h"


namespace tce{


CFileLog::CFileLog(const std::string& sLogFilePath,const uint32_t dwLogFileMaxSize, const uint32_t uiLogFileNum, const bool bShowCmd)
	:m_sLogFilePath(sLogFilePath+".log"),
	m_sLogBasePath(sLogFilePath),
	m_dwLogFileMaxSize(dwLogFileMaxSize),
	m_uiLogFileNum(uiLogFileNum),
	m_bShowCmd(bShowCmd)
{

}

CFileLog::~CFileLog()
{
	if (m_ofsOutFile.is_open())
	{
		m_ofsOutFile.close();
	}
}

bool CFileLog::Init(const std::string &sLogFilePath, const uint32_t dwLogFileMaxSize, const uint32_t uiLogFileNum, const bool bShowCmd)
{
	
	m_sLogFilePath=sLogFilePath+".log",
	m_sLogBasePath = sLogFilePath;
	m_dwLogFileMaxSize = dwLogFileMaxSize;
	m_uiLogFileNum = uiLogFileNum;
	m_bShowCmd = bShowCmd;

	return true;
}

bool CFileLog::ShiftFiles()
{
	size_t dwFileSize=this->GetFileSize(m_sLogFilePath);
	if (dwFileSize>=m_dwLogFileMaxSize)
	{
		if (m_ofsOutFile.is_open()) 
		{
			m_ofsOutFile.close();
		}

		char szLogFileName[1024];
		char szNewLogFileName[1024];

		xsnprintf(szLogFileName,sizeof(szLogFileName),"%s%u.log",m_sLogBasePath.c_str(),m_uiLogFileNum-1);
		if (access(szLogFileName, F_OK) == 0)
		{
			if (remove(szLogFileName) < 0 )
			{
				xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"remove error: errno=%d.",errno);
				return false;
			}
		}

		for(int32_t i=m_uiLogFileNum-2; i>=0; i--)
		{
			if (i == 0)
				xsnprintf(szLogFileName,sizeof(szLogFileName),"%s.log",m_sLogBasePath.c_str());
			else
				xsnprintf(szLogFileName,sizeof(szLogFileName),"%s%u.log",m_sLogBasePath.c_str(),i);
			
			if (access(szLogFileName, F_OK) == 0)
			{
				xsnprintf(szNewLogFileName,sizeof(szNewLogFileName),"%s%d.log",m_sLogBasePath.c_str(),i+1);
				if (rename(szLogFileName,szNewLogFileName) < 0 )
				{
					xsnprintf(m_szErrMsg, sizeof(m_szErrMsg),"rename error:errno=%d.",errno);
					return false;
				}
			}
		}
	}
	
	return true;
}

bool CFileLog::WriteFile(const std::string &str, const bool bEnd)
{

	if (m_bShowCmd)
	{
		std::cout << str << std::endl;
	}

	if (!m_ofsOutFile.is_open())
	{
		m_ofsOutFile.open(m_sLogFilePath.c_str(),std::ios::out|std::ios::app);
	}

	if(m_ofsOutFile.fail())
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"open file<%s> err!", m_sLogFilePath.c_str());
		return false;
	}

	if (!ShiftFiles())	
	{
		return false;
	}	

	if (!m_ofsOutFile.is_open())
	{
		m_ofsOutFile.open(m_sLogFilePath.c_str(),std::ios::out|std::ios::app);
	}

	if(m_ofsOutFile.fail())
	{
		xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"open file<%s> err!", m_sLogFilePath.c_str());
		return false;
	}

	m_ofsOutFile << str;
	if (bEnd)
	{
		m_ofsOutFile << std::endl;
	}

	return true;
}



bool CFileLog::Write(const char *sFormat, ...)
{
	
	tce::CAutoLock autoLock(m_mutex);
	char szTemp[10240];
	va_list ap;

	va_start(ap, sFormat);
	memset(szTemp, 0, sizeof(szTemp));
#ifdef WIN32
	_vsnprintf_s(szTemp,sizeof(szTemp)-1,_TRUNCATE,sFormat, ap);
#else
	vsnprintf(szTemp,sizeof(szTemp),sFormat, ap);
#endif
	va_end(ap);
	std::string sLog;
	sLog = "[" + GetDateTime() + "]:";
	sLog += szTemp;

	if(!WriteFile(sLog))
		return false;

		
	return true;
}

bool CFileLog::Write(const std::string& sMsg)
{
	
	tce::CAutoLock autoLock(m_mutex);

	if(!WriteFile("[" + GetDateTime() + "]" + sMsg))
		return false;

	return true;
}

bool CFileLog::WriteRaw(const std::string& sMsg)
{
	
	tce::CAutoLock autoLock(m_mutex);

	if(!WriteFile(sMsg))
		return false;

	return true;
}


};


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


#ifndef __TCE_FILE_LOG_H__
#define __TCE_FILE_LOG_H__

#include "tce_lock.h"
#include <string>
#include <fstream>
#include <sstream>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/types.h>
#include <iostream>
#include <time.h>
#include <iconv.h>

#ifdef WIN32
#include <io.h>
#pragma warning(disable: 4996)
#else
#include<sys/unistd.h>
#endif

#include "tce_singleton.h"
#include "tce_utils.h"
#include <sys/stat.h>

namespace tce{

class CFileLog
	:CNonCopyAble
{
#ifdef WIN32
	enum{
		X_OK=0, 
		F_OK=6,
	};
#endif

public:
	CFileLog(const std::string& sLogFilePath="",
		const uint32_t dwLogFileMaxSize=1000000, 
		const uint32_t uiLogFileNum=5, 
		const bool bShowCmd=false);
	~CFileLog(void);

	bool Init(const std::string& sLogFilePath,
			const uint32_t dwLogFileMaxSize=1000000, 
			const uint32_t uiLogFileNum=5, 
			const bool bShowCmd=false);

	template<class T>
		std::ofstream& operator<<(const T& t)
	{
	
		tce::CAutoLock autoLock(m_mutex);
		std::stringstream sstr;
		sstr << t;
		std::string sLog;
		sLog = "[" + GetDateTime() + "]:";

		WriteFile(sLog+sstr.str(), false);
		return m_ofsOutFile;

	}

	bool Write(const std::string& sMsg);
	bool WriteRaw(const std::string& sMsg);
	bool Write(const char *sFormat, ...);
	const char* GetErrMsg() const {	return m_szErrMsg;	}
private:
	size_t GetFileSize(const std::string& sFile){
		struct stat stStat;
		if (lstat(sFile.c_str(), &stStat) >= 0) 
		{
			return stStat.st_size;
		}
		
		return 0;
	}
	
	std::string GetDateTime(){
		time_t	iCurTime;
		time(&iCurTime);
		struct tm curr;
		curr = *localtime(&iCurTime);
		char szDate[50]; 
		tce::xsnprintf(szDate, sizeof(szDate), "%04d-%02d-%02d %02d:%02d:%02d", curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday, curr.tm_hour, curr.tm_min, curr.tm_sec);
		return std::string(szDate); 
	}

	bool ShiftFiles();
	bool WriteFile(const std::string& str, const bool bEnd=true);
private:
	char m_szErrMsg[1024];
	std::ofstream m_ofsOutFile;
	std::string m_sLogFilePath;
	std::string m_sLogBasePath;
	uint32_t m_dwLogFileMaxSize;
	uint32_t m_uiLogFileNum;
	bool m_bShowCmd;
	tce::CMutex m_mutex;
};


};

#endif 


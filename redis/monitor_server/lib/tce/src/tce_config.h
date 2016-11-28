
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


#ifndef __TCE_CONFIG_H__
#define __TCE_CONFIG_H__

#include <string>
#include <map>
#include <vector>
#include "tce_utils.h"
#include "tce_value.h"

namespace tce{

class CConfig
{
	typedef std::map<std::string, std::string> CONFIG_VALUE;
	typedef std::map<std::string, CONFIG_VALUE > MAPCONFIG;
public:
	CConfig(void);
	~CConfig(void);

	bool LoadConfig(const std::string& sCfgFileName);
	
	//new接口
	inline tce::CValue GetValue(const std::string& sApp, const std::string& sName, const tce::CValue& vDefValue="")
	{
		MAPCONFIG::iterator it = m_mapConfig.find(sApp);
		if (it != m_mapConfig.end())
		{
			CONFIG_VALUE::iterator subIt = it->second.find(sName);
			if (subIt != it->second.end())
			{
				return tce::CValue(subIt->second);
			}
		}
		return vDefValue;
	}
	inline bool Has(const std::string& sApp, const std::string& sName)
	{
		MAPCONFIG::iterator it = m_mapConfig.find(sApp);
		if (it != m_mapConfig.end())
		{
			CONFIG_VALUE::iterator subIt = it->second.find(sName);
			if (subIt != it->second.end())
			{
				return true;
			}
		}
		return false;
	}

		
	//old接口，不建议使用
	std::string& GetValue(const std::string& sApp, const std::string& sName, std::string& sValue, const std::string& sDefault = "");
	int GetValue(const std::string& sApp, const std::string& sName,int& iValue, const int iDefault = 0);
	unsigned int GetValue(const std::string& sApp, const std::string& sName,unsigned int& uiValue, const unsigned int uiDefault = 0);
	long GetValue(const std::string& sApp, const std::string& sName,long& lValue, const long lDefault = 0);
	unsigned short GetValue(const std::string& sApp, const std::string& sName,unsigned short& wValue, const unsigned short wDefault=0);
	unsigned long GetValue(const std::string& sApp, const std::string& sName,unsigned long& dwValue, const unsigned long dwDefault=0);
	bool GetValue(const std::string& sApp, const std::string& sName,bool& bValue, const bool bDefault = false );

	const char* GetErrMsg() const {	return m_szErrMsg;	}
private:
	CConfig(const CConfig& rhs);
	CConfig& operator=(const CConfig& rhs);
private:
	std::string m_sCfgFileName;
	char m_szErrMsg[1024];
	MAPCONFIG m_mapConfig;
};

};
#endif


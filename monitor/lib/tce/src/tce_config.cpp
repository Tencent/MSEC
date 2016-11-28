
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


#include "tce_config.h"
#include <errno.h>
#include <fstream>


namespace tce{

CConfig::CConfig(void)
{
}

CConfig::~CConfig(void)
{
}

bool CConfig::LoadConfig(const std::string& sCfgFileName)
{
	std::ifstream ifsConfig;
	ifsConfig.open(sCfgFileName.c_str());
	if (ifsConfig.fail())
	{
		tce::xsnprintf(m_szErrMsg,sizeof(m_szErrMsg),"errno=%d",errno);
		return false;
	}

	std::string sApp;
	std::string sName;
	std::string sValue;
	std::string sLine;
	while (getline(ifsConfig,sLine))
	{
		if (sLine.empty())
		{
			continue;
		}

		size_t i = 0;
		for(i = 0; i < sLine.size(); i++)
		{
			if(sLine[i] != ' ' || sLine[i] != '\t') 
			{
				break;
			}
		}

		switch(sLine[i]) {
		case '#':
		case ';':
			break;
		case '[':
			{
				size_t j = sLine.find(']', i);
				if(std::string::npos != j)
				{
					sApp = sLine.substr(i+1, j-i-1);
					TrimString(sApp);
					if (sApp.empty())
					{
						break;
					}
				}
				else
				{
					break;
				}
			}
		default:
			size_t j = sLine.find('=', i);
			if(j > i)
			{
				sName = sLine.substr(i, j-i);

				TrimString(sName);
				TrimString(sApp);

				if(!sName.empty())
				{
					sValue = sLine.substr(j+1);
					m_mapConfig[sApp][sName] = TrimString(sValue);
				}
			}
			break;
		}
	}

	return true;
}

std::string& CConfig::GetValue(const std::string& sApp, const std::string& sName, std::string& sValue, const std::string& sDefault)
{
	sValue = sDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
			sValue = subIt->second;
		}
	}

	return sValue;
}

int CConfig::GetValue(const std::string& sApp, const std::string& sName,int& iValue, const int iDefault)
{
	iValue = iDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
			iValue = atoi(subIt->second.c_str());
		}
	}
	return iValue;
}

long CConfig::GetValue(const std::string& sApp, const std::string& sName,long& lValue, const long lDefault)
{
	lValue = lDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
			lValue =  atol(subIt->second.c_str());
		}
	}
	return lValue;
}



unsigned short CConfig::GetValue(const std::string& sApp, const std::string& sName, unsigned short& wValue,const unsigned short wDefault)
{
	wValue = wDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
			wValue =  static_cast<unsigned short>(atol(subIt->second.c_str()));
		}
	}
	return wValue;
}
unsigned long CConfig::GetValue(const std::string& sApp, const std::string& sName,unsigned long& dwValue,const unsigned long dwDefault)
{
	dwValue = dwDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
#ifdef WIN32
			dwValue = static_cast<unsigned long>(_atoi64(subIt->second.c_str()));
#else
			dwValue = static_cast<unsigned long>(atoll(subIt->second.c_str()));
#endif
		}
	}
	return dwValue;
}

bool CConfig::GetValue(const std::string& sApp, const std::string& sName,bool& bValue, const bool bDefault /* = false */)
{
	bValue = bDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
			bValue =  atol(subIt->second.c_str()) == 0 ? false : true;
		}
	}
	return bValue;
}

unsigned int CConfig::GetValue(const std::string& sApp, const std::string& sName,unsigned int& uiValue, const unsigned int uiDefault)
{
	uiValue = uiDefault;
	MAPCONFIG::iterator it = m_mapConfig.find(sApp);
	if (it != m_mapConfig.end())
	{
		CONFIG_VALUE::iterator subIt = it->second.find(sName);
		if (subIt != it->second.end())
		{
#ifdef WIN32
			uiValue = static_cast<unsigned int>(_atoi64(subIt->second.c_str()));
#else
			uiValue = static_cast<unsigned int>(atoll(subIt->second.c_str()));
#endif
		}
	}
	return uiValue;
}

};


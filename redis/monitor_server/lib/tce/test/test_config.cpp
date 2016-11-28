
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


#include "tce.h"
#include <iostream>

using namespace std;


int main()
{
	tce::CConfig oCfg;
	
	if ( !oCfg.LoadConfig("./test_config.ini") )
	{
		cout << "load config error:" << oCfg.GetErrMsg() << endl;
		return false;
	}
	
	std::string sName1 = oCfg.GetValue("test", "name", "haha");
	std::string sName2 = oCfg.GetValue("test", "name1", "haha");
	int32_t iNum1 = oCfg.GetValue("test", "num1", -1);
	int32_t iNum2 = oCfg.GetValue("test", "num2", -1);
	int32_t iNum3 = oCfg.GetValue("test", "num3");
	uint64_t nUin = oCfg.GetValue("test1", "uin", -1);	
	cout << "sName1=" << sName1 << "; name2=" << sName2 << endl;
	cout << "iNum1=" << iNum1 << "; iNum2=" << iNum2 << ";iNum3=" << iNum3 << endl;
	cout << "nUin=" << nUin << endl;
	cout << oCfg.Has("test", "name") << endl;
	cout << oCfg.Has("test", "name3") << endl;
	return 0;
}

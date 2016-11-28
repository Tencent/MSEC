
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


// test_any_value.cpp : Defines the entry point for the console application.
//
#include <iostream>
#include "tce.h"
#include <time.h>

using namespace std;

struct SHead{
	int i;
};

typedef tce::CAnyValuePackage<SHead> PKG; 

int main(int argc, char* argv[])
{
	try{

		//////////////////////////////////////////////////////////////////////////
		//组包///
		cout << "***0***" << endl;
		PKG stPkg;

		stPkg["g"] = 123456;			//数值
		stPkg["str"] = "group_name";	//字符串
		stPkg["open"] = false;			//bool

		cout << "***1***" << endl;
		//list 1
		for ( int i=0; i<10; ++i )
		{
			stPkg["list1"].push_back(i);
		}

		cout << "******" << endl;
		//list 2
		for ( int i=0; i<10; ++i ) {
			tce::CAnyValue anyItem;
			std::string s="a";
			anyItem[s] = i;
			anyItem["name"] = "test";

			//list里还可以嵌套list
			for( int j=0; j<10; ++j )
			{
				tce::CAnyValue anyItem2;
				anyItem2["id"] = j;
				anyItem2["name"] = "test";
				anyItem["list"].push_back(anyItem2);
			}

			stPkg["list2"].push_back(anyItem);
		}

		//encode 成二进制
		stPkg.encode();

		//or encode 成 xml 
		//stPkg.encodeXML();

		//or encode 成 json
		//stPkg.encodeJSON();

		//std::string sPkgData;
		//sPkgData.assign(stPkg.data(), stPkg.size());


		cout << "****" << endl;
		//解包
		PKG stDecodePkg;
		stDecodePkg.decode(stPkg.data(), stPkg.size());//只支持二进制

		
		unsigned long dwGID = stDecodePkg["g"];
		std::string sStr = stDecodePkg["str"];
		bool bOpen = stDecodePkg["open"];

		tce::CAnyValue& anyList1 = stDecodePkg["list2"];
		cout << "anyList1.size():" << anyList1.size() << endl;
		for ( size_t i=0; i<anyList1.size(); ++i )
		{
			unsigned long dwID = anyList1.row(i)["id"];
			std::string sName = anyList1.row(i)["name"];
			cout << "id=" << dwID << "; name=" << sName << endl;
		}
		
		cout << "end" << endl;

	}catch(const tce::CAnyValuePackage<SHead>::Error e){

		cout << "error:" << e.what() << endl;
	}

	//tce::xsleep(1000000);

	return 0;
}


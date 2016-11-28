
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
#include "any_value_pkg.h"
#include "utils.h"
#include <time.h>
#include <fstream>
#include "tconv_g2u.h"


using namespace std;

size_t GetFileSize(const std::string& sFile){

	//2009-04-09 modified by alex
	struct stat stStat;
	if (stat(sFile.c_str(), &stStat) >= 0) 
	{
		return stStat.st_size;
	}

	return 0;
}

bool ReadFile(const string& sFilePath, const size_t dwFileSize, string& sData)
{
	bool bOk = true;

	sData.resize(dwFileSize);
	FILE* pFile = fopen(sFilePath.c_str(),"r");
	if( NULL != pFile )
	{
		if(fread((char*)sData.data(), 1, dwFileSize,pFile) != dwFileSize)
		{
			bOk = false;
		}
		fclose(pFile);
	}
	else
	{
		bOk = false;
	}

	return bOk;
}

std::string Gbk2Utf8(const std::string &sGbk)
{

	size_t dwUtfLen = 4 * sGbk.length();
	char *sUtf8 = new char[dwUtfLen];
	tconv_gbk2utf8(sGbk.c_str(), sGbk.length(), sUtf8, &dwUtfLen);

	sUtf8[dwUtfLen] = 0;

	string s(sUtf8);
	delete []sUtf8;

	return s;
}

std::string Utf82Gbk(const std::string &sUtf8)
{

	size_t dwGbkLen = sUtf8.length()+1;
	char *sGbk = new char[dwGbkLen];
	tconv_utf82gbk(sUtf8.c_str(), sUtf8.length(), sGbk, &dwGbkLen);
	sGbk[dwGbkLen] = 0;

	string s(sGbk);
	delete []sGbk;

	return s;
}


int main(int argc, char* argv[])
{
	try{

		//////////////////////////////////////////////////////////////////////////
		//×é°ü///

		std::string sData;
		ifstream ifs("./data");
		size_t dwFileSize = GetFileSize("./data");
		ReadFile("./data", dwFileSize, sData);

		cout << "filesize=" << dwFileSize << endl;
		cout << "datasize=" << sData.size() << endl;
		tce::CAnyValueRoot root;
		root.decode(sData.data(), sData.size());

		//for ( int i=0; i<root["ls"].size(); ++i )
		//{
			tce::CAnyValue item = root["ls"][5];
			unsigned long dwUin = item["u"];
			std::string sName = item["qqn"];
			cout << "hexshow=" << tce::HexShow(sName) << endl;
			std::string jsonName = "\""; 
			jsonName += tce::TextEncodeJSON(sName, true);
			jsonName += "\""; 

			cout << "dwUin=" << dwUin << "; name=" << sName << "; jsonName=" << jsonName << endl;
		//}

		std::string sTest = "ÅûÅ£Æ¤µØÀÇ";
		cout << "2 hex=" << tce::HexShow(Gbk2Utf8(sTest)) << endl;

		tce::CAnyValueRoot newroot;
		newroot["u"] = item["u"];
		newroot["qqn"] = Gbk2Utf8(sTest);//item["qqn"];

		newroot.encodeJSON(true);
		cout << newroot.data() << endl;

	}catch(const tce::CAnyValueRoot::Error e){

		cout << "error:" << e.what() << endl;
	}

//	tce::xsleep(1000000);

	return 0;
}


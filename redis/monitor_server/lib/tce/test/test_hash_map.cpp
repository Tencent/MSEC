
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


#include <iostream>
#include <string>
#include "tce.h"

using namespace std;

struct STest;
typedef tce::shm::CHashMap<unsigned long, STest> SHMHASHMAP;
typedef tce::shm::CHashMap<std::string, STest, tce::Int2Type<tce::shm::KT_STRING_64> > STR_SHMHASHMAP;

#pragma pack(1)

struct SMyStrct{
	int iValue;
	char szName[200];
};

struct STest 
{
	SMyStrct stMyStruct;
	unsigned long time;
	int i;
};
#pragma pack()


void printStepRate(SHMHASHMAP& hashmap)
{
	cout << "step using rate list:" << endl;
	for ( unsigned long i=0; i<hashmap.step_count(); ++i )
	{
		cout << "step=" << i << "; usingsize=" << hashmap.step_using_size(i) << endl;
	}
}


void printStrStepRate(STR_SHMHASHMAP& hashmap)
{
	cout << "step using rate list:" << endl;
	for ( unsigned long i=0; i<hashmap.step_count(); ++i )
	{
		cout << "step=" << i << "; usingsize=" << hashmap.step_using_size(i) << endl;
	}
}

//first example 
void TestIntHashMap(){
	SHMHASHMAP hashmap;

	unsigned char ucTmp = 255;
	cout << (int)ucTmp << endl;

	//init
	cout << "init begin..."  << endl;
	if ( !hashmap.init(1234,		/*shm key*/
		1024*1024*100,
		false))	/*shm buffer size*/
	{
		cout << "hashmap.init error=" << hashmap.err_msg() << endl;
		return ;
	}

	unsigned long dwID = 1;
	STest stTest;
	stTest.i = dwID;
	stTest.stMyStruct.iValue = 123;
	sprintf(stTest.stMyStruct.szName, "happy-%lu-%d", dwID, stTest.i);
	if ( !hashmap.insert( SHMHASHMAP::value_type(dwID, stTest) ).second )
	{
//			cout << "insert error:" << hashmap.err_msg() << endl;
	}	

	dwID = 1+565*564;
	stTest.i = dwID;
	stTest.stMyStruct.iValue = 123;
	sprintf(stTest.stMyStruct.szName, "happy-%lu-%d", dwID, stTest.i);
	if ( !hashmap.insert( SHMHASHMAP::value_type(dwID, stTest) ).second )
	{
//			cout << "insert error:" << hashmap.err_msg() << endl;
	}

	dwID = 1+565*564*563;
	stTest.i = dwID;
	stTest.stMyStruct.iValue = 123;
	sprintf(stTest.stMyStruct.szName, "happy-%lu-%d", dwID, stTest.i);
	if ( !hashmap.insert( SHMHASHMAP::value_type(dwID, stTest) ).second )
	{
//			cout << "insert error:" << hashmap.err_msg() << endl;
	}

	//find
	SHMHASHMAP::iterator it=hashmap.find(dwID);
	if ( it != hashmap.end() )
	{
		cout << "find ok" << endl;
		cout << "first=" << it->first << endl;
		cout << "MyStruct.szName=" << it->second.stMyStruct.szName << endl;
	} 
	else
	{
		cout << "no find." << endl;
	}

	cout << "review all begin...size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
	//review all
	for ( SHMHASHMAP::iterator it=hashmap.begin(); it!=hashmap.end(); ++it )
	{
		cout << "first=" << it->first << "; second=(name:" << it->second.stMyStruct.szName << ",i:" << it->second.i << ",value:" << it->second.stMyStruct.iValue << ")" << endl;
	}


//	hashmap.erase(1+565*564);
//	hashmap.erase(1+565*564*563);
//
//	dwID = 1+565*564*563;
//	stTest.i = dwID;
//	stTest.stMyStruct.iValue = 123;
//	sprintf(stTest.stMyStruct.szName, "happy-%d-%d", dwID, stTest.i);
//	if ( !hashmap.insert( SHMHASHMAP::value_type(dwID, stTest) ).second )
//	{
////			cout << "insert error:" << hashmap.err_msg() << endl;
//	}


	printStepRate(hashmap);


	cout << "review all begin...size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
	//review all
	for ( SHMHASHMAP::iterator it=hashmap.begin(); it!=hashmap.end(); ++it )
	{
		cout << "first=" << it->first << "; second=(name:" << it->second.stMyStruct.szName << ",i:" << it->second.i << ",value:" << it->second.stMyStruct.iValue << ")" << endl;
	}

	cout << "init end...:size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
	printStepRate(hashmap);
	//insert 
	for ( int i=0; i<15000; ++i )
	{
//		cout << "insert " << i  << "; size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
		STest stTest;
		stTest.i = 1234+i*10;
		stTest.stMyStruct.iValue = 123;
		sprintf(stTest.stMyStruct.szName, "happy-%d-%d", i, stTest.i);
		if ( !hashmap.insert( SHMHASHMAP::value_type(dwID+i, stTest) ).second )
		{
//			cout << "insert error:" << hashmap.err_msg() << endl;
		}	
	}
	printStepRate(hashmap);

	cout << "insert end...size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;

	////find
	//SHMHASHMAP::iterator it=hashmap.find(13800);
	//if ( it != hashmap.end() )
	//{
	//	cout << "find ok" << endl;
	//	cout << "first=" << it->first << endl;
	//	cout << "MyStruct.szName=" << it->second.stMyStruct.szName << endl;
	//	//modify item
	//	//if ( !it->second.sTitle.assign("haha") )
	//	//{
	//	//	cout << "assgin fail" << endl;
	//	//}
	//} 
	//else
	//{
	//	cout << "no find." << endl;
	//}

	cout << "review all begin...size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
	//review all
	for ( SHMHASHMAP::iterator it=hashmap.begin(); it!=hashmap.end(); ++it )
	{
//		cout << "first=" << it->first << "; second=(name:" << it->second.stMyStruct.szName << ",i:" << it->second.i << ",value:" << it->second.stMyStruct.iValue << ")" << endl;
	}

	cout << "review all end..size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;
	//erase
	hashmap.erase(dwID);
	printStepRate(hashmap);

	cout << "erase end..size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;

	//clear all data
	hashmap.clear();
	printStepRate(hashmap);

	cout << "clear all end..size=" << hashmap.size() << "; maxsize=" << hashmap.max_size()  << endl;

	//print and see memory alloc
//	hashmap.print();

}




void TestStrHashMap(){
	STR_SHMHASHMAP hashmap;

	//init
	if ( !hashmap.init(1235,		/*shm key*/
		1024*1024))	/*shm buffer size*/
	{
		cout << "hashmap.init error=" << hashmap.err_msg() << endl;
		return ;
	}
	cout << "init ok" << endl;

	std::string sKey = "test";
	char szKey[64]={0};
	for ( int i=0; i<1000; ++i )
	{
		snprintf(szKey,sizeof(szKey), "test_%d", i);
		//insert 
		STest stTest;
		stTest.i = 1;
		stTest.stMyStruct.iValue = 123;
		sprintf(stTest.stMyStruct.szName, "happy-%d-%d", 2, stTest.i);
		if ( !hashmap.insert( STR_SHMHASHMAP::value_type(szKey, stTest) ).second )
		{
	//			cout << "insert error:" << hashmap.err_msg() << endl;
		}
	}

	cout << "insert	 ok" << endl;


	//find
	STR_SHMHASHMAP::iterator it=hashmap.find(sKey);
	if ( it != hashmap.end() )
	{
		cout << "find ok" << endl;
		cout << "first=" << it->first.asString() << endl;
		cout << "stMyStruct.iValue=" << it->second.stMyStruct.iValue << endl;
		cout << "stMyStruct.szName=" << it->second.stMyStruct.szName << endl;
	} 
	else
	{
		cout << "no find." << endl;
	}

	cout << "find	 ok" << endl;


	//review all
	cout << "review all:" << endl;
	for ( STR_SHMHASHMAP::iterator it=hashmap.begin(); it!=hashmap.end(); ++it )
	{
		cout << "first=" << it->first.asString() << endl;
	}
	printStrStepRate(hashmap);

	//erase
	hashmap.erase("test");

	//review all
	cout << "review all:" << endl;
	for ( STR_SHMHASHMAP::iterator it=hashmap.begin(); it!=hashmap.end(); ++it )
	{
		cout << "first=" << it->first.asString() << endl;
	}
	printStrStepRate(hashmap);

	//clear all data
	hashmap.clear();
}

int main()
{
//	cout << "*********TestIntHashMap************" << endl;
//	TestIntHashMap();
	cout << "*********TestIntHashMap************" << endl;
	TestStrHashMap();

	return 0;
}


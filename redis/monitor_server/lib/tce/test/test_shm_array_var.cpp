
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


typedef tce::shm::CArrayVar<> SHM_ARRAY;


char* randstr(char* buffer, int len)
{
	char *chars="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
	int     i,chars_len;
	time_t mytime;

	time(&mytime);
	srand(mytime*getpid());
	chars_len= (int)strlen(chars);
	for ( i=0;i<len;i++)
	{
		buffer[i]=chars[ (int)( (float)strlen(chars)*rand()/(RAND_MAX+1.0) ) ];
	}
	buffer[len]='\0';
	return buffer;
}

int main()
{
	SHM_ARRAY shmArray;
	if ( !shmArray.init(123456,	/*shm key*/
				  100*1000,		/*数组大小*/
				  1024*1024,	/*动态分配内存大小*/
				  128,			/*每块大小*/
				  false,		/*是否做crc32校验*/
				  false))		/*是否只读*/
	{
		cout << "init error:" << shmArray.err_msg() << endl;
		return false;
	}

	cout << "total_item_count=" << shmArray.total_item_count() << " ;empty_item_count=" << shmArray.empty_item_count() << "; empty_real_item_count=" << shmArray.empty_real_item_count() << endl;

	unsigned long dwRunCount = 1000000;
	unsigned long dwBeginTime = tce::GetTickCount();
	//set
	time_t mytime;
	time(&mytime);
	srand(mytime*getpid());
	char* szData = new char[1024*1024*10];
	unsigned long dwDataLen = 1*1024;//rand()%102400;//sizeof(szData);
	randstr(szData, dwDataLen);


//	unsigned long dwCRC32 = tce::CRC32(szData, dwDataLen);


	cout << "data_size=" << dwDataLen << endl;
//	for (int i=0; i<dwRunCount; ++i )
	{
		if ( !shmArray.set(rand()%500, szData, dwDataLen) )
		{
			cout << "set error:" << shmArray.err_msg() << endl;
			return 0;
		}	
	}

	cout << "total_item_count=" << shmArray.total_item_count() << " ;empty_item_count=" << shmArray.empty_item_count() << "; empty_real_item_count=" << shmArray.empty_real_item_count() << endl;

	//get
	for (unsigned long i=0; i<dwRunCount; ++i )
	{
		char szResult[100*1024];
		SHM_ARRAY::size_type dwResultLen = sizeof(szResult);
		bool bRes = shmArray.get(rand()%500, szResult, dwResultLen);
//		std::string sResult = shmArray.get(1);
//		unsigned long dwResultCRC32 = tce::CRC32(szResult, dwResultLen);
//		if ( dwCRC32 != dwResultCRC32 )
//		{
//			cout << "crc32 error." << endl;
//		}
//		else
//		{
////			cout << "read data ok" << endl;
//		}	
	}

	unsigned long dwEndTime = tce::GetTickCount();
	cout << "rate=" << 1000*dwRunCount/(dwEndTime-dwBeginTime) << "/s" << endl;

	return 0;
}


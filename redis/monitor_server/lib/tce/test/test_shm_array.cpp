
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

struct STest 
{
	unsigned long dwUin;
	char szName[128];
	char szDesc[1024];
};

typedef tce::shm::CArray<STest> SHM_ARRAY;

int main()
{
	SHM_ARRAY shmArray;
	if ( !shmArray.init(12345,	/*shm key*/
				  100*1000,		/*数组大小*/
				  false))		/*是否只读*/
	{
		cout << "init error:" << shmArray.err_msg() << endl;
		return false;
	}

	//set
	shmArray[1].dwUin = 1234;
	snprintf(shmArray[1].szName, sizeof(shmArray[1].szName), "happy-1");

	//set
	shmArray[999].dwUin = 999;
	snprintf(shmArray[999].szName, sizeof(shmArray[999].szName), "happy-999");

	//get
	cout << "**print 1**" << endl;
	cout << shmArray[1].dwUin << endl;
	cout << shmArray[1].szName << endl;

	cout << "**print 999**" << endl;
	cout << shmArray[999].dwUin << endl;
	cout << shmArray[999].szName << endl;

	return 0;
}



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

typedef tce::shm::CStruct<STest> TEST_STRUCT;

int main()
{
	TEST_STRUCT stTest;
	if ( !stTest.init(12345))	/*shm key*/
	{
		cout << "init error." << endl;
	}

	//set
	stTest.value().dwUin = 12345;
	snprintf(stTest.value().szName, sizeof(stTest.value().szName), "happy");

	//get
	cout << stTest.value().dwUin << endl;

	return 0;
}


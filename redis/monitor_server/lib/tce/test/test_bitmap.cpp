
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


int main()
{
	tce::shm::BITMAP1 bitmap1;//1 bit

	if ( !bitmap1.init(12345, /*shm key*/
		1024*1024) )	/*bitamp size*/
	{
		cout << "init error:" << endl;
	}

	//set 
	bitmap1.set(1, true);

	//get
	bool bValue = bitmap1.get(1);
	cout << "bValue=" << bValue << endl;
	//
	//tce::shm::BITMAP1 bitmap2;//2 bit
	//tce::shm::BITMAP1 bitmap3;//3 bit
	//tce::shm::BITMAP1 bitmap4;//4 bit
	//tce::shm::BITMAP1 bitmap5;//5 bit
	//tce::shm::BITMAP1 bitmap6;//6 bit
	//tce::shm::BITMAP1 bitmap8;//8 bit

	return 0;
}


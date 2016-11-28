
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


#include <unistd.h>
#include "monitor_client.h"
#include "wbl_comm.h"

int main(int argc, char** argv)
{

	wbl::Daemon();

	int i = 0;
	while(1)	//1s 1000 times
	{
		static char func[256];
		snprintf(func, 256, "Function%u", i);
		int ret = Monitor_Add("QQService.TestSvc", func, 1);
		printf("%d\n", ret);
		i++;
		if( i >= 500 )
			i = 0;
		usleep(1000);
	}
	return 0;
}

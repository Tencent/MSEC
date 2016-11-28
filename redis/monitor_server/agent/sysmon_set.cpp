
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


#include <stdio.h>
#include <stdlib.h>
#include "monitor_client.h"

int main(int argc, char** argv)
{
	if(argc != 3)
		return 0;

	int ret = Monitor_Set("RESERVED.monitor", argv[1], atoi(argv[2]));
	if(ret != 0)
		printf("[ERR]%s|%s|%d\n", argv[1], argv[2], ret);
	return 0;
}


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


/*
	add by aoxu
   frame.h is included by spp frame but not by module.
*/

#ifndef _SPP_COMM_FRAME_
#define _SPP_COMM_FRAME_
#include <string.h>
#include <stdio.h>
#include "tbase/tlog.h"
#include "tbase/tstat.h"

#define LOG_SCREEN(lvl, fmt, args...)   do{ printf("[%d] ", getpid());printf(fmt, ##args); fflush(stdout); flog_.LOG_P_PID(lvl, fmt, ##args); }while (0)

using namespace tbase::tlog;
using namespace tbase::tstat;
using namespace std;

namespace spp
{
    namespace comm
    {
		//框架公共类
		class CFrame
        {
        public:
            CTLog flog_;	//框架日志对象
			CTLog monilog_;	//框架监控日志对象
			CTStat fstat_;	//框架统计对象
            int groupid_;
        };
    }
}

#endif


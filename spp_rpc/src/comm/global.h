
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


#ifndef _SPP_GLOBAL_H
#define _SPP_GLOBAL_H
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <string>
#include <fcntl.h>
#include <errno.h>
#include <tbase/tlog.h>

#define  INTERNAL_LOG  SingleTon<CTLog, CreateByProto>::Instance()

namespace spp
{
    namespace global
    {
        class spp_global
        {
        public:
            static void daemon_set_title(const char* fmt, ...);

            //拷贝参数
	    static void save_arg(int argc, char** argv);
            static char* arg_start;
            static char* arg_end;
            static char* env_start;

            //共享内存满的告警脚本
            static std::string mem_full_warning_script;

            //当前proxy处理那个group的共享内存
            static int cur_group_id;
            static void set_cur_group_id(int group_id);
            static int get_cur_group_id();
        };

    }

}

#endif


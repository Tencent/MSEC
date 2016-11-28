
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


#include "defaultworker.h"
#include "comm_def.h"

using namespace spp;
using namespace spp::worker;

// Coredump Check Point
// 进入待检测区域时，设置对应的CheckPoint， 退出待检测区域时，恢复为CoreCheckPoint_SppFrame
CoreCheckPoint g_check_point = CoreCheckPoint_SppFrame;

struct sigaction g_sa;
CServerBase* g_worker = NULL;

char* checkpoint2str(CoreCheckPoint p)
{
    if(p == CoreCheckPoint_SppFrame)
        return "spp frame";
    else if(p == CoreCheckPoint_HandleProcess)
        return "spp_handle_process";
    else if(p == CoreCheckPoint_HandleFini)
        return "spp_handle_fini";
    else if(p == CoreCheckPoint_HandleInit)
        return "spp_handle_init";
    else
        return "unknown";
}


void sigsegv_handler(int sig)
{
    if (sig == SIGSEGV && g_worker)
    {
        signal(SIGSEGV, SIG_DFL);
        ((CDefaultWorker *)g_worker)->flog_.LOG_P_PID(LOG_FATAL,
            "[!!!!!]segmentation fault at %s\n", checkpoint2str(g_check_point));
    }
    return;
}

int main(int argc, char* argv[])
{
    g_worker = new CDefaultWorker;

    if (g_worker)
    {
        g_worker->run(argc, argv);

        delete g_worker;
    }

    return 0;
}

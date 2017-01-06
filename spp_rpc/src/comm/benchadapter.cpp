
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


#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <sys/types.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <stdio.h>
#include <dlfcn.h>
#include "tbase/tlog.h"
#include "global.h"
#include "singleton.h"
#include "benchadapter.h"
#include "benchapiplus.h"
#include "spp_version.h"

using namespace spp::global;
using namespace spp::singleton;
using namespace tbase::tlog;

/////////////////////////////////////////////////////////////////////////////////
//so library
////////////////////////////////////////////////////////////////////////////////

#define PROCTECT_AREA_SIZE (512*1024)

spp_dll_func_t sppdll = {NULL};

bool CheckCompatible(void *handle)
{
    void *ver_addr[3];

    ver_addr[0] = dlsym(handle, "SPP_MAJOR_VER");
    ver_addr[1] = dlsym(handle, "SPP_MINOR_VER");
    ver_addr[2] = dlsym(handle, "SPP_PATCH_VER");

    if ((NULL == ver_addr[0])
        || (NULL == ver_addr[1])
        || (NULL == ver_addr[2])) {
        return false;
    }

    for (unsigned i = 0; i < sizeof(SPP_COMPATIBLE_VERSION)/sizeof(SPP_COMPATIBLE_VERSION[0]); i++)
    {
        int j;

        for (j = 0; j < 3; j++) {
            if (*((int *)ver_addr[j]) != SPP_COMPATIBLE_VERSION[i][j]) {
                break;
            }
        }

        if (j == 3) {
            return true;
        }
    }

    return false;
}

int load_bench_adapter(const char* file, bool isGlobal)
{
    if (sppdll.handle != NULL)
    {
        dlclose(sppdll.handle);
    }

    memset(&sppdll, 0x0, sizeof(spp_dll_func_t));

    int flag = isGlobal ? RTLD_NOW | RTLD_GLOBAL : RTLD_NOW;

    /* 提早发现部分内存写乱现象 */
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    void* handle = dlopen(file, flag);
    mmap(NULL, PROCTECT_AREA_SIZE, PROT_NONE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);

    if (!handle)
    {
        printf("[ERROR]dlopen %s failed, errmsg:%s\n", file, dlerror());
        exit(-1);
    }

    if (!CheckCompatible(handle)) {
        printf("[ERROR]incompatible version, [%d.%d.%d]\n", SPP_MAJOR_VER, SPP_MINOR_VER, SPP_PATCH_VER);
        exit(-1);
    }

    void* func = dlsym(handle, "spp_handle_init");

    if (func != NULL)
    {
        sppdll.spp_handle_init = (spp_handle_init_t)dlsym(handle, "spp_handle_init");
        sppdll.spp_handle_input = (spp_handle_input_t)dlsym(handle, "spp_handle_input");
        sppdll.spp_handle_route = (spp_handle_route_t)dlsym(handle, "spp_handle_route");
        sppdll.spp_handle_process = (spp_handle_process_t)dlsym(handle, "spp_handle_process");
        sppdll.spp_handle_fini = (spp_handle_fini_t)dlsym(handle, "spp_handle_fini");
        sppdll.spp_handle_close = (spp_handle_close_t)dlsym(handle, "spp_handle_close");
        sppdll.spp_handle_exception = (spp_handle_exception_t)dlsym(handle, "spp_handle_exception");
        sppdll.spp_handle_loop = (spp_handle_loop_t)dlsym(handle, "spp_handle_loop");
        sppdll.spp_mirco_thread = (spp_mirco_thread_t)dlsym(handle, "spp_mirco_thread");
        sppdll.spp_handle_switch = (spp_handle_switch_t)dlsym(handle, "spp_handle_switch");
        sppdll.spp_set_notify = (spp_set_notify_t)dlsym(handle, "spp_set_notify");		

        if (sppdll.spp_handle_input == NULL)
        {
            printf("[ERROR]spp_handle_input not implemented.\n");
            exit(-1);
        }

        if (sppdll.spp_handle_process == NULL)
        {
            printf("[ERROR]spp_handle_process not implemented.\n");
            exit(-1);
        }

        //assert(sppdll.spp_handle_input != NULL && sppdll.spp_handle_process != NULL);
        sppdll.handle = handle;
        return 0;
    }
    else
    {
        printf("[ERROR]cannot find spp_handle_init in module.\n");
        return -1;
    }
}

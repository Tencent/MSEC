
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


/**
 * @filename comm.c
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "commstruct.h"

/**
 * @brief 初始化shm_servers
 */
void init_shm_servers(struct shm_servers *servers)
{
    servers->cost_total     = 0;
    servers->fail_total     = 0;
    servers->success_total  = 0;
    servers->server_num     = 0;
    servers->dead_num       = 0;
    servers->dead_retry_times = 0;
    servers->weight_total     = 0;
    servers->weight_dead_base = 0;
    servers->mhash_order      = 0;
    memset(servers->mhash_idx, 0xff, sizeof(servers->mhash_idx));
}


/**
 * @brief 通过IP查找路由服务器信息
 */
struct server_info *get_server_by_ip(struct shm_servers *servers, uint32_t ip)
{
    int32_t  i;
    uint32_t hash, idx, base = 0;
    struct server_info *server;

    /* 通过多阶hash快速查找 */
    for (i = 0; i < servers->mhash_order; i++) {
        hash = ip % servers->mhash_mods[i];
        idx  = servers->mhash_idx[base + hash];

        if (idx >= NLB_SERVER_MAX) {
            return NULL;
        }

        server = servers->svrs + idx;
        if (server->server_ip == ip) {
            return server;
        }

        base += servers->mhash_mods[i];
    }

    return NULL;
}



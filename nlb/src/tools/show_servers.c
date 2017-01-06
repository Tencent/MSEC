
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


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "nlbapi.h"
#include "nlbfile.h"
#include "comm.h"
#include "hash.h"
#include "commstruct.h"

static struct shm_meta *meta;
static struct shm_servers *servers0;
static struct shm_servers *servers1;

static void dumpservers(struct shm_servers *servers)
{
    uint32_t i;
    struct server_info *server;
    char weight_str[100];
    char port_str[100];
    char req_str[100];

    printf("general infomation:\n");
    printf("  cost_total:%lu  success_total:%lu fail_total:%lu server_num:%u dead_num:%u dead_retry_times:%u\n"
           "  weight_total: %u weight_dead_base:%u mhash_order:%u\n"
           "  version: %d weight_static_total: %u weight_low_num: %u shaping_request_min: %d \n"
           "  success_ratio_base: %f success_ratio_min: %f resume_weight_ratio: %f dead_retry_ratio: %f\n"
           "  weight_low_watermark: %f weight_low_ratio: %f weight_incr_ratio: %f\n\n",
           servers->cost_total, servers->success_total, servers->fail_total, servers->server_num, servers->dead_num,
           servers->dead_retry_times, servers->weight_total, servers->weight_dead_base, servers->mhash_order,
           servers->version, servers->weight_static_total, servers->weight_low_num, servers->shaping_request_min,
           servers->success_ratio_base, servers->success_ratio_min, servers->resume_weight_ratio, servers->dead_retry_ratio,
           servers->weight_low_watermark, servers->weight_low_ratio, servers->weight_incr_ratio);

    if (servers->server_num == 0) {
        return;
    }

    printf("servers infomation:\n");
    printf("  %-16s%-32s%-16s%-24s%-16s\n", "ip", "weight(static/dynamic/base)", "port(type/num)", "req(fail/success/cost)", "dead_time");
    for (i = 0; i < servers->server_num; i++) {
        server = servers->svrs + i;
        snprintf(weight_str, sizeof(weight_str), "%u/%u/%u", server->weight_static, server->weight_dynamic, server->weight_base);
        snprintf(port_str, sizeof(port_str), "%u/%u", server->port_type, server->port_num);
        snprintf(req_str, sizeof(req_str), "%u/%u/%lu", server->failed, server->success, server->cost);
        printf("  %-16s%-32s%-16s%-24s%lu\n", inet_ntoa(*(struct in_addr *)&server->server_ip), weight_str, port_str, req_str, server->dead_time);
    }
}


int main(int argc, char **argv)
{
    uint32_t mmaplen;
    uint32_t mmaplen0;
    uint32_t mmaplen1;
    struct shm_servers *servers;

    if (argc < 2) {
        printf(" Usage: %s <service_name>.\n", argv[0]);
        exit(1);
    }

    meta = load_meta_data(argv[1], &mmaplen);
    servers0 = load_server_data(argv[1], 0, &mmaplen0);
    servers1 = load_server_data(argv[1], 1, &mmaplen1);

    if (NULL == meta || NULL == servers0 || NULL == servers1) {
        printf("Load service data failed.\n");
        exit(1);
    }

    if (meta->index == 0) {
        servers = servers0;
    } else {
        servers = servers1;
    }

    dumpservers(servers);

    return 0;
}


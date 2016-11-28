
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


#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nlbapi.h"
#include "nlbfile.h"
#include "comm.h"
#include "hash.h"
#include "commstruct.h"

#define MHASH_NODE_CNT 1000000

void print_usage(const char *exe)
{
    printf("Usage: %s <servicename> <count>", exe);
}

static uint64_t cnt;
static char *service_name;

static uint32_t mhash_steps;
static uint32_t mhash_mods[MAX_ROW_COUNT];
static uint32_t mhash_index[MHASH_NODE_CNT];
static uint32_t mhash_statistics[MHASH_NODE_CNT];
static struct shm_meta *meta;
static struct shm_servers *servers;

void add_statistic(uint32_t ip)
{
    uint32_t i;
    uint32_t hash, index, base = 0;

    for (i = 0; i < mhash_steps; i++) {
        hash    = ip%mhash_mods[i];
        index   = hash + base;

        if (ip == mhash_index[index]) {
            mhash_statistics[index]++;
            break;
        } else if (mhash_index[index] == 0) {
            mhash_index[index] = ip;
            mhash_statistics[index]++;
            break;
        }

        base += mhash_mods[i];
    }
}

int32_t load_data(const char *name)
{
    uint32_t mmaplen;
    meta = load_meta_data(name, &mmaplen);
    if (NULL == meta) {
        return -1;
    }

    servers = load_server_data(name, meta->index, &mmaplen);
    if (NULL == servers) {
        return -2;
    }

    return 0;
}

int main(int argc, char **argv)
{
    int32_t ret, loop, i;
    uint32_t ip;
    struct routeid id;
    struct server_info *server;

    if (argc < 3) {
        print_usage(argv[0]);
        return 0;
    }

    cnt = (uint64_t)atoll(argv[2]);
    service_name = strdup(argv[1]);
    calc_hash_mods(MHASH_NODE_CNT, &mhash_steps, mhash_mods);

    loop = cnt;
    while (loop--) {
        ret = getroutebyname(service_name, &id);
        if (ret < 0) {
            printf("getroutebyname failed, ret [%d].\n", ret);
            continue;
        }

        add_statistic(id.ip);
    }

    ret = load_data(service_name);
    if (ret < 0) {
        printf("load route data failed. ret [%d]\n", ret);
        exit(1);
    }

    printf("statistic details (weight total=%u):\n", servers->weight_total);
    printf("%-16s%-8s%-8s%-8s%-8s%-10s%-8s\n", "ip", "static", "ratio", "dynamic", "ratio", "req_total", "ratio");

    for (i = 0; i < MHASH_NODE_CNT; i++) {
        if (mhash_index[i]) {
            ip = mhash_index[i];
            server = get_server_by_ip(servers, ip);
            if (NULL == server) {
                printf("%-16s%-8s%-8s%-8s%-8s%-10u%.2lf\n", inet_ntoa(*(struct in_addr *)&ip),
                       "no", "no", "no", "no", mhash_statistics[i], ((double)mhash_statistics[i])/((double)cnt));
                continue;
            }
            printf("%-16s%-8u%-8s%-8u%-8.4lf%-10u%-3.5lf\n", inet_ntoa(*(struct in_addr *)&ip),
                   server->weight_static, "unknown", server->weight_dynamic,
                   (double)(server->weight_dynamic)/(double)(servers->weight_total),
                   mhash_statistics[i], ((double)mhash_statistics[i])/((double)cnt));
        }
    }

    return 0;
}





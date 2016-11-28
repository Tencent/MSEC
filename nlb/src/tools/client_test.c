
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "nlbapi.h"

void print_usage(int argc, char **argv)
{
    printf("Usage: %s <service_name> <ip> <fail|success> <cost>", argv[0]);
    exit(1);
}

int main(int argc, char **argv)
{
    int32_t  ret;
    uint32_t ip;
    int32_t  cost = 0;
    int32_t  failed;
    struct routeid id;

    if (argc < 5) {
        print_usage(argc, argv);
        return -1;
    }

    ip = (uint32_t)inet_addr(argv[2]);
    if (strncmp(argv[3], "fail", 5) == 0) {
        failed = 1;
    }else {
        failed = 0;
        cost   = atoi(argv[4]);
    }

    ret = getroutebyname(argv[1], &id);
    if (ret < 0) {
        printf("getroutebyname failed, ret [%d].\n", ret);
        return -2;
    }

    ret = updateroute(argv[1], ip, failed, cost);
    if (ret < 0) {
        printf("updateroute failed, ret [%d].\n", ret);
        return -2;
    }

    return 0;
}




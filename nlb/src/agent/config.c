
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
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <getopt.h>
#include "config.h"
#include "zookeeper.h"
#include "version.h"
#include "utils.h"
#include "commdef.h"
#include "log.h"

struct config g_agent_config;

/**
 * @brief 打印版本信息
 */
static void print_version(void)
{
    printf(" Nlb version: %s\n", NLB_VERSION);
    printf(" Zookeeper c libary version: %d.%d.%d\n", ZOO_MAJOR_VERSION, ZOO_MINOR_VERSION, ZOO_PATCH_VERSION);
}

/**
 * @brief 打印帮助信息
 */
static void print_usage(const char *name)
{
    print_version();
    printf(" This is a program for nlb agent.\n");
    printf(" Usage:  %s zookeeper_servers -m [client|server|mix] [OPTION]\n", name);
    printf("        -h  --help          Print this usage\n");
    printf("        -v  --version       Print nlb version\n");
    printf("        -t  --timeout       Set agent timeout\n");
    printf("        -m  --mode          Set agent work mode [client|server|mix], default mix\n");
    printf("        -i  --interface     Set server agent network interface, default eth0\n");
    printf("        -p  --plugin        Set plugin dynamic libary path\n");
    printf("        -l  --log-level     Set agent log level (ERROR/WARN/INFO/DEBUG), default ERROR\n");
}

/**
 * @brief 处理命令行参数
 */
void parse_args(int32_t argc, char **argv)
{
    //int32_t  c;
    uint32_t ip = 0;
    int32_t  timeout = 10000, mode = MIX_MODE;
    int32_t  log_levl = ERROR;
    int32_t  index;
    char *   host;
    char *   plugin = "msec_rpc.so";
#if 0
    const char *short_opts = "vht:s:m:p:i:l:";
    const struct option long_opts[] = {
            {"version", no_argument, NULL, 'v'},
            {"help", no_argument, NULL, 'h'},
            {"timeout", required_argument, NULL, 't'},
            {"mode", required_argument, NULL, 'm'},
            {"interface", required_argument, NULL, 'i'},
            {"plugin", required_argument, NULL, 'p'},
            {"log-level", required_argument, NULL, 'l'},
            {0,0,0,0}
    };
#endif

    if (argc < 2 || *argv[1] == '-') {
        printf("No zookeeper_servers argment.\n");
        print_usage(argv[0]);
        exit(1);
    }

    host = strdup(argv[1]);

    for (index = 2; index < argc; ) {
        if (!strcmp(argv[index], "-h")
            || !strcmp(argv[index], "--help")) {
            print_usage(argv[0]);
            exit(1);
        }

        if (!strcmp(argv[index], "-v")
            || !strcmp(argv[index], "--version")) {
            print_version();
            exit(1);
        }

        if (!strcmp(argv[index], "-t")
            || !strcmp(argv[index], "--timeout")) {
            if (index == (argc - 1)) {
                printf("Invalid %s option!\n", argv[index]);
                exit(1);
            }

            timeout = atoi(argv[index+1]);
            if (timeout <= 0) {
                printf("Invalid timeout: %s\n", argv[index+1]);
                exit(1);
            }

            index = index + 2;
            continue;
        }

        if (!strcmp(argv[index], "-m")
            || !strcmp(argv[index], "--mode")) {
            if (index == (argc - 1)) {
                printf("Invalid %s option!\n", argv[index]);
                exit(1);
            }

            if (!strcmp(argv[index+1], "server"))
                mode = SERVER_MODE;
            else if(!strcmp(argv[index+1], "client"))
                mode = CLIENT_MODE;
            else if(!strcmp(argv[index+1], "mix"))
                mode = MIX_MODE;
            else {
                printf("Invalid agent mode: %s\n", argv[index+1]);
                exit(1);
            }

            index = index + 2;
            continue;
        }

        
        if (!strcmp(argv[index], "-i")
            || !strcmp(argv[index], "--interface")) {

            if (index == (argc - 1)) {
                printf("Invalid %s option!\n", argv[index]);
                exit(1);
            }

            ip = get_ip(argv[index + 1]);
            if (!ip) {
                printf("Invalid interface: %s\n", argv[index + 1]);
                exit(1);
            }

            index = index + 2;
            continue;
        }

        if (!strcmp(argv[index], "-p")
            || !strcmp(argv[index], "--plugin")) {
            if (index == (argc - 1)) {
                printf("Invalid %s option!\n", argv[index]);
                exit(1);
            }

            plugin = strdup(argv[index + 1]);
            index  = index + 2;
            continue;
        }

        if (!strcmp(argv[index], "-l")
            || !strcmp(argv[index], "--log-level")) {
            if (index == (argc - 1)) {
                printf("Invalid %s option!\n", argv[index]);
                exit(1);
            }

            if (!strcmp(argv[index + 1], "ERROR")) {
                log_levl = ERROR;
            } else if (!strcmp(argv[index + 1], "WARN")) {
                log_levl = WARN;
            } else if (!strcmp(argv[index + 1], "INFO")) {
                log_levl = INFO;
            } else if (!strcmp(argv[index + 1], "DEBUG")) {
                log_levl = DEBUG;
            }else {
                printf("Invalid log level: %s\n", argv[index + 1]);
                exit(1);
            }

            index = index + 2;
            continue;
        }

        printf("Error: unknown option '%s'\n", argv[index]);
        print_usage(argv[0]);
        exit(1);
        
    }

    if (!ip && (g_agent_config.mode != CLIENT_MODE)) {
        printf("Error: mix and server mode need interface argument\n");
        exit(1);
    }

    /* 保存参数数据 */
    g_agent_config.mode     = mode;
    g_agent_config.timeout  = timeout;
    g_agent_config.port     = NLB_AGENT_LISTEN_PORT;
    g_agent_config.local_ip = ip;
    g_agent_config.log_level= log_levl;
    g_agent_config.host     = host;
    g_agent_config.plugin   = plugin;

    print_version();
    printf("    mode        : %-16d (1:SERVER_MODE 2:CLIENT_MODE 3:MIX_MODE)\n", mode);
    printf("    timeout     : %-16d (zookeeper timeout config)\n", timeout);
    printf("    port        : %-16d (nlb agent listen port)\n", NLB_AGENT_LISTEN_PORT);
    printf("    local addr  : %-16s (local interface address)\n", inet_ntoa(*(struct in_addr *)&ip));
    printf("    log level   : %-16d (1: ERROR 2: WARN 3: INFO 4:DEBUG)\n", log_levl);
    printf("    zk host     : %s (zookeeper server host)\n", host);
}




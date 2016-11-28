
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


#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <linux/unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "commstruct.h"
#include "nlbapi.h"

#define SERVER_NUM 100

void init_dummy_servers_data(void)
{
    int i;
    int fd;
    int ret, left, wlen = 0;
    int len = sizeof(struct shm_servers) + sizeof(struct server_info)*SERVER_NUM;
    int mlen = sizeof(struct shm_servers) + sizeof(struct server_info)*NLB_SERVER_MAX;
    unsigned int w_base = 0;
    struct shm_servers *servers = calloc(1, len);
    struct server_info *server = servers->servers;
    servers->sb.server_num = SERVER_NUM;
    servers->sb.weight_total = 5050; // [1.....100]
    servers->sb.dead_num = 0;

    for (i = 0; i < SERVER_NUM; i++, server++) {
        server->server_ip = i+1;
        server->weight_base = w_base;
        server->weight_static = i+1;
        server->weight_dynamic = server->weight_static;
        server->port_base = 1111;
        server->port_step = 1;
        server->port_num = 2;
        server->port_type = 3;
        w_base += server->weight_dynamic;
    }

    fd = open("/var/nlb/naming/login/ptlogin/servers.dat0", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error [%m]!\n");
        exit(1);
    }

    left = len;
    while (left > 0) {
        ret = write(fd, ((char *)servers + wlen), left);
        if (ret < 0) {
            continue;
        }

        left -= ret;
        wlen += ret;
    }

    ftruncate(fd, (mlen+4095)/4096*4096);

    close(fd);

    fd = open("/var/nlb/naming/login/ptlogin/servers.dat1", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error [%m]!\n");
        exit(1);
    }

    left = len;
    wlen = 0;
    while (left > 0) {
        ret = write(fd, ((char *)servers + wlen), left);
        if (ret < 0) {
            continue;
        }

        left -= ret;
        wlen += ret;
    }

    ftruncate(fd, (mlen+4095)/4096*4096);

    close(fd);
}

void init_dummy_stat_data(void)
{
    int i;
    int fd;
    int ret, left, wlen = 0;
    int len = sizeof(struct shm_statistics) + sizeof(struct server_statistics) * SERVER_NUM;
    int mlen = sizeof(struct shm_statistics) + sizeof(struct server_statistics) * NLB_SERVER_MAX;;
    struct shm_statistics *stat = calloc(1, len);
    struct server_statistics *s_stat = stat->statistics;

    for (i = 0; i < SERVER_NUM; i++) {
        stat->statistics[i].server_ip = i+1;
        stat->sb.data[i+1] = i;
    }

    stat->sb.mods[0] = 10000;
    stat->sb.mods[1] = 9999;
    stat->sb.mods[2] = 9998;
    stat->sb.mods[3] = 9997;
    stat->sb.mods[4] = 9996;
    stat->sb.mods[5] = 9995;
    stat->sb.mods[6] = 9994;
    stat->sb.mods[7] = 9993;
    stat->sb.mods[8] = 9992;
    stat->sb.mods[9] = 9991;
    stat->sb.mods[10] = 9990;
    stat->sb.mods[11] = 37;
    stat->sb.mods[12] = 18;

    fd = open("/var/nlb/naming/login/ptlogin/statistics.dat0", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error [%m]!\n");
        exit(1);
    }

    left = len;
    while (left > 0) {
        ret = write(fd, ((char *)stat + wlen), left);
        if (ret < 0) {
            continue;
        }

        left -= ret;
        wlen += ret;
    }

    ftruncate(fd, (mlen+4095)/4096*4096);
    close(fd);

    fd = open("/var/nlb/naming/login/ptlogin/statistics.dat1", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error [%m]!\n");
        exit(1);
    }

    left = len;
    wlen = 0;
    while (left > 0) {
        ret = write(fd, ((char *)stat + wlen), left);
        if (ret < 0) {
            continue;
        }

        left -= ret;
        wlen += ret;
    }

    ftruncate(fd, (mlen+4095)/4096*4096);

    close(fd);
}

void init_dummy_meta_data(void)
{
    int fd;
    int left, ret, wlen, len = sizeof(struct shm_meta);
    struct shm_meta meta = {0,0,1,"login.ptlogin"};

    fd = open("/var/nlb/naming/login/ptlogin/meta.dat", O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        printf("open error [%m]!\n");
        exit(1);
    }

    left = len;
    wlen = 0;
    while (left > 0) {
        ret = write(fd, ((char *)&meta + wlen), left);
        if (ret < 0) {
            continue;
        }

        left -= ret;
        wlen += ret;
    }

    ftruncate(fd, (len+4095)/4096*4096);

    close(fd);
}

int main(int argc, char **argv)
{
    int i;
    int ret;
    int cnt;
    int server_stat[SERVER_NUM+1] = {0};
    struct routeid id;

    if (argc == 1)
    {
        init_dummy_stat_data();
        init_dummy_servers_data();
        init_dummy_meta_data();
        return 0;
    }

    cnt = atoi(argv[1]);
    for (i = 0; i < cnt; i++) {
        ret = getroutebyname("login.ptlogin", &id);

        if (id.ip > 100000 || id.ip == 0 || (id.port != 1111 && id.port != 1112)) {
            printf("get route failed, %d, ip:%u port:%u .\n", ret, id.ip, id.port);
        }

        server_stat[id.ip]++;
    }

    for (i = 1; i < SERVER_NUM+1; i++) {
        printf("serverid: %10d  called:%10d\n", i, server_stat[i]);
    }

    return 0;
}

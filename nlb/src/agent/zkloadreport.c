
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
#include <string.h>
#include "zookeeper.h"
#include "hash.h"
#include "config.h"
#include "log.h"
#include "event.h"
#include "commdef.h"
#include "commstruct.h"
#include "zkplugin.h"
#include "zkloadreport.h"
#include "sysinfo.h"
#include "nlbtime.h"

static bool loadreport_created = false;

/* 检查是否创建心跳节点 */
bool check_loadreport_created(void)
{
    return loadreport_created;
}

/* 设置心跳节点已经创建 */
void set_loadreport_created(void)
{
    loadreport_created = true;
}

/**
 * @brief 获取zookeeper节点路径
 */
static int32_t make_zk_loadreport_path(uint32_t ip, char *buff, int32_t len)
{
    int32_t slen;

    slen = snprintf(buff, len, "/loadreport/%s", inet_ntoa(*(struct in_addr *)&ip));
    if (slen >= len) {
        return -1;
    }

    return 0;
}

/**
 * @brief 创建服务器负载上报临时节点回调函数
 */
static void loadreport_create_complate(int32_t rc, const char *name, const void *data)
{
    uint32_t ip = (uint32_t)(long)data;

    if ((rc == ZNODEEXISTS) || (rc == ZOK)) {
        set_loadreport_created();
        NLOG_INFO("create loadreport node success, [%s]", inet_ntoa(*(struct in_addr *)&ip));
        return;
    }

    NLOG_ERROR("create loadreport node failed, [%s] [%s]", 
               inet_ntoa(*(struct in_addr *)&ip), zerror(rc));
}

/**
 * @brief 创建服务器负载上报临时节点
 * @info  服务提供方agent创建，[server | mix]
 *        上报数据包含CPU\MEM
 */
int32_t create_loadreport_node(uint32_t ip)
{
    int32_t ret;
    int32_t flags = ZOO_EPHEMERAL;
    char    path[NLB_PATH_MAX_LEN];

    if (!zk_connected()) {
        NLOG_DEBUG("zookeeper is not connected");
        return 0;
    }

    /* 创建父节点 */
    ret = zk_simple_create("/loadreport");
    if (ret < 0) {
        NLOG_ERROR("create /loadreport node failed, [%d]", ret);
        return -1;
    }

    /* 创建负载上报子节点 */
    make_zk_loadreport_path(ip, path, sizeof(path));
    ret = zoo_acreate(get_zk_instance(), path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, flags,
                      loadreport_create_complate, (void *)(long)ip);
    if (ret != ZOK) {
        NLOG_ERROR("create loadreport node failed, [%s] [%s]",
                  inet_ntoa(*(struct in_addr *)&ip), zerror(ret));
        return -2;
    }

    return 0;
}

/**
 * @brief 负载上报完成回调函数
 */
static void loadreport_set_completion(int32_t rc, const struct Stat *stat, const void *data)
{
    uint32_t ip = (uint32_t)(long)data;

    if (rc != ZOK) {
        if (rc == ZNONODE) {
            create_loadreport_node(ip);
        }

        NLOG_ERROR("load report failed, [%s] [%s]", inet_ntoa(*(struct in_addr *)&ip), zerror(rc));
        return;
    }

    NLOG_DEBUG("load report success, [%s]", inet_ntoa(*(struct in_addr *)&ip));
}

/**
 * @brief 负载上报主处理函数
 */
void load_report(uint32_t ip)
{
    int32_t  ret, data_len;
    uint32_t cpu_percent;
    uint64_t mem_total;
    uint64_t mem_free;
    char     buff[1024];
    char     path[NLB_PATH_MAX_LEN];

    if (!zk_connected()) {
        NLOG_DEBUG("zookeeper is not connected");
        return;
    }

    make_zk_loadreport_path(ip, path, sizeof(path));

    /* 获取系统信息 */
    get_sysinfo(&cpu_percent, &mem_total, &mem_free);

    /* 组装json字符串 */
    data_len = snprintf(buff, sizeof(buff), "{\"timestamp\": %lu, \"cpu\": %u, \"mem_total\": %lu, \"mem_free\": %lu}", 
                        get_time_ms(), cpu_percent, mem_total, mem_free);

    ret = zoo_aset(get_zk_instance(), path, buff, data_len, -1, loadreport_set_completion, (void *)(long)ip);
    if (ret != ZOK) {
        NLOG_ERROR("set load report data failed, [%s]", zerror(ret));
    }
}



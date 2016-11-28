
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
#include "zkheartbeat.h"
#include "policy.h"

#define NLB_NODE_WATCHER_MAX        1000000              /* 多阶hash节点数，100万 */

static bool heartbeat_created = false;
static uint32_t node_watcher_mod_cnt = MAX_ROW_COUNT;    /* 多阶hash阶数 */
static uint32_t node_watcher_mods[MAX_ROW_COUNT];        /* 多阶hash模数 */
static uint32_t node_watcher_mhash[NLB_NODE_WATCHER_MAX];/* 多阶hash数组 */

/**
 * @brief 获取zookeeper节点路径
 */
static int32_t make_zk_heartbeat_path(uint32_t ip, char *buff, int32_t len)
{
    int32_t slen;

    slen = snprintf(buff, len, "/serverheartbeat/%s", inet_ntoa(*(struct in_addr *)&ip));
    if (slen >= len) {
        return -1;
    }

    return 0;
}

/**
 * @brief 检查一个节点是否在监视中
 */
static bool is_node_watching(uint32_t ip)
{
    uint32_t i;
    uint32_t hash, idx, base = 0;

    for (i = 0; i < node_watcher_mod_cnt; i++) {
        hash = ip%node_watcher_mods[i];
        idx  = hash + base;
        if (node_watcher_mhash[idx] == ip) {
            return true;
        }
        base += node_watcher_mods[i];
    }

    return false;
}

/**
 * @brief 清除一个节点的监视状态
 */
static void clean_node_watching(uint32_t ip)
{
    uint32_t i;
    uint32_t hash, idx, base = 0;

    for (i = 0; i < node_watcher_mod_cnt; i++) {
        hash = ip%node_watcher_mods[i];
        idx  = hash + base;
        if (node_watcher_mhash[idx] == ip) {
            node_watcher_mhash[idx] = 0;
            return;
        }

        base += node_watcher_mods[i];
    }
}

/**
 * @brief 设置一个节点已经在监视中
 * @info  在多阶hash中写入IP地址
 */
static void set_node_watching(uint32_t ip)
{
    uint32_t i;
    uint32_t hash, idx, base = 0;

    for (i = 0; i < node_watcher_mod_cnt; i++) {
        hash = ip%node_watcher_mods[i];
        idx  = hash + base;

        if (node_watcher_mhash[idx] == 0) {
            node_watcher_mhash[idx] = ip;
            return;
        }
        base += node_watcher_mods[i];
    }

    NLOG_ERROR("node_watcher_mhash is full, [%s].", inet_ntoa(*(struct in_addr *)&ip));
}

/* 检查是否创建心跳节点 */
bool check_heartbeat_created(void)
{
    return heartbeat_created;
}

/* 设置心跳节点已经创建 */
void set_heartbeat_created(void)
{
    heartbeat_created = true;
}

/* 初始化节点监视数据 */
void heartbeat_data_init(void)
{
    /* 初始化保存节点监视状态的多阶hash */
    calc_hash_mods(NLB_NODE_WATCHER_MAX, &node_watcher_mod_cnt, node_watcher_mods);
}

/**
 * @brief 创建心跳节点回调函数
 */
static void heartbeat_create_complate(int32_t rc, const char *name, const void *data)
{
    uint32_t ip = (uint32_t)(long)data;
    if ((rc == ZNODEEXISTS) || (rc == ZOK)) {
        set_heartbeat_created();
        NLOG_INFO("create heartbeat node success, [%s]", inet_ntoa(*(struct in_addr *)&ip));
        return;
    }

    NLOG_ERROR("create heartbeat node failed, [%s] [%s]", inet_ntoa(*(struct in_addr *)&ip), zerror(rc));
}

/**
 * @brief 创建服务器临时节点，用于检测节点死活
 * @info  服务提供方agent创建，[server | mix]
 */
int32_t create_heartbeat_node(uint32_t ip)
{
    int32_t ret;
    int32_t flags = ZOO_EPHEMERAL;
    char    path[NLB_PATH_MAX_LEN];

    if (!zk_connected()) {
        NLOG_DEBUG("zookeeper is not connected");
        return 0;
    }

    /* 创建父节点 */
    ret = zk_simple_create("/serverheartbeat");
    if (ret < 0) {
        NLOG_ERROR("create /serverheartbeat node failed, [%d]", ret);
        return -1;
    }

    /* 创建心跳子节点 */
    make_zk_heartbeat_path(ip, path, sizeof(path));
    ret = zoo_acreate(get_zk_instance(), path, NULL, 0, &ZOO_OPEN_ACL_UNSAFE, flags,
                      heartbeat_create_complate, (void *)(long)ip);
    if (ret != ZOK) {
        NLOG_ERROR("create %s node failed, [%s] [%s]", path,
                   inet_ntoa(*(struct in_addr *)&ip), zerror(ret));
        return -2;
    }

    return 0;
}

/**
 * @brief /serverheartbeat/ip节点exists回调函数
 */
static void heartbeat_exist_complate(int32_t rc, const struct Stat *stat, const void *data)
{
    uint32_t ip = (uint32_t)(long)data;

    if (rc == ZNONODE) {
        NLOG_INFO("Server (%s) no heartbeat", inet_ntoa(*(struct in_addr *)&ip));
        add_node_event((uint32_t)(long)data, NLB_EVENT_TYPE_NODE_DEAD);
        set_node_watching(ip);
        return;
    }

    if (rc == ZOK) {
        NLOG_DEBUG("Server (%s) alive", inet_ntoa(*(struct in_addr *)&ip));
        add_node_event((uint32_t)(long)data, NLB_EVENT_TYPE_NODE_RESUME);
        set_node_watching(ip);
        return;
    }

    NLOG_ERROR("heartbeat_complate failed, [%s]", zerror(rc));
    clean_node_watching(ip);
}

/**
 * @brief /serverheartbeat/ip节点exists watcher函数
 */
static void heartbeat_exist_watcher(zhandle_t *zzh, int32_t type, int32_t state, const char *path, void* context)
{
    uint32_t ip = (uint32_t)(long)context;

    NLOG_DEBUG("heartbeat watcher %s state %s ip %s",
              zk_type_2_str(type), zk_stat_2_str(state),
              inet_ntoa(*(struct in_addr *)&ip));

    if (state == ZOO_CONNECTED_STATE) {
        if (type == ZOO_SESSION_EVENT) {
            return;
        }

        if (type == ZOO_CREATED_EVENT) {
            NLOG_ERROR("Server (%s) alive", inet_ntoa(*(struct in_addr *)&ip));
            add_node_event(ip, NLB_EVENT_TYPE_NODE_RESUME);
        }

        if (type == ZOO_DELETED_EVENT) {
            NLOG_ERROR("Server (%s) no heartbeat", inet_ntoa(*(struct in_addr *)&ip));
            add_node_event(ip, NLB_EVENT_TYPE_NODE_DEAD);
        }
    }

    clean_node_watching(ip);
}

/**
 * @brief 设置节点监视事件
 */
int32_t set_node_watcher(uint32_t ip)
{
    int32_t ret;
    char    path[NLB_PATH_MAX_LEN];

    if (!is_node_watching(ip) && zk_connected()) {
        make_zk_heartbeat_path(ip, path, sizeof(path));
        ret = zoo_awexists(get_zk_instance(), path, heartbeat_exist_watcher, (void *)(long)ip,
                           heartbeat_exist_complate, (void *)(long)ip);
        if (ret != ZOK) {
            NLOG_ERROR("set node watcher failed, [%s] [%s].",
                       inet_ntoa(*(struct in_addr *)&ip), zerror(ret));
            return -1;
        }
    }

    return 0;
}

/**
 * @brief 设置单个业务所有节点的watch信息
 */
void set_service_nodes_wather(struct agent_local_rdata *rdata)
{
    uint32_t i, index;
    uint32_t ip;
    struct shm_servers *servers;
    struct server_info *server;
    struct shm_meta *meta = rdata->route_meta;

    index   = meta->index;
    servers = rdata->servs_data[index];

    /* 检查策略是否需要关注服务器心跳 */
    if (!check_need_heartbeat(servers->policy)) {
        return;
    }

    /* 循环遍历所有服务器 */
    for (i = 0; i < servers->server_num; i++) {
        server  = &servers->svrs[i];
        ip      = server->server_ip;
        set_node_watcher(ip);
    }
}

/**
 * @brief 设置所有节点没有watch
 */
void clean_nodes_watching(void)
{
    memset(node_watcher_mhash, 0, sizeof(node_watcher_mhash));
}



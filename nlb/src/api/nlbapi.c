
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
 * @filename nlbapi.c
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
#include "hash.h"
#include "commtype.h"
#include "commdef.h"
#include "commstruct.h"
#include "slist.h"
#include "nlbapi.h"
#include "nlbfile.h"
#include "comm.h"
#include "routeproto.h"
#include "utils.h"
#include "atomic.h"
#include "version.h"
#include "nlbrand.h"

#define NLB_ROUTE_DATA_HASHLEN 107

/* 一个后台服务的路由相关数据 */
struct api_routedata
{
    struct slist_head node;                  /* 链表节点   */
    char name[NLB_SERVICE_NAME_LEN];         /* 业务名     */
    struct shm_meta *route_meta;             /* 元数据信息 */
    struct shm_servers *servers_data[2];     /* 服务器信息 */
};

/* API所有业务路由数据，使用hash建索引，快速查找 */
static struct slist_head route_data_hash[NLB_ROUTE_DATA_HASHLEN];


/**
 * @brief 更新服务器和统计数据
 * @param rdata: 路由数据保存数据结构
 */
int32_t update_route_data(struct api_routedata *rdata)
{
    int32_t  result;
    uint32_t maplen0;
    uint32_t maplen1;

    struct shm_meta *meta = rdata->route_meta;
    void *server_data0 = NULL;
    void *server_data1 = NULL;

    /* 加载服务器数据 */
    server_data0 = load_server_data(meta->name, 0, &maplen0);
    if (NULL == server_data0) {
        result = -1;
        goto EXIT_LABEL;
    }

    server_data1 = load_server_data(meta->name, 1, &maplen1);
    if (NULL == server_data1) {
        result = -2;
        goto EXIT_LABEL;
    }

    /* 更新路由数据 */
    rdata->servers_data[0] = server_data0;
    rdata->servers_data[1] = server_data1;

    return 0;

EXIT_LABEL:
    if (NULL != server_data0) {
        munmap(server_data0, maplen0);
    }

    if (NULL != server_data1) {
        munmap(server_data1, maplen1);
    }

    return result;
}


/**
 * @brief 通过服务名查找路由服务器数据
 */
struct api_routedata *get_route_data(const char *name)
{
    uint32_t hash = gen_hash_key(name);
    uint32_t idx  = hash % NLB_ROUTE_DATA_HASHLEN;

    struct slist_head *list = route_data_hash + idx;
    struct slist_head *curr;
    struct api_routedata *rdata;

    /* 在当前数据中查找路由数据 */
    slist_for_each_entry(rdata, curr, list, node) {
        if (!strncmp(name, rdata->name, NLB_SERVICE_NAME_LEN)) {
            return rdata;
        }
    }

    return NULL;
}

/**
 * @brief 检查死机机器是否可用
 */
bool check_dead_useable(struct shm_servers *servers)
{
    uint32_t old_retry;

    old_retry = servers->dead_retry_times;
    while (old_retry) {
        if (compare_and_swap(&servers->dead_retry_times, old_retry, old_retry-1)) {
            return true;
        }

        old_retry = servers->dead_retry_times;
    }

    return false;
}

/* 随机获取一个服务器的端口 */
uint16_t get_one_port(struct server_info *server)
{
    uint16_t idx = nlb_rand() % server->port_num;
    return (server->port[idx]);
}

/* 获取端口类型 */
NLB_PORT_TYPE get_port_type(struct server_info *server)
{
    return (NLB_PORT_TYPE)server->port_type;
}

float calc_success_ratio(struct shm_servers *shm_servers, struct server_info *server)
{
    uint32_t req_total;

    req_total = server->failed + server->success;

    if (req_total < shm_servers->shaping_request_min) {
        return 100.0;
    }

    return ((float)server->success)/req_total;
}

/**
 * @brief 通过二分查找法查找路由服务器
 * @info  1. 服务器都死机，会随机找一个服务器
 *        2. 死机服务器如果有dead_retrys,会尝试dead_retrys次
 * @return <0 失败 =0 成功
 */
int32_t search_route(struct api_routedata *route_data, struct routeid *route)
{
    uint32_t high, mid, low = 0;
    uint32_t weight_rand, weight_total;
    uint32_t server_num, dead_num, dead_base;
    uint32_t index = route_data->route_meta->index;

    struct shm_servers *servers_data = route_data->servers_data[index];
    struct server_info *servers      = servers_data->svrs;
    struct server_info *server;

    server_num   = servers_data->server_num;
    dead_num     = servers_data->dead_num;
    dead_base    = servers_data->weight_dead_base;
    weight_total = servers_data->weight_total;

    /* 没有服务器信息 */
    if (!server_num) {
        return NLB_ERR_NO_ROUTE;
    }

    /* 服务器全死机,原则上不会出现总权重为0的情况 */
    if (dead_num == server_num || !dead_base || !weight_total) {
        server = servers + nlb_rand() % server_num;
        goto FOUND_ROUTE;
    }

    /* 如果权重落入死机区域，随机选择一个 */
    weight_rand = nlb_rand() % weight_total;
    if (weight_rand >= dead_base && check_dead_useable(servers_data)) {
        server = servers + server_num - dead_num + nlb_rand()%dead_num;
        goto FOUND_ROUTE;
    }

    /* 二分查找 */
    weight_rand = nlb_rand() % dead_base;
    high = server_num - dead_num - 1;
    while (low <= high) {
        mid    = (low + high)/2;
        server = servers + mid;
        if (low == high) {
            goto FOUND_ROUTE;
        }

        if (weight_rand < server->weight_base) {
            high = mid - 1;
            continue;
        }

        if (weight_rand >= (server->weight_dynamic + server->weight_base)) {
            low = mid + 1;
            continue;
        }

        goto FOUND_ROUTE;
    }

FOUND_ROUTE:

    /* 如果当前机器成功率太低，重新再随机选择一个 */
    if (calc_success_ratio(servers_data, server) < servers_data->success_ratio_min) {
        server = servers + nlb_rand() % server_num;
    }

    route->ip    = server->server_ip;
    route->port  = get_one_port(server);
    route->type  = get_port_type(server);

    return 0;
}

/**
 * @brief 加载路由服务器数据
 */
struct api_routedata *load_route_data(const char *name)
{
    uint32_t hash;
    uint32_t maplen, maplen0, maplen1;

    struct shm_meta *meta =NULL;
    struct shm_servers *servers0 = NULL;
    struct shm_servers *servers1 = NULL;
    struct api_routedata *route_data = NULL;

    if (NULL == name || name[0] == '\0') {
        return NULL;
    }

    /* 加载元数据 */
    meta = load_meta_data(name, &maplen);
    if (NULL == meta) {
        goto EXIT_LABEL;
    }

    /* 加载服务器数据 */
    servers0 = load_server_data(name, 0, &maplen0);
    if (NULL == servers0) {
        goto EXIT_LABEL;
    }

    servers1 = load_server_data(name, 1, &maplen1);
    if (NULL == servers0) {
        goto EXIT_LABEL;
    }

    /* 初始化routedata */
    route_data = calloc(1, sizeof(struct api_routedata));
    if (NULL == route_data) {
        goto EXIT_LABEL;
    }

    strncpy(route_data->name, name, NLB_SERVICE_NAME_LEN);
    route_data->route_meta      = meta;
    route_data->servers_data[0] = servers0;
    route_data->servers_data[1] = servers1;

    /* 加入单向链表 */
    hash = gen_hash_key(name);
    slist_add(&route_data_hash[hash % NLB_ROUTE_DATA_HASHLEN], &route_data->node);

    return route_data;

EXIT_LABEL:
    if (NULL != meta)
        munmap(meta, maplen);
    if (NULL != servers0)
        munmap(servers0, maplen0);
    if (NULL != servers1)
        munmap(servers1, maplen1);
    if (NULL != route_data)
        free(route_data);

    return NULL;
}

/**
 * @brief  通过业务名到Agent获取路由
 * @return <0 失败  0 成功
 */
int32_t get_route_from_agent(const char *name, struct routeid *route)
{
    int32_t ret, len, result = 0;
    int32_t fd;
    char    buff[1024];
    struct  sockaddr_in server_addr;

    if (NULL == name || NULL == route) {
        return NLB_ERR_INVALID_PARA;
    }

    /* 创建UDP socket，并设置非阻塞 */
    fd = create_udp_socket();
    if (fd < 0) {
        return NLB_ERR_CREATE_SOCKET_FAIL;
    }

    /* 组包并发送路由请求 */
    make_inet_addr("127.0.0.1", (uint16_t)NLB_AGENT_LISTEN_PORT, &server_addr);
    len = serialize_route_request(name, buff, sizeof(buff));
    ret = udp_send(fd, &server_addr, buff, len);
    if (ret < 0) {
        result = NLB_ERR_SEND_FAIL;
        goto EXIT_LABEL;
    }

    /* 阻塞接收路由请求应答: 1秒超时 */
    len = udp_recv(fd, buff, sizeof(buff), 1000);
    if (len <= 0) {
        result = NLB_ERR_RECV_FAIL;
        goto EXIT_LABEL;
    }

    ret = deserialize_route_response(buff, len, &result, route);
    if (ret < 0) {
        result = NLB_ERR_INVALID_RSP;
        goto EXIT_LABEL;
    }

    /* agent回复错误码 */
    if (result) {
        result = NLB_ERR_AGENT_ERR;
        goto EXIT_LABEL;
    }

    close(fd);
    return 0;

EXIT_LABEL:

    close(fd);
    return result;
}

/**
 * @brief 检查业务名是否有效
 * @info  必须两级业务名，"Login.ptlogin"
 */
bool check_service_name(const char *name)
{
    char *pos;

    if (NULL == name || name[0] == '\0') {
        return false;
    }

    pos = strchr(name, '.');
    if (NULL == pos || *(pos + 1) == '\0') {
        return false;
    }

    return true;
}

/**
 * @brief 通过业务名获取路由信息
 * @para  name:  输入参数，业务名字符串  "Login.ptlogin"
 * @      route: 输出参数，路由信息(ip地址，端口，端口类型)
 * @return  0: 成功  others: 失败
 */
int32_t getroutebyname(const char *name, struct routeid *route)
{
    struct api_routedata *route_data;

    if (!check_service_name(name) || NULL == route) {
        return NLB_ERR_INVALID_PARA;
    }

    route_data = get_route_data(name);
    if (NULL == route_data) {
        route_data = load_route_data(name);
    }

    if (NULL == route_data) {
        return get_route_from_agent(name, route);
    }

    return search_route(route_data, route);
}

/**
 * @brief 更新路由统计数据
 * @info  每次收发结束后，需要将成功与否、时延数据更新到统计数据
 * @para  name:  输入参数，业务名字符串  "Login.ptlogin"
 *        ip:    输入参数，IPV4地址
 *        failed:输入参数，>=1:失败次数 0->成功
 *        cost:  输入参数，时延
 */
int32_t updateroute(const char *name, uint32_t ip, int32_t failed, int32_t cost)
{
    uint32_t idx;
    struct api_routedata *route_data;
    struct shm_servers *svrs;
    struct server_info *server;

    if (!check_service_name(name)) {
        return NLB_ERR_INVALID_PARA;
    }

    route_data = get_route_data(name);
    if (NULL == route_data) {
        return NLB_ERR_NO_ROUTEDATA;
    }

    idx     = route_data->route_meta->index;
    svrs    = route_data->servers_data[idx];
    server  = get_server_by_ip(svrs, ip);
    if (NULL == server) {
        return NLB_ERR_NO_SERVER;
    }

    if (failed) {
        fetch_and_add(&server->failed, (uint32_t)failed);
        //fetch_and_add(&svrs->failed, (uint64_t)failed);
    } else {
        //fetch_and_add(&svrs->success, (uint32_t)1);
        //fetch_and_add(&svrs->cost, (uint64_t)cost);
        fetch_and_add(&server->success, (uint32_t)1);
        fetch_and_add_8(&server->cost, (uint64_t)cost);
    }

    return 0;
}


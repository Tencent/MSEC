
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
#include <string.h>
#include "commdef.h"
#include "list.h"
#include "nlbtime.h"
#include "hash.h"
#include "log.h"
#include "commtype.h"
#include "commstruct.h"
#include "routeproto.h"
#include "config.h"
#include "routeprocess.h"
#include "agent.h"
#include "zkservice.h"
#include "nlbapi.h"
#include "networking.h"
#include "nlbrand.h"

#define NLB_ROUTE_TASK_MAX      100  /* 单个业务最大路由请求数 */
#define NLB_ROUTE_TASK_HASHLEN  17   /* hash查找 */

/* 路由任务hash链表 */
static struct list_head route_task_hash[NLB_ROUTE_TASK_HASHLEN];

/* 路由请求任务数据结构 */
struct route_task
{
    struct list_head list_node; /* 用hash保存的链表节点 */
    uint64_t  ctime;            /* task创建时间 */
    uint64_t  mtime;            /* task修改时间 */
    int32_t   request_num;      /* 请求数       */
    char      service_name[NLB_SERVICE_NAME_LEN];
    struct sockaddr_in addr[NLB_ROUTE_TASK_MAX];
};

/**
 * @brief  获取指定业务的路由请求
 * @return 返回路由请求信息
 */
struct route_task *get_route_task(const char *name)
{
    struct route_task *task;
    uint32_t hash;

    hash = gen_hash_key(name) % NLB_ROUTE_TASK_HASHLEN;

    list_for_each_entry(task, &route_task_hash[hash], list_node)
    {
        if (!strncmp(name, task->service_name, NLB_SERVICE_NAME_LEN))
        {
            return task;
        }
    }

    return NULL;
}

/**
 * @brief 删除指定业务的路由任务
 * @info  在获取业务路由配置失败，或者配置为空的情况下调用
 */
void delete_route_task(const char *name)
{
    struct route_task *task;

    task = get_route_task(name);
    if (task) {
        list_del(&task->list_node);
        free(task);
    }
}

/**
 * @brief  创建路由请求任务
 * @info   如果该业务已经存在路由请求，将该次请求的地址加入任务中；如果任务已满，返回指定错误码
 *         否则，创建一个新的任务
 * @return =0 成功 <0 失败 =1 该业务路由任务已满
 */
int32_t create_route_task(const char *name, const struct sockaddr_in *addr)
{
    int32_t  ret;
    uint32_t hash;
    struct route_task *task;

    /* 查找业务路由请求任务 */
    task = get_route_task(name);
    if (task) {

        /* 路由请求任务满了，直接返回 */
        if (task->request_num >= NLB_ROUTE_TASK_MAX) {
            NLOG_DEBUG("route request to service (%s) is too much (%d).", name, task->request_num);
            return 1;
        }

        task->mtime = get_time_ms();
        memcpy(&task->addr[task->request_num++], addr, sizeof(*addr));
        return 0;
    }

    /* 创建新业务路由请求任务 */
    NLOG_DEBUG("recevice new service (%s) route request", name);
    task = calloc(1, sizeof(struct route_task));
    if (NULL == task) {
        NLOG_DEBUG("No memory!");
        return -1;
    }

    /* 初始化路由任务 */
    task->ctime = get_time_ms();
    task->mtime = task->ctime;
    strncpy(task->service_name, name, NLB_SERVICE_NAME_LEN);
    memcpy(&task->addr[task->request_num++], addr, sizeof(*addr));

    hash = gen_hash_key(name) % NLB_ROUTE_TASK_HASHLEN;
    list_add(&task->list_node, &route_task_hash[hash]);

    /* 开始加载新业务配置 */
    ret = load_service_config(name);
    if (ret < 0) {
        NLOG_ERROR("load_service_config failed, ret [%d]", ret);
        return -2;
    }

    return 0;
}

/**
 * @brief  随机获取本地路由
 * @return =-1 没有路由 =0 找到路由
 */
int32_t get_random_route(const char *name, struct routeid *id)
{
    struct agent_local_rdata *rdata = get_local_rdata(name);
    struct shm_servers *shm_servers;
    struct server_info *server;

    if (NULL == rdata) {
        NLOG_DEBUG("No this service (%s) local route", name);
        return -1;
    }

    shm_servers = rdata->servs_data[rdata->route_meta->index];
    server = shm_servers->svrs + nlb_rand()%shm_servers->server_num;

    id->ip   = server->server_ip;
    id->port = server->port[0];
    id->type = (NLB_PORT_TYPE)server->port_type;
    return 0;
}

/**
 * @brief 处理路由请求
 * @info  路由请求都是从API发送过来
 *        每个业务暂时最多只支持100个路由请求，如果超过，直接回复错误
 */
void process_route_request(int32_t listen_fd)
{
    int32_t ret;
    int32_t len;
    char    buff[1024];
    char    service_name[NLB_SERVICE_NAME_LEN];
    struct routeid id;
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (true) {
        /* 接收路由请求包 */
        len = recvfrom(listen_fd, buff, sizeof(buff), 0, &addr, &addr_len);
        if (len == -1) {
            NLOG_DEBUG("recv route request failed, [%m]");
            /* 不判断错误码，如果EAGAIN,EINTR错误，等待下次处理 */
            return;
        }

        /* 解路由请求包 */
        ret = deserialize_route_request(buff, len, service_name, sizeof(service_name));
        if (ret < 0) {
            NLOG_ERROR("Invalid route request package");
            continue;
        }

        NLOG_DEBUG("recevice service (%s) route request", service_name);

        /* 试着从本地获取路由，如果有，直接回复路由信息 */
        ret = get_random_route(service_name, &id);
        if (!ret) {
            ret = serialize_route_response(0, &id, buff, sizeof(buff));
            sendto(listen_fd, buff, ret, 0, &addr, addr_len);
            NLOG_DEBUG("send service (%s) route response", service_name);
            continue;
        }

        /* 创建路由请求任务 */
        ret = create_route_task(service_name, &addr);
        if (ret < 0) {
            NLOG_ERROR("create route request task failed");
            ret = serialize_route_response(1, NULL, buff, sizeof(buff));
            sendto(listen_fd, buff, ret, 0, &addr, addr_len);
            continue;
        }

        /* 创建路由请求task成功 */
        if (!ret) {
            continue;
        } 

        /* 该业务的路由请求已经满了，直接回复 */
        if (ret == 1) {
            ret = serialize_route_response(1, NULL, buff, sizeof(buff));
            sendto(listen_fd, buff, ret, 0, &addr, addr_len);
            continue;
        }
    }
}

/**
 * @brief  处理某个业务的路由任务
 * @info   在新获取到路由配置时调用
 */
void process_route_task(const char *name)
{
    int32_t  ret;
    uint32_t loop;
    char     buff[64*1024];
    struct routeid id;
    struct route_task *task;
    struct sockaddr_in *addr;

    task = get_route_task(name);
    if (NULL == task) {
        return;
    }

    /* 循环所有路由请求 */
    for (loop = 0; loop < task->request_num; loop++) {
        addr = &task->addr[loop];
        ret = get_random_route(name, &id);
        if (ret < 0) {
            ret = serialize_route_response(1, NULL, buff, sizeof(buff));
            continue;
        } else {
            ret = serialize_route_response(0, &id, buff, sizeof(buff));
        }

        ret = sendto(get_listen_fd(), buff, ret, 0, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
        if (ret == -1) {
            NLOG_ERROR("sendto request response failed, [127.0.0.1:%u] [%m]", ntohs(addr->sin_port));
            continue;
        }
    }

    list_del(&task->list_node);
    free(task);
}

/* 初始化路由任务 */
void init_route_task(void)
{
    int i;
    for (i = 0; i < NLB_ROUTE_TASK_HASHLEN; i++) {
        INIT_LIST_HEAD(&route_task_hash[i]);
    }
}




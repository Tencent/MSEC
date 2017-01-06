
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
#include <sys/mman.h>
#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
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
#include "zkheartbeat.h"
#include "zkplugin.h"
#include "zkloadreport.h"
#include "networking.h"
#include "utils.h"
#include "sysinfo.h"
#include "nlbfile.h"
#include "event.h"
#include "atomic.h"
#include "policy.h"

#define NLB_AGENT_ROUTE_DATA_HASH_LEN 107

static struct list_head agent_rdata_hash[NLB_AGENT_ROUTE_DATA_HASH_LEN];  /* 使用业务名计算hash */
static struct list_head agent_rdata_list;                                 /* agent路由数据链表  */

/* 多阶hash模数，20000个节点，15阶 */
static uint32_t mhash_mods[MAX_ROW_COUNT] = {4621, 3557, 2741, 2111, 1627, 1259, 971, 751, 577, 443, 347, 269, 211, 163, 352};

/**
 * @brief 获取agent路由数据链表
 */
struct list_head *get_rdata_list(void)
{
    return &agent_rdata_list;
}

/**
 * @brief 添加一个业务到agent本地数据
 */
struct agent_local_rdata *add_local_rdata(const char *name, struct shm_meta *meta,
                     struct shm_servers *s0, struct shm_servers *s1)
{
    uint32_t hash = gen_hash_key(name)%NLB_AGENT_ROUTE_DATA_HASH_LEN;
    struct agent_local_rdata *rdata = malloc(sizeof(struct agent_local_rdata));
    if (NULL == rdata) {
        NLOG_ERROR("No memory");
        return NULL;
    }

    strncpy(rdata->name, name, NLB_SERVICE_NAME_LEN);
    rdata->route_meta       = meta;
    rdata->servs_data[0]    = s0;
    rdata->servs_data[1]    = s1;
    rdata->watcher_flag     = false;
    rdata->update_time      = get_time_ms();

    list_add(&rdata->hash_node, &agent_rdata_hash[hash]);
    list_add_tail(&rdata->list_node, &agent_rdata_list);
    INIT_LIST_HEAD(&rdata->event_list);

    return rdata;
}

/**
 * @brief 获取本地管理路由数据信息
 */
struct agent_local_rdata *get_local_rdata(const char *name)
{
    uint32_t hash = gen_hash_key(name)%NLB_AGENT_ROUTE_DATA_HASH_LEN;
    struct agent_local_rdata *rdata;

    list_for_each_entry(rdata, &agent_rdata_hash[hash], hash_node)
    {
        if (!strncmp(name, rdata->name, NLB_SERVICE_NAME_LEN)) {
            return rdata;
        }
    }

    return NULL;
}

/**
 * @brief 清除所有业务的watch标记
 */
void clean_services_watching(void)
{
    int loop;
    struct agent_local_rdata *rdata;

    for (loop = 0; loop < NLB_AGENT_ROUTE_DATA_HASH_LEN; loop++)
    {
        list_for_each_entry(rdata, &agent_rdata_hash[loop], hash_node)
        {
            rdata->watcher_flag = 0;
        }
    }
}

/**
 * @brief 删除本地路由数据
 */
void delete_local_rdata(const char *name)
{
    uint32_t hash = gen_hash_key(name)%NLB_AGENT_ROUTE_DATA_HASH_LEN;
    struct agent_local_rdata *rdata;

    list_for_each_entry(rdata, &agent_rdata_hash[hash], hash_node)
    {
        if (!strncmp(name, rdata->name, NLB_SERVICE_NAME_LEN)) {
            list_del(&rdata->hash_node);
            list_del(&rdata->list_node);
            free(rdata);
        }
    }
}

/**
 * @brief 重新初始化多阶索引
 */
void calc_servers_hash(struct shm_servers *servers)
{
    uint32_t base, hash;
    int32_t  i, j;
    struct server_info *server;

    servers->mhash_order = MAX_ROW_COUNT;
    memcpy(servers->mhash_mods, mhash_mods, sizeof(mhash_mods));
    memset(servers->mhash_idx, 0xff, sizeof(servers->mhash_idx));

    for (i = 0; i < servers->server_num; i++) {
        server = &servers->svrs[i];
        base   = 0;

        for (j = 0; j < MAX_ROW_COUNT; j++) {
            hash = server->server_ip%servers->mhash_mods[j];

            if (servers->mhash_idx[base + hash] == 0xffffffff) {
                servers->mhash_idx[base + hash] = i;
                break;
            }

            base += servers->mhash_mods[j];
        }
    }
}

/**
 * @brief 清空服务器统计数据
 * @info  包括时延、成功数、失败数，清空后可以写入共享内存
 */
void clean_servers_stat(struct shm_servers *servers)
{
    uint32_t i;
    uint32_t svr_num = servers->server_num;
    struct server_info *server;


    servers->cost_total    = 0;
    servers->fail_total    = 0;
    servers->success_total = 0;

    for (i = 0; i < svr_num; i++) {
        server = &servers->svrs[i];

        server->failed  = 0;
        server->success = 0;
        server->cost    = 0;
    }
}

/**
 * @brief 交换两个server的信息
 */
void swap_server_info(struct server_info *server1, struct server_info *server2)
{
    struct server_info server_tmp;

    /* 相同地址，直接返回 */
    if (server1 == server2) {
        return;
    }

    memcpy(&server_tmp, server1, sizeof(struct server_info));
    memcpy(server1, server2, sizeof(struct server_info));
    memcpy(server2, &server_tmp, sizeof(struct server_info));
}

/**
 * @brief 计算服务器的权重信息
 */
void calc_servers_weight(struct shm_servers *servers)
{
    uint32_t i;
    uint32_t weight, base = 0;
    uint32_t dead_retrys;
    struct server_info *server;

    /* 设置每个服务器的权重基数 */
    for (i = 0; i < servers->server_num; i++) {
        server = &servers->svrs[i];
        if (server->dead_time) {
            break;
        }

        server->weight_base = base;
        base += server->weight_dynamic;
    }

    servers->dead_num         = servers->server_num - i;
    servers->weight_total     = base;
    servers->weight_dead_base = base;
    servers->dead_retry_times = 0;

    /* 计算死机服务器可以分配的权重，死机机器统一一个权重，随机选择一个 */
    if (servers->dead_num != 0) {
        weight      = (uint32_t)(servers->weight_total * servers->dead_retry_ratio + 1);
        dead_retrys = (uint32_t)((servers->success_total + servers->fail_total)*servers->dead_retry_ratio);
        servers->weight_total      += max(weight, (uint32_t)1);
        servers->dead_retry_times   = max(dead_retrys, (uint32_t)1);
    }
}

void _shaping_servers(struct shm_servers *servers, double success_ratio_base, bool weight_dec)
{
    int32_t  begin, end;
    uint16_t weight;
    uint32_t svr_num = servers->server_num;
    uint64_t single_req_total;
    double   single_success_ratio, ratio;

    struct server_info *server;

    /* 计算每一个服务器权重和死机状态 */
    end   = (int32_t)svr_num - 1;
    begin = 0;
    while ((begin <= end) && (end >= 0)) {
        server              = &servers->svrs[begin];
        single_req_total    = server->failed + server->success;

        /* 如果没有处理过请求，权重信息保持不变 */
        if (!single_req_total) {
            if (server->dead_time) {
                server->weight_dynamic = 0;
                swap_server_info(server, &servers->svrs[end]);
                end--;
            } else {
                begin++;
            }
            continue;
        }

        single_success_ratio = ((double)server->success)/single_req_total;

        /* 单机成功率大于基准成功率 */
        if (single_success_ratio >= success_ratio_base) {
            /* 死机机器，重新修改权重 */
            if (server->dead_time) {
                server->dead_time = 0;
                weight            = (uint16_t)(server->weight_static * servers->resume_weight_ratio);
                server->weight_dynamic  = max(weight, (uint16_t)1);
                begin++;
                continue;
            }

            /* 如果大于基准成功率，增加权重 */
            if (server->weight_static != server->weight_dynamic) {
                weight = (uint16_t)(server->weight_static * servers->weight_incr_ratio);
                weight = max(weight, (uint16_t)1);
                server->weight_dynamic += weight;
                server->weight_dynamic  = min(server->weight_static, server->weight_dynamic);
            }
            
            begin++;

            continue;
        }

        /* 单机成功率为0 */
        if (single_success_ratio <= 0.00001) {
            if (server->dead_time == 0) {
                server->dead_time = get_time_ms();
            }

            server->weight_dynamic  = 0;
            swap_server_info(server, &servers->svrs[end]);
            end--;
            continue;
        }

        if (!weight_dec) {
            begin++;
            continue;
        }

        /* 单机成功率低于平均成功率,减小权重 */
        ratio = single_success_ratio;
        ratio = ratio * ratio;
        server->weight_dynamic = (uint16_t)(server->weight_dynamic * ratio);

        /* 降低权重后，权重为零 */
        if (0 == server->weight_dynamic) {
            if (0 == server->dead_time) {
                server->dead_time = get_time_ms();
            }

            swap_server_info(server, &servers->svrs[end]);
            end--;
            continue;
        }

        begin++;
    }
}

uint32_t calc_weight_low_num(struct shm_servers *servers)
{
    struct server_info *info;
    float water_mark = servers->weight_low_watermark;
    uint32_t idx, cnt = 0;

    for (idx = 0; idx < servers->server_num; idx ++) {
        info = servers->svrs + idx;
        if (((float)info->weight_dynamic)/info->weight_static < water_mark) {
            cnt++;
        }
    }

    return cnt;
}

/**
 * @brief 对服务器信息做调整
 * @info  包括调整动态权重，死机等信息
 */
void shaping_servers(struct shm_servers *servers)
{
    uint32_t svr_num = servers->server_num;
    uint64_t req_total;
    double   success_rate;
    float    weight_low_real_ratio;

    /* 没有服务器，不用计算 */
    if (!svr_num) {
        return;
    }

    weight_low_real_ratio = ((float)servers->weight_low_num)/svr_num;
    if (weight_low_real_ratio <= servers->weight_low_ratio) {
        _shaping_servers(servers, servers->success_ratio_base, true);
    } else {
        /* 计算平均成功率，用平均成功率计算权重 */
        req_total = servers->fail_total + servers->success_total;
        if (req_total == 0) {
            success_rate = 100.0;
        } else {
            success_rate = ((double)servers->success_total)/req_total;
        }

        success_rate = min(success_rate, (double)servers->success_ratio_base);
        _shaping_servers(servers, success_rate, false);
    }

    servers->weight_low_num = calc_weight_low_num(servers);

    #if 0
    int32_t  begin, end;
    uint16_t weight;
    uint32_t svr_num = servers->server_num;
    uint64_t req_total, single_req_total;
    double   success_rate, single_success_rate, rate;
    double   cost_avg; //, single_cost_avg;


    /* 计算总成功率 */
    req_total = servers->fail_total + servers->success_total;
    if (req_total == 0) {
        success_rate = 100.0;
    } else {
        success_rate = ((double)servers->success_total)/req_total;
    }

    /* 计算总时延 */
    if (servers->success_total == 0) {
        cost_avg = (double)0;
    } else {
        cost_avg = ((double)servers->cost_total)/servers->success_total;
    }

    /* 计算每一个服务器权重和死机状态 */
    end   = (int32_t)svr_num - 1;
    begin = 0;
    while ((begin <= end) && (end >= 0)) {
        server              = &servers->svrs[begin];
        single_req_total    = server->failed + server->success;

        /* 如果没有处理过请求，权重信息保持不变 */
        if (!single_req_total) {
            if (server->dead_time) {
                server->weight_dynamic = 0;
                swap_server_info(server, &servers->svrs[end]);
                end--;
            } else {
                begin++;
            }
            continue;
        }

        single_success_rate = (server->success * 1.0)/single_req_total;

        /* 成功率小于10%,认为死机，死机的机器移到最后面统一划分权重 */
        if (single_success_rate < success_rate_min) {
            if (!server->dead_time) {
                server->dead_time = get_time_ms();
            }
            server->weight_dynamic = 0;
            swap_server_info(server, &servers->svrs[end]);
            end--;
            continue;
        }

        /* 单机成功率大于平均成功率 */
        if ((single_success_rate >= success_rate)
            || (single_success_rate > success_rate_base)) {
            /* 死机机器，重新修改权重 */
            if (server->dead_time) {
                server->dead_time = 0;
                weight            = (uint16_t)(server->weight_static*weight_incr_step);    // 先设置为5%的权重
                server->weight_dynamic  = max(weight, (uint16_t)1);
                begin++;
                continue;
            }

            /* 如果大于基准成功率，增加权重 */
            if ((single_success_rate > success_rate_base)
                && (server->weight_static != server->weight_dynamic)) {
                weight = (uint16_t)(server->weight_static * weight_incr_step);
                weight = max(weight, (uint16_t)1);
                server->weight_dynamic += weight;
                server->weight_dynamic  = min(server->weight_static, server->weight_dynamic);
            }
            
            begin++;

            continue;
        }

        /* 单机成功率低于平均成功率,减小权重 */
        rate = single_success_rate/success_rate;
        rate = rate*rate;
        server->weight_dynamic = (uint16_t)(server->weight_dynamic*rate);

        /* 降低权重后，权重为零 */
        if (0 == server->weight_dynamic) {
            if (0 == server->dead_time) {
                server->dead_time = get_time_ms();
            }

            swap_server_info(server, &servers->svrs[end]);
            end--;
            continue;
        }

        begin++;
    }

    #endif
}

/**
 * @brief 拷贝服务器信息数据
 * @info  拷贝服务器数据的同时，计算统计数据
 */
void copy_servers(struct shm_servers *dst_svrs, struct shm_servers *src_svrs, uint32_t lower)
{
    uint32_t i, j;
    uint32_t svr_num = src_svrs->server_num;
    struct server_info *dst_svr;
    struct server_info *src_svr;

    dst_svrs->cost_total    = 0;
    dst_svrs->fail_total    = 0;
    dst_svrs->success_total = 0;

    if (src_svrs->version == NLB_SHM_VERSION1) {
        dst_svrs->server_num            = svr_num;
        dst_svrs->policy                = src_svrs->policy;
        dst_svrs->weight_total          = src_svrs->weight_total;
        dst_svrs->weight_static_total   = src_svrs->weight_static_total;
        dst_svrs->shaping_request_min   = src_svrs->shaping_request_min;
        dst_svrs->success_ratio_base    = src_svrs->success_ratio_base;
        dst_svrs->success_ratio_min     = src_svrs->success_ratio_min;
        dst_svrs->resume_weight_ratio   = src_svrs->resume_weight_ratio;
        dst_svrs->dead_retry_ratio      = src_svrs->dead_retry_ratio;
        dst_svrs->weight_low_watermark  = src_svrs->weight_low_watermark;
        dst_svrs->weight_low_ratio      = src_svrs->weight_low_ratio;
        dst_svrs->weight_incr_ratio     = src_svrs->weight_incr_ratio;
        dst_svrs->version               = NLB_SHM_VERSION1;        
        dst_svrs->weight_low_num        = src_svrs->weight_low_num;
    } else {
        dst_svrs->server_num            = svr_num;
        dst_svrs->policy                = src_svrs->policy;
        dst_svrs->weight_total          = src_svrs->weight_total;
        dst_svrs->weight_static_total   = 0;
        dst_svrs->shaping_request_min   = NLB_SHAPING_REQUEST_MIN;
        dst_svrs->success_ratio_base    = NLB_SUCCESS_RATIO_BASE;
        dst_svrs->success_ratio_min     = NLB_SUCCESS_RATIO_MIN;
        dst_svrs->resume_weight_ratio   = NLB_RESUME_WEIGHT_RATIO;
        dst_svrs->dead_retry_ratio      = NLB_DEAD_RETRY_RATIO;
        dst_svrs->weight_low_watermark  = NLB_WEIGHT_LOW_WATERMARK;
        dst_svrs->weight_low_ratio      = NLB_WEIGHT_LOW_RATIO;
        dst_svrs->weight_incr_ratio     = NLB_WEIGHT_INCR_RATIO;
        dst_svrs->version               = NLB_SHM_VERSION1;
        dst_svrs->weight_low_num        = src_svrs->weight_low_num;
    }

    for (i = 0; i < svr_num; i++) {
        dst_svr = &dst_svrs->svrs[i];
        src_svr = &src_svrs->svrs[i];

        dst_svr->server_ip      = src_svr->server_ip;
        dst_svr->weight_static  = src_svr->weight_static;
        dst_svr->weight_dynamic = src_svr->weight_dynamic;
        dst_svr->port_type      = src_svr->port_type;
        dst_svr->port_num       = src_svr->port_num;

        dst_svr->dead_time      = src_svr->dead_time;
        if ((dst_svr->dead_time != 0) || ((src_svr->failed + src_svr->success) >= lower)) {
            dst_svr->failed     = return_and_set(&src_svr->failed, (uint32_t)0);
            dst_svr->success    = return_and_set(&src_svr->success, (uint32_t)0);
            dst_svr->cost       = return_and_set_8(&src_svr->cost, (uint64_t)0);
        } else {
            dst_svr->failed     = 0;
            dst_svr->success    = 0;
            dst_svr->cost       = 0;
        }

        for (j = 0; j < src_svr->port_num; j++) {
            dst_svr->port[j]    = src_svr->port[j];
        }

        dst_svrs->cost_total   += dst_svr->cost;
        dst_svrs->fail_total   += dst_svr->failed;
        dst_svrs->success_total+= dst_svr->success;
    }
}


/**
 * @brief 拷贝指定的服务器信息数据
 * @info  拷贝指定的服务器数据的同时，计算统计数据
 */
void copy_specified_servers(struct shm_servers *dst_svrs, struct shm_servers *src_svrs, uint32_t lower)
{
    uint32_t i;
    uint32_t svr_num = dst_svrs->server_num;
    struct server_info *dst_svr;
    struct server_info *src_svr;

    dst_svrs->cost_total    = 0;
    dst_svrs->fail_total    = 0;
    dst_svrs->success_total = 0;
    dst_svrs->weight_low_num= 0;

    for (i = 0; i < svr_num; i++) {
        dst_svr = &dst_svrs->svrs[i];
        src_svr = get_server_info(dst_svr->server_ip, src_svrs);
        if (NULL == src_svr) {
            dst_svr->cost       = 0;
            dst_svr->failed     = 0;
            dst_svr->success    = 0;
            dst_svr->dead_time  = 0;
            dst_svr->weight_dynamic = dst_svr->weight_static;
            continue;
        }

        dst_svr->dead_time      = src_svr->dead_time;

        if ((dst_svr->dead_time != 0) || ((src_svr->failed + src_svr->success) >= lower)) {
            dst_svr->failed     = return_and_set(&src_svr->failed, (uint32_t)0);
            dst_svr->success    = return_and_set(&src_svr->success, (uint32_t)0);
            dst_svr->cost       = return_and_set_8(&src_svr->cost, (uint64_t)0);
        } else {
            dst_svr->failed     = 0;
            dst_svr->success    = 0;
            dst_svr->cost       = 0;
        }
        dst_svr->weight_dynamic = src_svr->weight_dynamic;

        dst_svrs->cost_total   += dst_svr->cost;
        dst_svrs->fail_total   += dst_svr->failed;
        dst_svrs->success_total+= dst_svr->success;
    }
}



/**
 * @brief 通过IP获取服务器信息
 */
struct server_info *get_server_info(uint32_t ip, struct shm_servers *servers)
{
    uint32_t i;
    uint32_t hash, idx, base = 0;
    struct server_info *server;

    for (i = 0; i < servers->mhash_order; i++) {
        hash    = ip % servers->mhash_mods[i];
        idx     = servers->mhash_idx[hash + base];
        if (idx == 0xffffffff) {
            continue;
        }

        if (idx >= NLB_SERVER_MAX) {
            NLOG_ERROR("Invalid service local config");
            return NULL;
        }

        server  = &servers->svrs[idx];
        if (server->server_ip == ip) {
            return server;
        }

        base += servers->mhash_mods[i];
    }

    return NULL;
}

/**
 * @brief 合并统计数据
 */
void merge_servers_stat(struct shm_servers *dst_svrs, struct shm_servers *src_svrs)
{
    uint32_t i;
    uint32_t svr_num = dst_svrs->server_num;
    struct server_info *dst_svr;
    struct server_info *src_svr;

    for (i = 0; i < svr_num; i++) {
        dst_svr = &dst_svrs->svrs[i];
        src_svr = get_server_info(dst_svr->server_ip, src_svrs);

        if (NULL == src_svr) {
            continue;
        }

        fetch_and_add(&dst_svr->failed, src_svr->failed);
        fetch_and_add(&dst_svr->success, src_svr->success);
        fetch_and_add_8(&dst_svr->cost, src_svr->cost);
    }
}

/**
 * @brief 检查服务器是否真死了
 */
bool check_server_real_dead(struct server_info *server, float success_ratio)
{
    uint32_t total = server->success + server->failed;

    if (total == 0) {
        return true;
    }

    if ((server->success*1.0)/total < success_ratio) {
        return true;
    }

    return false;
}

/**
 * @brief 处理节点事件
 * @info  如果节点死机，清除统计信息并设置死机标记(死机时间戳)
 * @      如果节点恢复，清除统计信息和死机标记，并将统计信息清零，权重设置为10%
 */
void handle_node_events(struct shm_servers *shm_servers, struct list_head *event_list)
{
    struct server_info *server;
    struct event *event;
    struct event *tmp;
    uint32_t ip;

    list_for_each_entry_safe(event, tmp, event_list, list_node)
    {
        /* 如果策略不需要处理节点心跳变化，直接删除事件 */
        if (!check_need_heartbeat(shm_servers->policy)) {
            delete_event(event, false);
            continue;
        }

        ip      = (uint32_t)(long)event->ctx;
        server  = get_server_info(ip, shm_servers);
        if (NULL == server) {
            NLOG_ERROR("unkown node events, no this server [%s]", inet_ntoa(*(struct in_addr *)&ip));
            delete_event(event, false);
            continue;
        }

        /* 设置死机标记和动态权重 */
        if (event->type == NLB_EVENT_TYPE_NODE_DEAD) {
            if (check_server_real_dead(server, shm_servers->success_ratio_min)) {
                server->dead_time      = get_time_ms();
                server->weight_dynamic = 0;
            }
            NLOG_DEBUG("process node dead event, [%s] new weight [%u]",
                       inet_ntoa(*(struct in_addr *)&ip), server->weight_dynamic);
        }else if (event->type == NLB_EVENT_TYPE_NODE_RESUME) {
            NLOG_DEBUG("process node resume event, [%s]", inet_ntoa(*(struct in_addr *)&ip));
            if (server->dead_time) {
                server->dead_time = 0;
                server->weight_dynamic = max((uint16_t)1, (uint16_t)(server->weight_static*shm_servers->resume_weight_ratio));
            } else { /* 可能有连续两次事件导致不一致，可以忽略 */
                delete_event(event, false);
                continue;
            }
        }else {
            NLOG_ERROR("unkown events, [%d]", event->type);
            //delete_event(event, false);
            continue;
        }

        /* 清空本次统计，该节点不会再做shaping */
        shm_servers->cost_total    -= server->cost;
        shm_servers->fail_total    -= server->failed;
        shm_servers->success_total -= server->success;
        server->cost    = 0;
        server->failed  = 0;
        server->success = 0;

        delete_event(event, false);
    }
}

void dumpevent(struct list_head *event_list)
{
    struct event *event;

    NLOG_DEBUG("event list   :");
    NLOG_DEBUG("  1: loadservice, 2: node dead, 3: node alive");
    list_for_each_entry(event, event_list, list_node)
    {
        NLOG_DEBUG("  type: %d ctx: 0x%lx", event->type, (long)event->ctx);
    }
}

void dumpservers(struct shm_servers *servers)
{
    uint32_t i;
    struct server_info *server;

    NLOG_DEBUG("servers info :");
    NLOG_DEBUG("  cost_total:%lu  success_total:%lu fail_total:%lu server_num:%u dead_num:%u dead_retry_times:%u\n"
              "  weight_total: %u weight_dead_base:%u mhash_order:%u\n"
              "servers:", 
              servers->cost_total, servers->success_total, servers->fail_total, servers->server_num, servers->dead_num,
              servers->dead_retry_times, servers->weight_total, servers->weight_dead_base, servers->mhash_order);
    for (i = 0; i < servers->server_num; i++) {
        server = servers->svrs + i;
        NLOG_DEBUG("  ip:%u weight_base:%u weight_static:%u weight_dynamic:%u port_type:%u port_num:%u dead_time:%lu failed:%u success:%u cost:%lu",
                  server->server_ip, server->weight_base, server->weight_static, server->weight_dynamic, server->port_type,
                  server->port_num, server->dead_time, server->failed, server->success, server->cost);
    }
}

void dumpinfo(struct agent_local_rdata *rdata)
{
    NLOG_DEBUG("------------------service dump info(%ld) ----------", time(NULL));
    NLOG_DEBUG("service name : %s", rdata->name);
    NLOG_DEBUG("watch_flag   : %d", rdata->watcher_flag);
    NLOG_DEBUG("index        : %u", rdata->route_meta->index);
    NLOG_DEBUG("mtime        : %lu", rdata->route_meta->mtime);

    dumpevent(&rdata->event_list);
    dumpservers(rdata->servs_data[rdata->route_meta->index]);
}


/**
 * @brief 更新业务配置
 * @param new_shm_servers --> 新加载的服务器信息
 * @return <0 更新失败 =0 更新成功
 */
int32_t update_config(struct agent_local_rdata *rdata, struct shm_servers *new_shm_servers, uint64_t mtime)
{
    uint32_t idx, new_idx, server_num;
    uint32_t data_len;
    struct shm_servers *cur_shm_servers;
    struct shm_servers *next_shm_servers;
    struct shm_servers *servers;
    struct shm_meta *meta = rdata->route_meta;
    struct list_head *event_list;

    idx              = meta->index;
    new_idx          = (idx+1)%2;
    cur_shm_servers  = rdata->servs_data[idx];
    next_shm_servers = rdata->servs_data[new_idx];
    event_list       = &rdata->event_list;

    //dumpinfo(rdata);

    NLOG_DEBUG("update service [%s] config", rdata->name);

    /* new_shm_servers非空，表示新加载的配置服务器信息，需要拷贝指定服务器的数据信息 */
    if (new_shm_servers) {
        servers     = new_shm_servers;
        server_num  = servers->server_num;
        data_len    = sizeof(struct shm_servers) + sizeof(struct server_info) * server_num;
        meta->mtime = mtime;
        copy_specified_servers(servers, cur_shm_servers, servers->shaping_request_min);
    } else {
        server_num  = cur_shm_servers->server_num;
        data_len    = sizeof(struct shm_servers) + sizeof(struct server_info) * server_num;

        /* 申请临时内存用于计算新配置信息 */
        servers = (struct shm_servers *)calloc(1, data_len);
        if (NULL == servers) {
            NLOG_ERROR("No memory");
            return -1;
        }

        /* 拷贝所有共享内存服务器信息到私有内存，同时计算统计信息 */
        copy_servers(servers, cur_shm_servers, servers->shaping_request_min);
        if (!servers->server_num) {
            free(servers);
            return 0;
        }
    }

    /* 处理节点事件 */
    if (!list_empty(event_list)) {
        /* 计算hash，处理节点事件时，需要用hash做查找 */
        calc_servers_hash(servers);
        /* 处理节点事件(死机和恢复) */
        handle_node_events(servers, &rdata->event_list);
    }

    /* 计算动态权重和死机信息 */
    shaping_servers(servers);

    /* 清除统计数据 */
    clean_servers_stat(servers);

    /* 统一计算每一个服务器的权重基数，以及死机机器的权重 */
    calc_servers_weight(servers);

    /* 计算多阶hash */
    calc_servers_hash(servers);

    /* 拷贝新服务器数据到共享内存 */
    memcpy(next_shm_servers, servers, data_len);

    /* 设置新寻址服务器数据 */
    mb();
    meta->index = new_idx;

    /* 合并统计数据到新服务器数据里面 */
    merge_servers_stat(next_shm_servers, cur_shm_servers);

    //dumpinfo(rdata);

    /* 如果为NULL，表示为本函数malloc的内存 */
    if (NULL == new_shm_servers)
        free(servers);

    NLOG_DEBUG("update service [%s] config end", rdata->name);

    return 0;
}

/**
 * @brief 定时处理业务配置变更函数
 */
int32_t process_service(struct agent_local_rdata *rdata)
{
    int32_t ret;
    struct list_head *event_list;
    struct event *event_first;

    if (NULL == rdata) {
        return -1;
    }

    /* 如果zookeeper没有连接上，直接更新配置 */
    if (!zk_connected()) {
        update_config(rdata, NULL, 0);
        return 0;
    }

    event_list = &rdata->event_list;
    if (!list_empty(event_list)) {
        event_first = list_first_entry(event_list, struct event, list_node);
        /* 需要重新load配置后更新 */
        if (event_first->type == NLB_EVENT_TYPE_LOAD_SERVICE) {
            ret = load_service_config(rdata->name);
            if (ret < 0) {
                NLOG_ERROR("load_service_config failed, ret [%d]", ret);
                return -2;
            }
            delete_event(event_first, false);
            return 0;
        }
    }

    update_config(rdata, NULL, 0);

    return 0;
}

/**
 * @brief 设置监视事件
 */
void set_services_watcher(void)
{
    struct agent_local_rdata *rdata;

    list_for_each_entry(rdata, &agent_rdata_list, list_node) {
        set_service_watcher(rdata);
    }
}

/**
 * @brief 处理业务的配置,定时调用
 */
void handle_service(void)
{
    uint64_t now;
    struct agent_local_rdata *rdata;

    if (list_empty(&agent_rdata_list)) {
        return;
    }

    /* 每次只更新一个业务，防止出现多个业务同时更新占用CPU时间过长问题 */
    now   = get_time_ms();
    rdata = list_first_entry(&agent_rdata_list, struct agent_local_rdata, list_node);
    if (rdata->update_time + 5000 <= now) { // 5秒更新一次数据
        process_service(rdata);
        set_service_watcher(rdata);
        set_service_nodes_wather(rdata);
        rdata->update_time = now;

        /* 将处理后的业务移到链表尾部 */
        list_del(&rdata->list_node);
        list_add_tail(&rdata->list_node, &agent_rdata_list);
    } 
}

/**
 * @brief 加载本地业务到agent私有内存
 */
int32_t load_local_service(const char *name)
{
    int32_t  result;
    uint32_t mmaplen, mmaplen0, mmaplen1;
    void *meta         = NULL;
    void *server_data0 = NULL ;
    void *server_data1 = NULL;

    NLOG_INFO("load local service (%s)", name);

    /* 加载元数据信息 */
    meta = load_meta_data(name, &mmaplen);
    if (NULL == meta) {
        NLOG_ERROR("Load meta data for %s failed", name);
        result = -1;
        goto ERR_EXIT;
    }

    /* 加载server信息 */
    server_data0 = load_server_data(name, 0, &mmaplen0);
    if (NULL == server_data0) {
        NLOG_ERROR("Load server data for %s failed", name);
        result = -2;
        goto ERR_EXIT;
    }

    server_data1 = load_server_data(name, 1, &mmaplen1);
    if (NULL == server_data1) {
        NLOG_ERROR("Load server data for %s failed", name);
        result = -3;
        goto ERR_EXIT;
    }

    /* 添加信息到agent本地数据管理 */
    add_local_rdata(name, meta, server_data0, server_data1);

    return 0;

ERR_EXIT:

    if (meta)
        munmap(meta, mmaplen);
    if (server_data0)
        munmap(server_data0, mmaplen0);
    if (server_data1)
        munmap(server_data1, mmaplen1);

    return result;
}

/**
 * @brief 加载某个一级业务名下的所有二级目录业务
 */
int32_t load_2_level_services(const char *base, const char *lvl_1_name)
{
    int32_t ret;
    DIR *dir;
    struct dirent *ptr;
    char path[NLB_PATH_MAX_LEN];
    char name[NLB_SERVICE_NAME_LEN];

    if (NULL == path || NULL == lvl_1_name) {
        NLOG_DEBUG("Invalid input parameter for load_2_level_services.");
        return -1;
    }

    /* 组装目录名 */
    ret = snprintf(path, NLB_PATH_MAX_LEN, "%s/%s", base, lvl_1_name);
    if (ret >= NLB_PATH_MAX_LEN) {
        NLOG_DEBUG("Invalid directory path for load_2_level_services.");
        return -2;
    }

    dir = opendir(path);
    if (NULL == dir) {
        NLOG_ERROR("Open directory failed, [%m].");
        return -3;
    }

    /* 循环读取所有二级业务 */
    while ((ptr = readdir(dir)) != NULL) {
        if ((strcmp(ptr->d_name,".") == 0)
            || (strcmp(ptr->d_name,"..") == 0)) {
            continue;
        }

        //if (ptr->d_type == DT_DIR) {
            ret = snprintf(name, NLB_SERVICE_NAME_LEN, "%s.%s", lvl_1_name, ptr->d_name);
            if (ret >= NLB_SERVICE_NAME_LEN) {
                continue;
            }

            /* 加载某个指定业务的本地数据 */
            load_local_service(name);
        //}
    }

    closedir(dir);

    return 0;
}

/**
 * @brief 加载所有本地业务数据
 */
int32_t load_1_level_services(void)
{
    DIR *dir;
    struct dirent *ptr;
    char path[NLB_PATH_MAX_LEN] = NLB_NAME_BASE_PATH;

    /* 打开根目录 */
    dir = open_and_create_dir(path);
    if (NULL == dir) {
        NLOG_ERROR("Open directory(%s) failed, [%m]", path);
        return -1;
    }

    /* 循环读取所有一级目录 */
    while ((ptr = readdir(dir)) != NULL) {
        if ((strcmp(ptr->d_name,".") == 0) || (strcmp(ptr->d_name,"..") == 0)) {
            continue;
        }

        //if (ptr->d_type == DT_DIR) {
            /* 加载一级目录下所有二级目录 */
            load_2_level_services(path, ptr->d_name);
        //}
    }

    closedir(dir);
    return 0;
}

/**
 * @brief 把服务器配置和元数据配置写入文件
 */
int32_t write_config_2_file(const struct shm_meta *meta, struct shm_servers *servers)
{
    int32_t  ret = 0;

    ret += write_server_data(meta->name, 0, servers);
    ret += write_server_data(meta->name, 1, servers);
    ret += write_meta_data(meta);  // 最后写元数据配置文件
    if (ret < 0) {
        NLOG_ERROR("Write %s config failed, ret [%d] [%m]", meta->name, ret);
        return -1;
    }

    return 0;
}

/**
 * @brief  添加一个新的业务到本地
 * @return =0 成功 <0 失败
 */
int32_t add_new_service(const char *name, struct shm_servers *shm_srvs, uint64_t mtime)
{
    int32_t  ret, result = 0;
    int32_t  i, base = 0;
    char     path[NLB_PATH_MAX_LEN];
    struct shm_meta *meta = NULL;
    struct server_info *server;

    NLOG_INFO("add new service (%s)", name);

    /* 创建目录 */
    ret = get_service_dir(name, path, sizeof(path));
    if (ret < 0) {
        return -1;
    }

    ret = mkdir_recursive(path);
    if (ret < 0) {
        NLOG_ERROR("mkdir (%s) failed, [%m]", path);
        return -2;
    }

    /* 初始化元数据信息 */
    meta = calloc(1, sizeof(*meta));
    if (NULL == meta) {
        NLOG_ERROR("No memory.");
        return -3;
    }

    meta->ctime = 0;
    meta->mtime = mtime;
    meta->index = 0;
    strncpy(meta->name, name, NLB_SERVICE_NAME_LEN);

    /* 初始化寻址权重信息 */
    for (i = 0; i < shm_srvs->server_num; i++) {
        server = shm_srvs->svrs + i;
        server->weight_dynamic = server->weight_static;
        server->weight_base    = base;
        base += server->weight_dynamic;
    }

    shm_srvs->dead_num          = 0;
    shm_srvs->weight_total      = base;
    shm_srvs->weight_dead_base  = base;

    /* 更新hash信息 */
    calc_servers_hash(shm_srvs);

    /* 写配置到文件 */
    ret = write_config_2_file(meta, shm_srvs);
    if (ret < 0) {
        result = -4;
        goto ERR_RET;
    }

    /* 加载本地业务到内存中 */
    ret = load_local_service(name);
    if (ret < 0) {
        result = -5;
        goto ERR_RET;
    }

    return 0;

ERR_RET:
    if (meta)
        free(meta);

    return result;
}


/**
 * @brief 初始化客户端agent
 */
int32_t init_client_agent(void)
{
    int32_t ret;
    int32_t i;

    /* 初始化全局变量 */
    for (i = 0; i < NLB_AGENT_ROUTE_DATA_HASH_LEN; i++) {
        INIT_LIST_HEAD(&agent_rdata_hash[i]);
    }

    INIT_LIST_HEAD(&agent_rdata_list);

    /* 初始化路由任务 */
    init_route_task();

    /* load所有本地业务配置 */
    ret = load_1_level_services();
    if (ret < 0) {
        NLOG_ERROR("Load client agent services failed, ret [%d]", ret);
        return -1;
    }

    /* 初始化节点监视 */
    heartbeat_data_init();

    /* 设置业务监视事件 */
    set_services_watcher();

    return 0;
}

/**
 * @brief 初始化服务端agent
 */
void init_server_agent(void)
{
    /* 初始化系统信息，做CPU/MEM分析上报 */
    init_sysinfo();

    /* 创建心跳临时节点 */
    create_heartbeat_node(get_local_ip());

    /* 创建负载上报节点 */
    create_loadreport_node(get_local_ip());

}

/**
 * @brief Agent统一初始化函数
 */
int32_t init(void)
{
    int32_t ret  = 0;
    int32_t mode = get_worker_mode();

    /* 网络初始化 */
    ret = network_init();
    if (ret) {
        NLOG_ERROR("Network init faild, ret [%d]", ret);
        return -1;
    }

    /* 初始化服务提供方 */
    if (mode == SERVER_MODE || mode == MIX_MODE) {
        init_server_agent();
    }

    /* 初始化服务使用方 */
    if (mode == CLIENT_MODE || mode == MIX_MODE) {
        ret = init_client_agent();
        if (ret) {
            NLOG_ERROR("Init client agent failed, ret [%d]", ret);
            return -3;
        }
    }

    NLOG_DEBUG("Agent[%d] init success...\n", getpid());
    return 0;
}

/**
 * @brief Agent主处理函数
 */
void run(void)
{
    static uint64_t last_time;
    uint64_t now;

    network_poll();
    network_process();

    /* 检查是否需要退出 */
    if (quit()) {
        NLOG_ERROR("Agent recevice quit signal...");
        nlb_zk_close();
        exit(0);
    }

    /* 检查zookeeper是否在非法状态(expire/authfail)，需要重启 */
    if (zk_need_reinit()) {
        nlb_zk_reinit();
    }

    /* 客户模式: 定时处理业务配置变更 */
    if ((get_worker_mode() == CLIENT_MODE)
        || (get_worker_mode() == MIX_MODE)) {
        handle_service();
    }

    /* 服务模式 */
    now = get_time_s();
    if ((get_worker_mode() == SERVER_MODE)
        || (get_worker_mode() == MIX_MODE)) {
        if (now >= last_time + 20) {
            /* 上报负载 */
            load_report(get_local_ip());

            /* 检查心跳节点是否创建 */
            //if (!check_heartbeat_created()) {
                create_heartbeat_node(get_local_ip());
            //}

            last_time = now;
        }
    }
}



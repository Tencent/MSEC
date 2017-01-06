
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


#ifndef _COMMSTRUCT_H_
#define _COMMSTRUCT_H_

/**
 * @filename commstruct.h
 */
#include <stdint.h>
#include "hash.h"

#define NLB_SERVER_MAX          10000       /* 最大服务器数: 1万 */
#define NLB_SERVER_HASH_LEN     20000
#define NLB_WEIGHT_MAX          10000       /* 权重最大值 */
#define NLB_WEIGHT_MIN          100         /* 权重最小值 */
#define NLB_SERVICE_NAME_LEN    256         /* 业务名最大长度 */
#define NLB_PORT_MAX            8           /* 最大端口个数 */

#define NLB_SHAPING_REQUEST_MIN     (10)    /* 统计周期最小请求数,默认10个 */
#define NLB_SUCCESS_RATIO_BASE      (0.98)  /* 成功率基准，一般较高，默认98% */
#define NLB_SUCCESS_RATIO_MIN       (0.6)   /* 最小成功率，小于该值，不要选择对应的机器，随机选择其它机器 */
#define NLB_RESUME_WEIGHT_RATIO     (0.1)   /* 死机回复后设置的权重比例，默认10% */
#define NLB_DEAD_RETRY_RATIO        (0.01)  /* 死机机器探测包比例 */
#define NLB_WEIGHT_LOW_WATERMARK    (0.50)  /* 机器权重低于该值，会被标记为低权重机器 */
#define NLB_WEIGHT_LOW_RATIO        (0.50)  /* 低权重机器总数低于该值，不降低权重，只给大于平均成功率的机器加权重 */
#define NLB_WEIGHT_INCR_RATIO       (0.05)  /* 每次增加权重的比例 */

#define NLB_SHM_VERSION1            (1)     /* 共享内存版本号 */

/*********** 所有数据结构都已经手工8字节对齐，兼容32/64位CPU ********/

#pragma pack(push, 1)

/* 路由数据元数据数据结构 */
struct shm_meta
{
    volatile uint64_t ctime;        /* 创建时间 */
    volatile uint64_t squence;      /* 配置号   */
    volatile uint64_t mtime;        /* 修改时间 */
    volatile uint32_t index;        /* 文件下标 */
    volatile uint32_t reserved[9];  /* 保留     */
    char name[NLB_SERVICE_NAME_LEN];/* 业务名   */
};

/* 服务器信息数据结构 */
struct server_info
{
    /*******  寻址信息  *******/
    uint32_t server_ip;            /* IP地址     */
    uint32_t weight_base;          /* 起始权重   */
    uint16_t weight_static;        /* 静态权重   */
    uint16_t weight_dynamic;       /* 动态权重   */
    uint16_t port_type;            /* 端口类型   */
    uint16_t port_num;             /* 端口个数   */
    uint16_t port[NLB_PORT_MAX];   /* 端口列表   */

    /*******  统计信息  *******/
    uint32_t failed;               /* 失败数     */
    uint32_t success;              /* 成功数     */
    uint64_t cost;                 /* 时延总和   */
    uint64_t dead_time;            /* 死机时间   */

    uint64_t reserved[2];          /* 保留 */
};

/* 服务器信息数据 */
struct shm_servers
{
    uint64_t cost_total;           /* 时延总数     */
    uint64_t success_total;        /* 成功总数     */
    uint64_t fail_total;           /* 失败总数     */
    uint32_t server_num;           /* 服务器数     */
    uint32_t dead_num;             /* 死机数       */
    uint32_t dead_retry_times;     /* 死机重试数   */
    uint32_t weight_total;         /* 权重总量     */
    uint32_t weight_dead_base;     /* 死机机器权重 */
    uint32_t mhash_order;          /* 多阶hash阶数 */
    uint32_t mhash_mods[MAX_ROW_COUNT+1];      /* 多阶hash每一阶的模数 */
    uint32_t mhash_idx[NLB_SERVER_HASH_LEN];   /* 多阶hash数据 */
    int32_t  policy;                           /* 寻址算法策略 */
////
    int32_t  version;               // version标识
    uint32_t weight_static_total;   // 静态权重总和
    uint32_t weight_low_num;        // 低权重机器数
    int32_t  shaping_request_min;   // 统计周期最小请求数,默认10

    float    success_ratio_base;    // 成功率基准，默认98%
    float    success_ratio_min;     // 最小成功率，小于该值，不要选择对应的机器，随机选择其它机器
    float    resume_weight_ratio;   // 死机恢复后设置的权重比例，默认10%
    float    dead_retry_ratio;      // 死机机器探测包比例
    float    weight_low_watermark;  // 机器权重低于该值，会被标记为低权重机器
    float    weight_low_ratio;      // 低权重机器总数低于该值，不降低权重，只给大于平均成功率的机器加权重
    float    weight_incr_ratio;     // 每次增加权重的比例

    uint32_t reserved[88];                     /* 保留字段     */
///
    struct server_info svrs[0];                /* 所有服务器信息 */
};

#pragma pack(pop)

#endif





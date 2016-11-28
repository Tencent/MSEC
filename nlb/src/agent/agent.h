
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


#ifndef _AGENT_H_
#define _AGENT_H_

#include <stdint.h>
#include "list.h"
#include "commtype.h"
#include "commstruct.h"

/* agent本地路由数据 */
struct agent_local_rdata
{
    struct list_head hash_node;         /* hash链表节点 */
    struct list_head list_node;         /* 链表节点     */
    struct list_head event_list;        /* 事件列表     */

    char name[NLB_SERVICE_NAME_LEN];    /* 业务名       */
    uint64_t update_time;               /* 更新时间戳   */
    bool     watcher_flag;              /* 是否设置监视 */
    struct shm_meta * route_meta;       /* 元数据信息   */
    struct shm_servers * servs_data[2]; /* 服务器信息   */
};

/**
 * @brief 获取agent路由数据链表
 */
struct list_head *get_rdata_list(void);

/**
 * @brief 获取本地管理路由数据信息
 */
struct agent_local_rdata *get_local_rdata(const char *name);

/**
 * @brief 通过IP获取服务器信息
 */
struct server_info *get_server_info(uint32_t ip, struct shm_servers *servers);

/**
 * @brief 清除所有业务的watch标记
 */
void clean_services_watching(void);

/**
 * @brief 在没有新事件(重新下发配置和死机事件)的情况下，更新配置
 * @param new_shm_servers --> 新加载的服务器信息
 * @return <0 更新失败 =0 更新成功
 */
int32_t update_config(struct agent_local_rdata *rdata, struct shm_servers *new_shm_servers, uint64_t mtime);

/**
 * @brief  添加一个新的业务到本地
 * @return =0 成功 <0 失败
 */
int32_t add_new_service(const char *name, struct shm_servers *shm_srvs, uint64_t mtime);

/**
 * @brief Agent统一初始化函数
 */
int32_t init(void);

/**
 * @brief Agent主处理函数
 */
void run(void);

#endif


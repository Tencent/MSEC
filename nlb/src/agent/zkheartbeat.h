
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


#ifndef _ZKHEARTBEAT_H_
#define _ZKHEARTBEAT_H_

#include <stdint.h>
#include "agent.h"

/* 检查是否创建心跳节点 */
bool check_heartbeat_created(void);

/* 初始化节点监视数据 */
void heartbeat_data_init(void);

/**
 * @brief 创建服务器临时节点，用于检测节点死活
 * @info  服务提供方agent创建，[server | mix]
 */
int32_t create_heartbeat_node(uint32_t ip);

/**
 * @brief 设置单个业务所有节点的watch信息
 */
void set_service_nodes_wather(struct agent_local_rdata *rdata);

/**
 * @brief 设置所有节点没有watch
 */
void clean_nodes_watching(void);

#endif


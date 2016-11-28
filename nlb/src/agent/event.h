
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


#ifndef _TASK_H_
#define _TASK_H_

#include <stdint.h>
#include "list.h"
#include "commtype.h"
#include "commstruct.h"

enum {
    NLB_EVENT_TYPE_LOAD_SERVICE = 1,
    NLB_EVENT_TYPE_NODE_DEAD    = 2,
    NLB_EVENT_TYPE_NODE_RESUME  = 3,
};

/* 事件数据结构 */
struct event {
    struct list_head list_node;
    int32_t type;
    void *ctx;
    char name[NLB_SERVICE_NAME_LEN];
};

/**
 * @brief 添加业务事件
 * @info  暂时只有重新load配置的事件
 */
void add_service_event(const char *name);

/**
 * @brief 添加节点事件
 */
void add_node_event(uint32_t ip, int32_t type);

/**
 * @brief 删除一个事件
 */
void delete_event(struct event *event, bool free_ctx);

#endif


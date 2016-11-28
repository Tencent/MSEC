
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


#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "commtype.h"
#include "list.h"
#include "event.h"
#include "log.h"
#include "agent.h"

/**
 * @brief 创建任务
 */
struct event *create_event(int32_t type, const char *name, void *ctx)
{
    struct event *event = malloc(sizeof(struct event));
    if (NULL == event) {
        NLOG_ERROR("No memory");
        return NULL;
    }

    event->type = type;
    event->ctx  = ctx;
    strncpy(event->name, name, NLB_SERVICE_NAME_LEN);
    return event;
}

/**
 * @brief 直接添加事件到链表
 */
void add_event(struct list_head *event_list, int32_t type, const char *name, void *ctx, bool tail)
{
    struct event *event;

    /* 创建一个事件 */
    event = create_event(type, name, ctx);
    if (NULL == event) {
        NLOG_ERROR("create_event failed");
        return;
    }

    if (tail) {
        list_add_tail(&event->list_node, event_list);
    } else {
        list_add(&event->list_node, event_list);
    }

    return;
}

/**
 * @brief 合并新任务
 */
void merge_new_event(struct list_head *event_list, int32_t type, const char *name, void *ctx)
{
    //struct event_node_ctx *node_ctx;
    struct event *event_first;
    struct event *event;

    /* 如果当前没有事件，直接添加事件 */
    if (list_empty(event_list)) {
        add_event(event_list, type, name, ctx, false);
        return;
    }

    /* 重新load配置的事件，判断是否有重复，重新load的事件总是放在链表头 */
    event_first = list_first_entry(event_list, struct event, list_node);
    if (type == NLB_EVENT_TYPE_LOAD_SERVICE) {
        if (event_first->type == type) {
            return;
        }

        add_event(event_list, type, name, ctx, false);
        return;
    }

    /* 节点死机和节点恢复的事件，需要查找是否有重复，如果重复，直接覆盖事件类型 */
    list_for_each_entry(event, event_list, list_node)
    {
        /* 比较IP地址是否相同 */
        if (event->ctx == ctx) {
            event->type = type;
            return;
        }
    }

    add_event(event_list, type, name, ctx, true);
    return;
}

/**
 * @brief 添加业务事件
 * @info  暂时只有重新load配置的事件
 */
void add_service_event(const char *name)
{
    struct agent_local_rdata *rdata;
    struct list_head *event_list;

    if (NULL == name) {
        return;
    }

    rdata = get_local_rdata(name);
    if (NULL == rdata) {
        return;
    }

    event_list = &rdata->event_list;

    merge_new_event(event_list, NLB_EVENT_TYPE_LOAD_SERVICE, name, NULL);
}

/**
 * @brief 添加节点事件
 */
void add_node_event(uint32_t ip, int32_t type)
{
    struct agent_local_rdata *rdata;    
    struct list_head *rdata_list = get_rdata_list();

    if ((type != NLB_EVENT_TYPE_NODE_DEAD)
        && (type != NLB_EVENT_TYPE_NODE_RESUME))
    {
        NLOG_ERROR("invalid node event type (%d)", type);
        return;
    }

    /* 循环所有的业务，如果存在该服务器，就需要添加事件 */
    list_for_each_entry(rdata, rdata_list, list_node)
    {
        if (get_server_info(ip, rdata->servs_data[rdata->route_meta->index])) {
            merge_new_event(&rdata->event_list, type, rdata->name, (void *)(long)ip);
        }
    }
}

/**
 * @brief 删除一个事件
 */
void delete_event(struct event *event, bool free_ctx)
{
    list_del(&event->list_node);
    if (free_ctx && (event->ctx != NULL)) {
        free(event->ctx);
    }

    free(event);
}


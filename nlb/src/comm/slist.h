
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
 * @filename slist.h
 * @info     线程安全单项链表，支持添加，遍历节点，不支持删除节点
 */

#ifndef _SLIST_H__
#define _SLIST_H__

#include "atomic.h"

#if __GNUC__ >= 4
#define offsetof(type, member)  __builtin_offsetof (type, member)
#else
#define offsetof(type, member) (unsigned long)(&((type *)0)->member)
#endif

#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-offsetof(type, member)))

/**
 * @brief  无锁多线程单向链表
 * @info   限制: 不支持删除节点
 */
struct slist_head
{
    struct slist_head * volatile  next;   // 下一个节点的指针
};

/**
 * @brief 添加一个节点到链表头
 */
void slist_add(struct slist_head *head, struct slist_head *node)
{
    struct slist_head *next;

    do {
        next = head->next;
        node->next = next;
        if (compare_and_swap_l((long *)&head->next, (long)next, (long)node)) {
            break;
        }
    } while (1);
}

/**
 * @brief  单向遍历整个链表
 * @pos    当前遍历节点指针
 * @curr   链表结构临时指针
 * @head   链表头结点指针
 * @member 链表节点在业务数据结构中的名字
 */
#define slist_for_each_entry(pos, curr, head, member) \
        for (curr = (head)->next;   \
             ((curr != NULL) && (pos = list_entry(curr, typeof(*pos), member))); \
             curr = curr->next)



#endif



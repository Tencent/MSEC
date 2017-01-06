
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


#ifndef __MEMQUEUE_H__
#define __MEMQUEUE_H__

#include <stdint.h>

typedef struct _tag_mem_queue_header
{
    volatile int32_t    version;            /* 版本号       */
    volatile int32_t    free_list;          /* 空闲链表     */
    volatile int32_t    block_num;          /* 块数         */
    volatile int32_t    unused;             /* 可用块数     */
    volatile int32_t    block_size;         /* 块大小       */
    volatile int32_t    block_data_size;    /* 块数据大小   */
    volatile int32_t    write_times;        /* 写次数       */
    volatile int32_t    read_times;         /* 读次数       */
    volatile int32_t    msg_cnt;            /* 当前消息数   */
    volatile int32_t    nomem;              /* 没有块次数   */
}mem_queue_header_t;

typedef struct _tag_mem_block_queue
{
    volatile int32_t    read_pos;           /* 队列读指针 */
    volatile int32_t    write_pos;          /* 队列写指针 */
    volatile int32_t    size;               /* 队列大小   */
    volatile int32_t    reserved[13];
    volatile int32_t    queue[0];           /* 队列数组   */
}mem_block_queue_t;

typedef struct _tag_mem_block
{
    volatile int32_t    next;               /* 链表指针 */
    volatile int32_t    index;              /* 块下标   */
    volatile int32_t    total_len;          /* 块链表数据总大小 */
    volatile int32_t    data_len;           /* 当前块数据大小   */
    volatile int32_t    reserved[4];
    char                data[0];            /* 数据起始未知     */
}mem_block_t;

typedef struct _tag_mem_queue_desc
{
    mem_queue_header_t *header;
    mem_block_queue_t * queue;
    mem_block_t *       blocks;
}mem_queue_desc_t;

/**
 * @brief   无锁队列初始化
 * @return  0       成功
 *          其它    失败
 * @param   desc        无锁队列描述符
 *          vaddr       无锁队列内存地址
 *          mem_size    无锁队列大小
 *          data_size   使用者数据大小，通过该值，确认块大小
 * @info    可以同时多进程初始化
 */
int32_t mem_queue_init(mem_queue_desc_t *desc, void *vaddr, int32_t mem_size, int32_t data_size);

/**
 * @brief   无锁队列入队列接口函数
 * @return  >=0     队列包个数
 *          <0      失败
 * @param   desc        无锁队列描述符
 *          data        数据
 *          len         数据长度
 */
int32_t mem_queue_push(mem_queue_desc_t *desc, char *data, int32_t len);

/**
 * @brief   无锁队列入队列接口函数
 * @return  >=0     队列包个数
 *          <0      失败
 * @param   desc        无锁队列描述符
 *          iov         iovec地址
 *          iovlen      iov个数
 */
int32_t mem_queue_pushv(mem_queue_desc_t *desc, struct iovec *iov, int32_t iovlen);

/**
 * @brief   无锁队列出队列接口函数
 * @return  0       没有数据
 *          >0      出队列包长度
 *          <0      失败
 * @param   desc        无锁队列描述符
 *          data        数据
 * @info    data需要调用者free
 */
int32_t mem_queue_pop(mem_queue_desc_t *desc, char **data);

/**
 * @brief   无锁队列出队列接口函数
 * @return  0       没有数据
 *          >0      出队列包长度
 *          <0      失败
 * @param   desc        无锁队列描述符
 *          buff        数据
 *          len         buff长度
 */
int32_t mem_queue_pop_nm(mem_queue_desc_t *desc, char *buff, int32_t len);

#endif


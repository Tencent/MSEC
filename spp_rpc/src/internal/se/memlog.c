
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
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "vma.h"
#include "memlog.h"

/* buffer数据结构定义 */
typedef struct _tag_ml_buffer
{
    void *  next;   // 链表指针
    void *  mem;    // 内存指针
    int     len;    // 内存长度
}ml_buffer_t;

/* buffer链表头数据结构 */
typedef struct _tag_buffer_list
{
    ml_buffer_t *   head;   // 头指针
    ml_buffer_t *   tail;   // 尾指针
    int             size;   // 链表大小
    int             maxsize;// 链表最大长度
}ml_buffer_list_t;


/**
 * @brief 释放一个内存日志buffer的内存
 */
void ml_buffer_free(ml_buffer_t *buffer)
{
    if (NULL == buffer)
        return;

    if (buffer->mem)
        free(buffer->mem);

    free(buffer);
}

/**
 * @brief 分配一个内存日志buffer的内存，并初始化
 */
ml_buffer_t *ml_buffer_alloc(const void *mem, int len)
{
    ml_buffer_t *buffer;

    buffer = (ml_buffer_t *)malloc(sizeof(ml_buffer_t));
    if (NULL == buffer) {
        return NULL;
    }

    buffer->mem = malloc(len);
    if (NULL == buffer->mem) {
        free(buffer);
        return NULL;
    }

    memcpy(buffer->mem, mem, len);
    buffer->next    = NULL;
    buffer->len     = len;

    return buffer;
}

/**
 * @brief 检查memlog的bugffer是否有效
 */
int ml_buffer_check_valid(ml_buffer_t *buffer, vma_list_t *vma_list)
{
    int flags;

    if (NULL == buffer) {
        return 0;
    }

    if (NULL == vma_list) {
        return 1;
    }

    flags = VMA_PROT_READ | VMA_PROT_WRITE; 
    if (vma_check_prot(vma_list, buffer, sizeof(*buffer), flags)
        && vma_check_prot(vma_list, buffer->mem, buffer->len, flags)) {
        return 1;
    }

    return 0;
}


/* 内存日志的链表本地变量 */
static __thread ml_buffer_list_t mem_log_list = {
    .head       = NULL,
    .tail       = NULL,
    .size       = 0,
    .maxsize    = 1,
};

/**
 * @brief 删除buffer链表头
 */
void ml_buffer_list_del_head(ml_buffer_list_t *list)
{
    ml_buffer_t *head = list->head;

    if (NULL == head) {
        return;
    }

    list->head  = head->next;
    list->size--;

    if (NULL == list->head)
        list->tail = NULL;

    ml_buffer_free(head);
}

/**
 * @brief 添加一个buffer到链表中
 */
void ml_buffer_list_add(ml_buffer_list_t *list, ml_buffer_t *buffer)
{
    if (NULL == list || NULL == buffer) {
        return;
    }

    buffer->next = NULL;
    if (NULL == list->head) {
        list->head  = buffer;
        list->tail  = buffer;
        list->size  = 1;
        return;
    }

    list->tail->next = buffer;
    list->tail       = buffer;
    list->size++;

    if (list->size > list->maxsize) {
        ml_buffer_list_del_head(list);
    }

    return;
}

/**
 * @brief 输出buf到文件描述符
 */
int write_all(int fd, void *mem, int len)
{
    char *buf = (char *)mem;
    int ret;
    int wlen = 0;

    while (wlen != len) {
        ret = write(fd, buf + wlen, len - wlen);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -2;
        }

        wlen += ret;
    }

    return 0;
}

/**
 * @brief 从文件描述符读取数据到buf
 */
int read_fd(int fd, void *mem, int len)
{
    char *buf = (char *)mem;
    int ret;
    int rlen = 0;

    while (rlen != len) {
        ret = read(fd, buf + rlen, len - rlen);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -2;
        }

        if (ret == 0) {
            break;
        }

        rlen += ret;
    }

    return rlen;
}



/**
 * @brief 添加memlog
 */
void add_memlog(const void *mem, int len)
{
    ml_buffer_t *buffer;

    assert(mem != NULL);
    assert(len > 0);

    buffer = ml_buffer_alloc(mem, len);
    if (NULL == buffer) {
        return;
    }

    ml_buffer_list_add(&mem_log_list, buffer);
}

/**
 * @brief 设置memlog保存的最大条数
 */
void set_memlog_maxsize(int size)
{
    mem_log_list.maxsize = size;
}

/**
 * @brief 输出memlog到指定文件描述符
 * @info  为了在信号处理函数中安全调用，不能访问非法地址
 */
int show_memlog_fd(int fd, vma_list_t *vma_list)
{
    ml_buffer_list_t *list;
    ml_buffer_t *     buffer ;

    if (fd < 0) {
        return -1;
    }

    list    = &mem_log_list;
    buffer  = list->head;

    while (buffer) {
        if (!ml_buffer_check_valid(buffer, vma_list)) {
            return -2;
        }

        write_all(fd, &buffer->len, sizeof(buffer->len));
        write_all(fd, buffer->mem, buffer->len);

        buffer = buffer->next;
    }

    return 0;
}


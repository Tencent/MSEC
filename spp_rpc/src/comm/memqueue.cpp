
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
 * @brief 一写多读或者多写一读内存队列
 * @info  [注意] 非多读多写队列
 */

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <pthread.h>
#include "memqueue.h"

#define MEM_QUEUE_VERSION       (0x20160920)
#define MEM_QUEUE_HEADER_SIZE   (4096)
#define MEM_QUEUE_VADDR_MIN     (0x100000)
#define MEM_ALIGN_SIZE          (64)

#ifndef max
#define max(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })
#endif

#ifndef min
#define min(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })
#endif

static __inline__ void atomic_inc(volatile int32_t *p)
{
    __asm__ __volatile__(
      "lock; incl %0"
    : "=m"(*p)
    : "m" (*p));
}

static __inline__ void atomic_dec(volatile int32_t *p)
{
    __asm__ __volatile__(
      "lock; decl %0"
    : "=m"(*p)
    : "m"(*p));
}

#define atomic_read(v)		(*v)

/* 块大小做64字节对其 */
int32_t calc_block_size(int32_t data_size)
{
    int32_t real_size;

    real_size = data_size + sizeof(mem_block_t);
    real_size = (real_size + MEM_ALIGN_SIZE - 1) / MEM_ALIGN_SIZE * MEM_ALIGN_SIZE;
    return real_size;
}

/* 计算块个数 */
int32_t calc_block_num(int32_t mem_size, int32_t block_size)
{
    int32_t block_num;
    int32_t block_mem_size;

    block_mem_size  = mem_size - MEM_QUEUE_HEADER_SIZE - MEM_ALIGN_SIZE - sizeof(mem_block_queue_t) - sizeof(int32_t);
    block_num       = block_mem_size / (block_size + sizeof(int32_t));

    return block_num;
}

/* 计算队列大小，队列大小肯定大于块个数 */
int32_t calc_queue_size(int32_t block_num)
{
    int32_t queue_size;
    int32_t queue_mem_size;
    int32_t queue_mem_max;

    queue_mem_max   = sizeof(mem_block_queue_t) + (block_num + 1)*sizeof(int32_t) + MEM_ALIGN_SIZE;
    queue_mem_size  = (queue_mem_max + MEM_ALIGN_SIZE - 1)/MEM_ALIGN_SIZE * MEM_ALIGN_SIZE;
    queue_size      = (queue_mem_size - sizeof(mem_block_queue_t))/sizeof(int32_t);
    return queue_size;
}

int32_t mem_queue_init(mem_queue_desc_t *desc, void *vaddr, int32_t mem_size, int32_t data_size)
{
    mem_queue_header_t *header;
    mem_block_queue_t * queue;
    mem_block_t *       blocks;
    mem_block_t *       block = NULL;
    int32_t             fd;
    int32_t block_size, block_num, queue_size, loop;

    if (NULL == desc || NULL == vaddr
        || mem_size < MEM_QUEUE_VADDR_MIN
        || data_size <= 0 || data_size > mem_size) {
        return -1;
    }

    header      = (mem_queue_header_t *)vaddr;
    queue       = (mem_block_queue_t *)((char *)vaddr + MEM_QUEUE_HEADER_SIZE);
    block_size  = calc_block_size(data_size);
    block_num   = calc_block_num(mem_size, block_size);
    queue_size  = calc_queue_size(block_num);
    blocks      = (mem_block_t *)((char *)queue + sizeof(mem_block_queue_t) + queue_size * sizeof(int32_t));

    fd = open("./.mem_queue.lock", O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        return -2;
    }

    flock(fd, LOCK_EX);
    if (header->version == MEM_QUEUE_VERSION) {
        flock(fd, LOCK_UN);
        goto EXIT;
    }

    header->free_list       = 0;
    header->block_num       = block_num;
    header->unused          = block_num;
    header->block_size      = block_size;
    header->block_data_size = block_size - sizeof(mem_block_t);
    header->write_times     = 0;
    header->read_times      = 0;

    queue->read_pos         = 0;
    queue->write_pos        = 0;
    queue->size             = queue_size;
    for (loop = 0; loop < queue_size; loop++) {
        queue->queue[loop] = -1;
    }

    for (loop = 0; loop < block_num; loop++) {
        block       = (mem_block_t *)((char *)blocks + block_size * loop);
        block->index= loop;
        block->next = loop + 1;
    }

    block->next = -1;

    header->version = MEM_QUEUE_VERSION;
    flock(fd, LOCK_UN);

EXIT:

    desc->header    = header;
    desc->queue     = queue;
    desc->blocks    = blocks;
    
    if (fd >= 0) {
        close(fd);
    }

    return 0;
}

mem_block_t *mem_queue_block_list_add(mem_block_t *head, mem_block_t *block)
{
    if (NULL == block) {
        return head;
    }

    if (NULL == head) {
        block->next = -1;
        return block;
    }

    block->total_len    = 0;
    block->data_len     = 0;
    block->next         = head->index;

    return block;
}

static inline mem_block_t *mem_queue_i2b(mem_queue_desc_t *desc, int32_t index)
{
    return (mem_block_t *)((char *)desc->blocks + index * desc->header->block_size);
}

static inline int32_t mem_queue_b2i(mem_queue_desc_t *desc, mem_block_t *block)
{
    return (int32_t)(((unsigned long)((char *)block - (char *)desc->blocks))/desc->header->block_size);
}

int32_t mem_queue_free_block(mem_queue_desc_t *desc, mem_block_t *block)
{
    mem_queue_header_t *header;
    int32_t             free_index;

    if (NULL == desc || NULL == block) {
        return -1;
    }

    header  = desc->header;
    do {
        block->data_len = 0;
        free_index  = atomic_read(&header->free_list);
        block->next = free_index;
        if (__sync_bool_compare_and_swap(&header->free_list, free_index, block->index)) {
            atomic_inc(&header->unused);
            return 0;
        }
    } while (1);

    return 0;
}

int32_t mem_queue_free_blocks(mem_queue_desc_t *desc, mem_block_t *blocks)
{
    mem_block_t *block;
    int32_t index, next;

    if (NULL == desc) {
        return -1;
    }

    if (NULL == blocks) {
        return 0;
    }

    index = blocks->index;
    do {
        block = mem_queue_i2b(desc, index);
        next  = block->next;
        mem_queue_free_block(desc, block);
        index = next;
    } while (next != -1);

    return 0;
}

mem_block_t *mem_queue_alloc_block(mem_queue_desc_t *desc)
{
    mem_queue_header_t *header;
    mem_block_t *       block;
    int32_t             free_index;

    if (NULL == desc) {
        return NULL;
    }

    header  = desc->header;

    while (atomic_read(&header->unused)) {
        free_index  = atomic_read(&header->free_list);
        if (free_index == -1) {
            atomic_inc(&header->nomem);
            return NULL;
        }

        block = mem_queue_i2b(desc, free_index);
        if (__sync_bool_compare_and_swap(&header->free_list, free_index, block->next)) {
            atomic_dec(&header->unused);
            return block;
        }
    }

    return NULL;
}

mem_block_t *mem_queue_alloc_blocks(mem_queue_desc_t *desc, int32_t num)
{
    mem_queue_header_t *header;
    mem_block_t *       blocks;
    mem_block_t *       block;
    int32_t             loop;

    if (NULL == desc || num <= 0) {
        return NULL;
    }

    header  = desc->header;
    if (atomic_read(&header->unused) < num) {
        return NULL;
    }

    blocks = NULL;
    for (loop = 0; loop < num; loop++) {
        block = mem_queue_alloc_block(desc);
        if (NULL == block) {
            mem_queue_free_blocks(desc, blocks);
            return NULL;
        }

        blocks = mem_queue_block_list_add(blocks, block);
    }

    return blocks;
}

int32_t mem_block_write_data(mem_queue_desc_t *desc, mem_block_t *blocks, char *data, int32_t len)
{
    mem_queue_header_t *header;
    mem_block_t *       block;
    int32_t index, copy_len;

    if (NULL == desc || NULL == blocks || NULL == data || len <= 0) {
        return -1;
    }

    blocks->total_len   = len;
    header  = desc->header;
    index   = blocks->index;
    do {
        block    = mem_queue_i2b(desc, index);
        copy_len = min(header->block_data_size, len);
        memcpy(block->data, data, copy_len);
        data    += copy_len;
        len     -= copy_len;
        index    = block->next;
        block->data_len = copy_len;
    } while (index != -1 && len > 0);

    if (len || index != -1) {
        return -2;
    }

    return 0;
}

int32_t mem_block_writev_data(mem_queue_desc_t *desc, mem_block_t *blocks, struct iovec *iov, int32_t iovlen)
{
    mem_queue_header_t *header;
    mem_block_t *       block;
    mem_block_t *       block_head;
    int32_t index, copy_len, block_left, total;
    int32_t iov_index, iov_write_pos, iov_left;

    if (NULL == desc || NULL == blocks || NULL == iov || iovlen <= 0) {
        return -1;
    }

    header  = desc->header;
    index   = blocks->index;

    iov_index       = 0;
    iov_write_pos   = 0;
    total           = 0;
    block_head      = blocks;
    for (index = blocks->index; index != -1; ) {
        block = mem_queue_i2b(desc, index);
        while (iov_index < iovlen) {
            iov_left    = iov[iov_index].iov_len - iov_write_pos;
            block_left  = header->block_data_size - block->data_len;
            copy_len    = min(iov_left, block_left);

            memcpy(block->data + block->data_len, (char *)iov[iov_index].iov_base + iov_write_pos, copy_len);

            block->data_len += copy_len;
            iov_write_pos   += copy_len;
            total           += copy_len;

            if (block->data_len == header->block_data_size) {
                if (iov_write_pos == (int32_t)iov[iov_index].iov_len) {
                    iov_index++;
                    iov_write_pos = 0;
                }

                break;
            } else if (iov_write_pos == (int32_t)iov[iov_index].iov_len) {
                iov_index++;
                iov_write_pos = 0;
                continue;
            }

            /* no others */
        }
        index = block->next;
    }

    block_head->total_len   = total;

    return 0;
}

int32_t mem_block_read_data(mem_queue_desc_t *desc, mem_block_t *blocks, char *data, int32_t len)
{
    mem_block_t *       block;
    int32_t index, copy_len;

    if (NULL == desc || NULL == blocks || NULL == data || len <= 0) {
        return -1;
    }

    index   = blocks->index;
    do {
        block    = mem_queue_i2b(desc, index);
        copy_len = min(block->data_len, len);
        memcpy(data, block->data, copy_len);
        data    += copy_len;
        len     -= copy_len;
        index    = block->next;
    } while (index != -1 && len > 0);

    if (index != -1) {
        return -2;
    }

    return 0;
}


int32_t mem_block_queue_push(mem_block_queue_t *queue, int32_t index)
{
    int32_t next_pos, write_pos, read_pos;

    do {
        write_pos   = atomic_read(&queue->write_pos);
        read_pos    = atomic_read(&queue->read_pos);
        next_pos    = (write_pos + 1) % queue->size;

        if (((write_pos - read_pos) == (queue->size - 1))
            || (read_pos - write_pos) == 1) {
            return 0;
        }

        if (__sync_bool_compare_and_swap(&queue->queue[write_pos], -1, index)) {
            __sync_bool_compare_and_swap(&queue->write_pos, write_pos, next_pos);
            return 1;
        }

        __sync_bool_compare_and_swap(&queue->write_pos, write_pos, next_pos);

    } while (1);

    return 0;
}

int32_t mem_block_queue_pop(mem_block_queue_t *queue, int32_t *index)
{
    int32_t next_pos, read_pos;
    int32_t block_index;

    do {
        read_pos    = atomic_read(&queue->read_pos);
        next_pos    = (read_pos + 1) % queue->size;

        if (read_pos == atomic_read(&queue->write_pos)) {
            break;
        }

        block_index = queue->queue[read_pos];
        if (block_index == -1) {
            __sync_bool_compare_and_swap(&queue->read_pos, read_pos, next_pos);
            continue;
        }

        if (__sync_bool_compare_and_swap(&queue->queue[read_pos], block_index, -1)) {
            __sync_bool_compare_and_swap(&queue->read_pos, read_pos, next_pos);
            *index = block_index;
            return 1;
        }

        __sync_bool_compare_and_swap(&queue->read_pos, read_pos, next_pos);
    } while (1);

    return 0;
}

int32_t mem_queue_push(mem_queue_desc_t *desc, char *data, int32_t len)
{
    mem_queue_header_t *header;
    mem_block_t *       blocks;
    int32_t need_block_num, block_data_size, ret;

    if (NULL == desc || NULL == data || len <= 0) {
        return -1;
    }

    header          = desc->header;
    block_data_size = header->block_data_size;
    need_block_num  = (len + block_data_size - 1) / block_data_size;

    blocks = mem_queue_alloc_blocks(desc, need_block_num);
    if (NULL == blocks) {
        return -2;
    }

    ret = mem_block_write_data(desc, blocks, data, len);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        return -3;
    }

    atomic_inc(&header->write_times);
    atomic_inc(&header->msg_cnt);

    ret = mem_block_queue_push(desc->queue, blocks->index);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        return -4;
    }

    return atomic_read(&header->msg_cnt);
}

int32_t mem_queue_pushv(mem_queue_desc_t *desc, struct iovec *iov, int32_t iovlen)
{
    mem_queue_header_t *header;
    mem_block_t *       blocks;
    int32_t need_block_num, block_data_size, ret;
    int32_t len, loop;

    if (NULL == desc || NULL == iov || iovlen <= 0) {
        return -1;
    }

    len = 0;
    for (loop = 0; loop < iovlen; loop++) {
        len += (int32_t)iov[loop].iov_len;
    }

    header          = desc->header;
    block_data_size = header->block_data_size;
    need_block_num  = (len + block_data_size - 1) / block_data_size;

    blocks = mem_queue_alloc_blocks(desc, need_block_num);
    if (NULL == blocks) {
        return -2;
    }

    ret = mem_block_writev_data(desc, blocks, iov, iovlen);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        return -3;
    }

    atomic_inc(&header->write_times);
    atomic_inc(&header->msg_cnt);

    ret = mem_block_queue_push(desc->queue, blocks->index);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        return -4;
    }

    return atomic_read(&header->msg_cnt);
}


int32_t mem_queue_pop(mem_queue_desc_t *desc, char **data)
{
    mem_queue_header_t *header;
    mem_block_queue_t * queue;
    mem_block_t *       blocks;
    char *              buff = NULL;
    int32_t index, len, ret;

    if (NULL == desc || NULL == data) {
        return -1;
    }

    header  = desc->header;
    queue   = desc->queue;

    if (!mem_block_queue_pop(queue, &index)) {
        return 0;
    }

    blocks  = mem_queue_i2b(desc, index);
    len     = blocks->total_len;
    buff    = (char *)malloc(len);
    if (NULL == buff) {
        mem_queue_free_blocks(desc, blocks);
        return -2;
    }

    ret = mem_block_read_data(desc, blocks, buff, len);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        free(buff);
        return -3;
    }

    atomic_inc(&header->read_times);
    atomic_dec(&header->msg_cnt);

    mem_queue_free_blocks(desc, blocks);
    *data = buff;

    return len;
}

int32_t mem_queue_pop_nm(mem_queue_desc_t *desc, char *data, int32_t len)
{
    mem_queue_header_t *header;
    mem_block_queue_t * queue;
    mem_block_t *       blocks;
    int32_t index, ret, total_len;

    if (NULL == desc || NULL == data || len <= 0) {
        return -1;
    }

    header  = desc->header;
    queue   = desc->queue;

    if (!mem_block_queue_pop(queue, &index)) {
        return 0;
    }

    blocks      = mem_queue_i2b(desc, index);
    total_len   = blocks->total_len;
    if (len < total_len) {
        mem_queue_free_blocks(desc, blocks);
        return -2;
    }

    ret = mem_block_read_data(desc, blocks, data, len);
    if (ret < 0) {
        mem_queue_free_blocks(desc, blocks);
        return -3;
    }

    atomic_inc(&header->read_times);
    atomic_dec(&header->msg_cnt);

    mem_queue_free_blocks(desc, blocks);

    return total_len;
}

#if 0
static __thread mem_queue_desc_t g_desc;
#define MEM_SIZE 16*1024*1024
static volatile int32_t g_push_cnt;
static volatile int32_t g_pop_cnt;
void *vaddr;


void *push_entry(void *args)
{
    int32_t ret;
    int32_t data[30] = {0};

    mem_queue_init(&g_desc, vaddr, MEM_SIZE, 40);

    while (1) {
        data[0] = random();
        data[1] = random();
        data[29] = data[0] ^ data[1];

        ret = mem_queue_push(&g_desc, (char *)data, sizeof(data));
        if (ret < 0) {
            //printf("push ret:%d\n", ret);
            continue;
        }

        atomic_inc(&g_push_cnt);
    }

    return NULL;
}

void *pop_entry(void *args)
{
    int32_t ret;
    int32_t *data;

    mem_queue_init(&g_desc, vaddr, MEM_SIZE, 40);

    while (1) {
        ret = mem_queue_pop(&g_desc, (char **)&data);
        if (ret < 0) {
            printf("pop ret: %d\n", ret);
            continue;
        }

        if (ret == 0)
            continue;

        atomic_inc(&g_pop_cnt);

        if (ret != 100) {
            printf("pop ret: %d\n", ret);
            free(data);
            continue;
        }

        if ((data[0] ^ data[1]) != data[29]) {
            free(data);
            printf("pop data wrong!\n");
            continue;
        }

        free(data);
    }
}

int main(void)
{
    pthread_t id;

    vaddr = malloc(MEM_SIZE);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, pop_entry, NULL);

    sleep(10000);

    return 0;
}
#endif

/*********************** test **************************/

#if 0
mem_queue_desc_t g_desc;

static mem_block_t *volatile g_alloc_queue[8096];
static int32_t g_rindex;
static int32_t g_windex;
static int32_t g_wcnt;
static int32_t g_rcnt;


void *alloc_entry(void *args)
{
    mem_block_t *block;

    while (1) {
        block = mem_queue_alloc_blocks(&g_desc, 1);
        if (NULL == block) {
            continue;
        }

        g_wcnt++;
        g_alloc_queue[g_windex] = block;
        g_windex = (g_windex + 1) % (g_desc.queue->size);
    }

    return NULL;
}

void *free_entry(void * args)
{
    int32_t ret;
    mem_block_t *block;

    while (1) {
        if (g_windex == g_rindex) {
            continue;
        }

        block = g_alloc_queue[g_rindex];
        g_rindex = (g_rindex + 1) % (g_desc.queue->size);
        ret = mem_queue_free_blocks(&g_desc, block);
        if (ret < 0) {
            printf("free block failed: %d index: %d\n", ret, block->index);
            continue;
        }
        g_rcnt++;
    }

    return NULL;
}

int main()
{
    int32_t memsize = 16*1024*1024;
    void *vaddr = malloc(memsize);
    int32_t ret;
    pthread_t id;
    
    ret = mem_queue_init(&g_desc, vaddr, memsize, 4096);
    if (ret < 0) {
        printf("Init failed, %d\n", ret);
        return -1;
    }

    pthread_create(&id, NULL, alloc_entry, NULL);
    pthread_create(&id, NULL, free_entry, NULL);

    sleep(10000000);
    return 0;
}

volatile int32_t g_push_cnt;
volatile int32_t g_pop_cnt;
mem_block_queue_t *g_queue;

void *push_entry(void *args)
{
    int32_t index;
    while (1) {
        index = random();
        if (index == -1) {
            index = 0x123452;
        }
        if (mem_block_queue_push(g_queue, index)) {
            atomic_inc(&g_push_cnt);
        }
    }
}

void *pop_entry(void *args)
{
    int32_t index;
    while (1) {
        if (mem_block_queue_pop(g_queue, &index)) {
            atomic_inc(&g_pop_cnt);
        }
    }
}


int32_t main(void)
{
    int32_t i = 0;    
    pthread_t id;

    g_queue = malloc(sizeof(mem_block_queue_t) + 100000*sizeof(int32_t));

    for (i = 0; i < 100000; i++) {
        g_queue->queue[i] = -1;
    }

    g_queue->size = 100000;
    g_queue->read_pos = 0;
    g_queue->write_pos = 0;

    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, push_entry, NULL);
    pthread_create(&id, NULL, pop_entry, NULL);

    sleep(10000000);
    return 0;
}

#endif




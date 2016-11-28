
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


#ifndef __VMA_H__
#define __VMA_H__

#include "mem.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * 虚拟内存标志位宏定义
 */
#define VMA_PROT_READ       0x00000001
#define VMA_PROT_WRITE      0x00000002
#define VMA_PROT_EXEC       0x00000004
#define VMA_PROT_SHARED     0x00000008
#define VMA_PROT_PRIVATE    0x00000010

/**
 * 虚拟内存描述数据结构
 */
typedef struct _tag_vma
{
    void *  next;
    void *  start;
    void *  end;
    void *  file;
    void *  dev;
    int     flags;
    unsigned long ino;
    unsigned long offset;
}vma_t;

/**
 * 虚拟内存链表数据结构
 */
typedef struct _tag_vma_list
{
    vma_t * head;
    vma_t * tail;
    int     size;
}vma_list_t;

/**
 * @brief 检查[addr, addr+len)是否有相应的权限
 * @return  =1 有相应权限
 *          =0 没有权限
 */
int vma_check_prot(vma_list_t *list, void *addr, int len, int prot);

/**
 * @brief 返回地址所在的二进制文件
 */
const char *vma_get_bin_path(vma_list_t *list, void *addr);

/**
 * @brief 获取当前进程的虚拟地址空间信息
 * @info  为保证信号处理函数中能够安全调用，需要传入自定义的heap
 */
vma_list_t *vma_list_create(s_heap_t *heap);

/**
 * @brief 将虚拟地址信息输出到fd 同("/proc/self/maps")
 */
int show_vma_list_fd(int fd, vma_list_t *list);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif

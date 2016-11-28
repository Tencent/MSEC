
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



#ifndef __MEM_H__
#define __MEM_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * @brief 自定义heap管理数据结构
 * @info  由于在信号处理函数中不能使用malloc等函数，直接自建一个heap
 *        分配的内存不需要释放，信号处理函数执行完后统一释放
 */
typedef struct _tag_s_heap
{
    void *  addr;
    int     used;
    int     size;
}s_heap_t;

/**
 * @brief 初始化自定义heap
 */
s_heap_t *s_heap_init(int size);

/**
 * @brief 从自定义heap中分配空间
 */
void *s_heap_alloc(s_heap_t *heap, int size);

/**
 * @brief 释放自定义heap
 */
void s_heap_destory(s_heap_t *heap);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif


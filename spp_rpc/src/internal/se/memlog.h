
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


#ifndef __MEMLOG_H__
#define __MEMLOG_H__

#include "vma.h"

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * @brief 输出buf到文件描述符
 */
int write_all(int fd, void *mem, int len);

/**
 * @brief 从文件描述符读取数据到buf
 */
int read_fd(int fd, void *mem, int len);

/**
 * @brief 输出memlog到指定文件描述符
 * @info  为了在信号处理函数中安全调用，不能访问非法地址
 */
int show_memlog_fd(int fd, vma_list_t *vma_list);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif


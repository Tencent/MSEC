
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



#ifndef __EXCEPTION_H__
#define __EXCEPTION_H__

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

/**
 * @brief 添加memlog
 */
void add_memlog(const void *mem, int len);

/**
 * @brief 设置memlog保存的最大条数
 */
void set_memlog_maxsize(int size);

/**
 * @brief 信号处理回调函数
 */
typedef void (*signal_cb)(int, void *);

/**
 * @brief 重置信号处理函数
 */
void reset_signal_handler(void);

/**
 * @brief 安装信号处理函数
 */
int install_signal_handler(const char *prefix, signal_cb cb, void *args);

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif


#endif




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


#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#include <stdint.h>

#define NLB_POLLIN   (1<<0)
#define NLB_POLLOUT  (1<<1)

/* 获取agent监听套接字 */
int32_t get_listen_fd(void);

/**
 * @brief  网络初始化
 * @return =0 成功 <0 失败
 */
int32_t network_init(void);

/**
 * @brief  监听网络事件
 * @info   监听zookeeper和路由请求
 */
int32_t network_poll(void);

/**
 * @brief 网络事件处理主函数
 */
int32_t network_process(void);

#endif


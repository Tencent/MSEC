
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


#ifndef _ZKPLUGIN_H_
#define _ZKPLUGIN_H_

#include <stdint.h>
#include "commtype.h"
#include "zookeeper.h"

/* zookeeper事件类型转换成字符串 */
const char* zk_type_2_str(int32_t type);

/* zookeeper客户端状态转换成字符串 */
const char* zk_stat_2_str(int32_t state);

/* 获取zookeeper客户端句柄 */
zhandle_t *get_zk_instance(void);

/* 检查zookeeper是否已经连接上 */
bool zk_connected(void);

/* 检查是否需要重新初始化zookeeper */
bool zk_need_reinit(void);

/**
 * @brief  zookeeper创建节点函数
 * @return =0 成功 <0 失败
 */
int32_t zk_simple_create(const char *path);

/**
 * @brief zookeeper客户端句柄初始化
 */
int32_t nlb_zk_init(const char *host, int32_t timeout);

/**
 * @brief 获取zookeeper的poll事件
 */
int32_t nlb_zk_poll_events(int32_t *fd, int32_t *nlb_events, uint64_t *timeout);

/**
 * @brief zookeeper主处理函数
 */
void nlb_zk_process(uint32_t nlb_events);

/**
 * @brief 关闭zookeeper句柄
 */
void nlb_zk_close(void);

/**
 * @brief 重新初始化zookeeper句柄
 */
int32_t nlb_zk_reinit(void);

#endif


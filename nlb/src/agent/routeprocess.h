
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


#ifndef _ROUTEPROCESS_H_
#define _ROUTEPROCESS_H_

#include <stdint.h>

/**
 * @brief 删除指定业务的路由任务
 * @info  在获取业务路由配置失败，或者配置为空的情况下调用
 */
void delete_route_task(const char *name);

/**
 * @brief 处理路由请求
 * @info  路由请求都是从API发送过来
 *        每个业务暂时最多只支持100个路由请求，如果超过，直接回复错误
 */
void process_route_request(int32_t listen_fd);

/**
 * @brief  处理某个业务的路由任务
 * @info   在新获取到路由配置时调用
 */
void process_route_task(const char *name);

/* 初始化路由任务 */
void init_route_task(void);

#endif


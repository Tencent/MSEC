
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


#ifndef _ZKSERVICE_H_
#define _ZKSERVICE_H_

#include <stdint.h>
#include "agent.h"

/**
 * @brief  获取业务配置信息
 * @info   只有新业务请求和NLB_EVENT_TYPE_LOAD_SERVICE事件会调用该函数
 * @return =0 成功 <0 失败
 */
int32_t load_service_config(const char *name);

/**
 * @brief 设置业务监视事件
 * @info  必须保证在本地有该业务
 */
void set_service_watcher(struct agent_local_rdata *rdata);

#endif


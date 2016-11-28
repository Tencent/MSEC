
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


#ifndef _POLICY_H_
#define _POLICY_H_

#include <stdint.h>
#include "commtype.h"


/**
 * 加权轮询:    通过静态权重选择一个服务器
 * 动态加权:    带回包统计的加权轮询，通过动态权重寻址
 * 标准算法:    包含服务器保活检测的动态加权
 * 异构策略:    暂时同动态加权
 * 一致性哈希:  一致性hash寻址 
 */
enum {
    NLB_POLICY_DEFAULT      = 0,           /* 兼容老版本 */
    NLB_POLICY_WRR          = 1,           /* 加权轮询 */
    NLB_POLICY_DYNAMIC_WRR  = 2,           /* 动态加权 */
    NLB_POLICY_STANDARD     = 3,           /* 标准算法 */
    NLB_POLICY_ODD          = 4,           /* 异构策略 */
    NLB_POLICY_CONSIST_HASH = 5,           /* 一致性hash */
    NLB_POLICY_UNKOWN       = 100,
};

/* 字符串转换成策略类型 */
int32_t str2policy(const char *policy);

/* 策略类型转换字符串 */
const char *policy2str(int32_t policy);

/* 检查是否要做心跳上报 */
bool check_need_heartbeat(int32_t policy);


#endif


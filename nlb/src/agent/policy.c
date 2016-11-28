
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



#include <stdint.h>
#include <string.h>
#include "commtype.h"
#include "policy.h"

/* 字符串转换成策略类型 */
int32_t str2policy(const char *policy)
{
    if (!strcmp(policy, "wrr"))
        return NLB_POLICY_WRR;
    else if (!strcmp(policy, "dynamic wrr"))
        return NLB_POLICY_DYNAMIC_WRR;
    else if (!strcmp(policy, "standard"))
        return NLB_POLICY_STANDARD;
    else if (!strcmp(policy, "odd"))
        return NLB_POLICY_ODD;
    else if (!strcmp(policy, "consistent hash"))
        return NLB_POLICY_CONSIST_HASH;
    return NLB_POLICY_UNKOWN;
}


/* 策略类型转换字符串 */
const char *policy2str(int32_t policy)
{
    switch (policy)
    {
        case NLB_POLICY_WRR:
            return "wrr";
        case NLB_POLICY_DYNAMIC_WRR:
            return "dynamic wrr";
        case NLB_POLICY_STANDARD:
        case NLB_POLICY_DEFAULT:
            return "standard";
        case NLB_POLICY_ODD:
            return "odd";
        case NLB_POLICY_CONSIST_HASH:
            return "consistent hash";
        default:
            return "unkown";
    }

    return "unkown";
}

/* 检查是否要做心跳上报 */
bool check_need_heartbeat(int32_t policy)
{
    switch (policy)
    {
        case NLB_POLICY_WRR:
        case NLB_POLICY_DYNAMIC_WRR:
        case NLB_POLICY_ODD:
        case NLB_POLICY_CONSIST_HASH:
            return false;
        case NLB_POLICY_DEFAULT:
        case NLB_POLICY_STANDARD:
            return true;
        default:
            return false;
    }

    return false;
}




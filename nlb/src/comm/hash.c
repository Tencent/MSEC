
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


/**
 * @filename hash.c
 */
#include <stdint.h>
#include "commtype.h"
#include "hash.h"

#define CHANGE_RATE    1.3

/**
 * @brief 计算hash键值
 */
uint32_t gen_hash_key(const char *str)
{
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * @brief 判断是否质数
 */ 
bool prime(uint32_t n)
{
    int32_t i;

    if (n == 0 || n == 1) {
        return false;
    }

    if (n == 2) {
        return true;
    }

    if (n % 2 == 0) {
        return false;
    }

    for (i = 3; i * i <= n; i += 2) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 计算多阶hash每一阶的模数
 */
int32_t calc_hash_mods(uint32_t node_cnt, uint32_t *order, uint32_t *mods)
{
    uint32_t n      = (uint32_t)(node_cnt * (CHANGE_RATE - 1) / CHANGE_RATE); // 第一行的建议节点数
    uint32_t count  = 0;    // 记录已分配的节点数
    uint16_t i      = 0;

    while (1) {
        /* 从建议值开始找一个素数 */
        while (!prime(n)) {
            ++ n;
        }

        mods[i] = n;
        ++ i;
        count += n;
        n = (uint32_t)(n / CHANGE_RATE); // 下一个n的建议值

        if (count > node_cnt) {
            /* 一般也不可能超出，这里还是做个修正 */
            mods[i - 1] -= count - node_cnt;
            break;
        }

        if (i >= MAX_ROW_COUNT) {
            /* 满阶了，修正一下退出，最后一行的大小一般不影响效果 */
            mods[i - 1] += node_cnt - count;
            break;
        }
    }

    *order = i;

    return 0;
}



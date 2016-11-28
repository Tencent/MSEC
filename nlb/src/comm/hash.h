
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
 * @filename hash.h
 */
#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>

#define MAX_ROW_COUNT  15   /* 多阶hash最大阶数 */

uint32_t gen_hash_key(const char *str);

/**
 * @brief 计算多阶hash每一阶的模数
 */
int32_t calc_hash_mods(uint32_t node_cnt, uint32_t *order, uint32_t *mods);

#endif


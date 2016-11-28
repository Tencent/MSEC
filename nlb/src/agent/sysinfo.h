
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


#ifndef _SYSINFO_H_
#define _SYSINFO_H_

#include <stdint.h>

/**
 * @brief 初始化系统信息
 */
void init_sysinfo(void);

/**
 * @brief 获取系统信息
 * @info  CPU占用百分比，MEM总量，MEM空闲
 */
void get_sysinfo(uint32_t *load_percent, uint64_t *mem_total, uint64_t *mem_free);


#endif


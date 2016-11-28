
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


#ifndef _NLBTIME_H__
#define _NLBTIME_H__

#include <sys/time.h>
#include <time.h>

/**
 * @brief  获取当前时间毫秒数
 */
static inline uint64_t get_time_ms(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (tv.tv_sec*1000 + tv.tv_usec/1000);
}

/**
 * @brief  获取当前时间秒数
 */
static inline uint64_t get_time_s(void)
{
    return (uint64_t)time(NULL);
}

/**
 * @brief 转换timeval为毫秒
 */
static inline uint64_t covert_tv_2_ms(struct timeval *tv)
{
    return (tv->tv_sec*1000 + tv->tv_usec/1000);
}

/**
 * @brief 转换毫秒为timeval
 */
static inline void covert_ms_2_tv(uint64_t tms, struct timeval *tv)
{
    tv->tv_sec  = tms/1000;
    tv->tv_usec = tms%1000*1000;
}

#endif



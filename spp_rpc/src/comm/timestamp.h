
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


#ifndef __TTC_TIMESTAMP_H__
#define __TTC_TIMESTAMP_H__

#include <stdint.h>
#include <sys/time.h>

#define __must_inline__ __attribute__((always_inline))

// 由使用者负责去update
extern struct timeval __spp_g_now_tv; // session_mgr.cpp

inline void __must_inline__
__spp_do_update_tv(void)
{
    gettimeofday(&__spp_g_now_tv, NULL);
}

inline int64_t __must_inline__
__spp_get_now_ms(void)
{
    return (int64_t)__spp_g_now_tv.tv_sec * 1000 + __spp_g_now_tv.tv_usec / 1000;
}

inline time_t __must_inline__
__spp_get_now_s(void)
{
    return __spp_g_now_tv.tv_sec;
}

#endif

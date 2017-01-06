
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
 * @filename atomic.h
 */

#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#include <stdint.h>

static inline uint32_t fetch_and_add(uint32_t *ptr, uint32_t value)
{
    if (value) {
        return __sync_fetch_and_add(ptr, value);
    }

    return *ptr;
}

static inline uint64_t fetch_and_add_8(uint64_t *ptr, uint64_t value)
{
    if (value) {
        return __sync_fetch_and_add(ptr, value);
    }

    return *ptr;
}

static inline uint32_t return_and_set(uint32_t *ptr, uint32_t value)
{
    return __sync_lock_test_and_set(ptr, value);
}

static inline uint64_t return_and_set_8(uint64_t *ptr, uint64_t value)
{
    return __sync_lock_test_and_set(ptr, value);
}

static inline int32_t compare_and_swap(uint32_t *ptr, uint32_t o, uint32_t n)
{
    return __sync_bool_compare_and_swap(ptr, o, n);
}

static inline int32_t compare_and_swap_8(uint64_t *ptr, uint64_t o, uint64_t n)
{
    return __sync_bool_compare_and_swap(ptr, o, n);
}

static inline int32_t compare_and_swap_l(long *ptr, long o, long n)
{
    return __sync_bool_compare_and_swap(ptr, o, n);
}

#define mb() __asm__ __volatile__("mfence": : :"memory")

#endif



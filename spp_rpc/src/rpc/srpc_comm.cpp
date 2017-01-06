
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
#include <stdlib.h>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <math.h>
#include <sys/syscall.h>


#include "srpc_comm.h"

#define __GNU_SOURCE

namespace srpc {

/**
 * @brief 通过错误码，获取错误信息描述
 */
const char *errmsg(int32_t err)
{
    switch (err)
    {
        case SRPC_SUCCESS:
            return "success";
        case SRPC_ERR_PARA_ERROR:
            return "invalid parameter";
        case SRPC_ERR_SYSTEM_ERROR:
            return "system error";
        case SRPC_ERR_INVALID_PKG:
            return "invalid package";
        case SRPC_ERR_INVALID_PKG_HEAD:
            return "invalid package header";
        case SRPC_ERR_INVALID_PKG_BODY:
            return "invalid package body";
        case SRPC_ERR_INVALID_METHOD_NAME:
            return "invalid method name";
        case SRPC_ERR_HEADER_UNINIT:
            return "package header uninitialized";
        case SRPC_ERR_BODY_UNINIT:
            return "package body uninitialized";
        case SRPC_ERR_NO_MEMORY:
            return "no memory";
        case SRPC_ERR_TIMEOUT:
            return "timeout";
        case SRPC_ERR_NETWORK:
            return "network error";
        case SRPC_ERR_RECV_TIMEOUT:
            return "recevice timeout";
        case SRPC_ERR_SEND_RECV_FAILED:
            return "network sendrecv failed";
        case SRPC_ERR_INVALID_ENDPOINT:
            return "invalid endpoint";
        case SRPC_ERR_GET_ROUTE_FAILED:
            return "get route failed";
        case SRPC_ERR_INVALID_SEQUENCE:
            return "invalid sequence";
        case SRPC_ERR_NO_BODY:
            return "no package body";
        case SRPC_ERR_SERVICE_IMPL_FAILED:
            return "service implement failed";
        case SRPC_ERR_BACKEND:
            return "backend process failed, use GetErrText get more information";
        case SRPC_ERR_PHP_FAILED:
            return "php error";
        case SRPC_ERR_PYTHON_FAILED:
            return "python error";
        default:
            return "unkown error";
    }

    return "unkown error";
}

/**
 * @brief sequence生成函数
 */
uint64_t newseq(void)
{
    return my_rand();
}

/**
 * @brief 染色日志ID生成函数，通过方法名计算生成
 */
uint64_t gen_colorid(const char *method_name) 
{
    unsigned int hash = 5381;
    int c;
    const char* cstr = (const char*)method_name;
    while ((c = *cstr++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

/**
 * @brief 时延到字符串转换函数
 */
const char *Cost2TZ(int32_t cost)
{
    if (cost < 1)
        return "0~1ms";
    if (cost < 2)
        return "1~2ms";
    if (cost < 5)
        return "2~5ms";
    if (cost < 10)
        return "5~10ms";
    if (cost < 20)
        return "10~20ms";
    if (cost < 50)
        return "20~50ms";
    if (cost < 100)
        return "50~100ms";
    if (cost < 200)
        return "100~200ms";
    if (cost < 500)
        return "200~500ms";
    if (cost < 1000)
        return "500~1s";

    return "1s+";
}


// 标准库的随机算法有局限，这里用Wichman-Hill的算法
static uint32_t __thread init_flag;
static unsigned long long __thread x; //随机种子
static unsigned long long __thread y; //随机种子
static unsigned long long __thread z; //随机种子

static const unsigned long long N_RAND_MAX = 1ULL << 32;

// 初始化随机种子
static void my_srand(unsigned long long a)
{
    x = a % 30268;
    a /= 30268;
    y = a % 30306;
    a /= 30306;
    z = a % 30322;
    a /= 30322;
    ++x;
    ++y;
    ++z;
}

// 得到一个0和1之间的浮点随机数
static double my_random()
{
    if (!init_flag) {
        init_flag = 1;
        my_srand(time(NULL) ^ syscall(SYS_gettid));
    }

    x = x * 171 % 30269;
    y = y * 172 % 30307;
    z = z * 170 % 30323;
    return fmod(x / 30269.0 + y / 30307.0 + z / 30323.0, 1.0);
}

// 得到一个随机的unsigned int
unsigned int my_rand()
{
    return (unsigned int)(my_random() * N_RAND_MAX);
}


}


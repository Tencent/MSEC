
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

#include "srpc_comm.h"

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
    static uint64_t seq;
    if (!seq) {
        uint32_t rd;
        srandom(time(NULL) ^ getpid());
        rd = (uint32_t)(random() & 0x7fffffff);
        seq = (uint64_t)(rd ? rd : rd+1);
    }
    return ++seq;
}

/**
 * @brief 染色日志ID生成函数，通过方法名计算生成
 */
uint64_t gen_colorid(const char *method_name)
{
    uint64_t id = 0;
    while (*method_name++)
        id += *method_name * 107;
    return id;
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


}


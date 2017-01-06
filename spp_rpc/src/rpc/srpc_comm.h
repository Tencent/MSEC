
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


#ifndef __SRPC_COMM_H__
#define __SRPC_COMM_H__

#include <stdint.h>
#include <string>

namespace srpc {

// 本地返回码定义
enum SRPC_RETCODE
{
    SRPC_SUCCESS                    = 0,
    SRPC_ERR_PARA_ERROR             = -1,
    SRPC_ERR_SYSTEM_ERROR           = -2,
    SRPC_ERR_INVALID_PKG            = -3,
    SRPC_ERR_INVALID_PKG_HEAD       = -4,
    SRPC_ERR_INVALID_PKG_BODY       = -5,
    SRPC_ERR_INVALID_METHOD_NAME    = -6,
    SRPC_ERR_HEADER_UNINIT          = -7,
    SRPC_ERR_BODY_UNINIT            = -8,
    SRPC_ERR_NO_MEMORY              = -9,
    SRPC_ERR_TIMEOUT                = -10,
    SRPC_ERR_NETWORK                = -11,
    SRPC_ERR_RECV_TIMEOUT           = -12,
    SRPC_ERR_SEND_RECV_FAILED       = -13,
    SRPC_ERR_INVALID_ENDPOINT       = -14,
    SRPC_ERR_GET_ROUTE_FAILED       = -15,
    SRPC_ERR_INVALID_SEQUENCE       = -16,
    SRPC_ERR_NO_BODY                = -17,
    SRPC_ERR_SERVICE_IMPL_FAILED    = -18,
    SRPC_ERR_BACKEND                = -19,
    SRPC_ERR_PHP_FAILED             = -20,
    SRPC_ERR_PYTHON_FAILED          = -21,
};

/**
 * @brief 通过错误码，获取错误信息描述
 */
const char *errmsg(int32_t err);

/**
 * @brief sequence生成函数
 */
uint64_t newseq(void);

/**
 * @brief 染色日志ID生成函数，通过方法名计算生成
 */
uint64_t gen_colorid(const char *method_name);

/**
 * @brief 时延到字符串转换函数
 */
const char *Cost2TZ(int32_t cost);

/**
 * @brief 得到一个随机的unsigned int
 */
unsigned int my_rand();

}

#endif


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


#ifndef _NLBAPI_H_
#define _NLBAPI_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NLB_PORT_TYPE_UDP  = 1,
    NLB_PORT_TYPE_TCP  = 2,
    NLB_PORT_TYPE_ALL  = 3,
}NLB_PORT_TYPE;

/* 单条路由信息 */
struct routeid
{
    uint32_t ip;        // IPV4地址 : 网络字节序
    uint16_t port;      // 端口     : 本机字节序
    NLB_PORT_TYPE type; // 端口类型
};

/* API错误码 */
enum {
    NLB_ERR_INVALID_PARA    = -1,  // 参数无效
    NLB_ERR_NO_ROUTE        = -2,  // 查找路由失败
    NLB_ERR_NO_AGENT        = -3,  // 没有安装AGENT
    NLB_ERR_NO_ROUTEDATA    = -4,  // 没有该业务的路由信息
    NLB_ERR_NO_STATISTICS   = -5,  // 没有该业务的路由统计信息
    NLB_ERR_NO_HISTORY      = -6,  // 没有历史路由信息
    NLB_ERR_LOCK_CONFLICT   = -7,  // 锁冲突
    NLB_ERR_INVALID_MAGIC   = -8,  // ROUTEID magic无效
    NLB_ERR_NO_SERVER       = -9,  // 没有这个服务器
    NLB_ERR_CREATE_SOCKET_FAIL = -10, // 创建和AGENT通信的socket失败
    NLB_ERR_SEND_FAIL          = -11, // 发送路由请求失败
    NLB_ERR_RECV_FAIL          = -12, // 接收路由请求失败
    NLB_ERR_INVALID_RSP        = -13, // 路由请求回复报文无效
    NLB_ERR_AGENT_ERR          = -14, // Agent回复路由请求失败
};

/**
 * @brief 通过业务名获取路由信息
 * @para  name:  输入参数，业务名字符串  "Login.ptlogin"
 * @      route: 输出参数，路由信息(ip地址，端口，端口类型)
 * @return  0: 成功  others: 失败
 */
int32_t getroutebyname(const char *name, struct routeid *route);

/**
 * @brief 更新路由统计数据
 * @info  每次收发结束后，需要将成功与否、时延数据更新到统计数据
 * @para  name:  输入参数，业务名字符串  "Login.ptlogin"
 *        ip:    输入参数，IPV4地址
 *        failed:输入参数，>1:失败次数 0->成功
 *        cost:  输入参数，时延
 */
int32_t updateroute(const char *name, uint32_t ip, int32_t failed, int32_t cost);

#ifdef __cplusplus
}
#endif


#endif


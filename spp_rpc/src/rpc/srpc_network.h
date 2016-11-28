
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


#ifndef __SRPC_NETWORKING_H__
#define __SRPC_NETWORKING_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include "SyncFrame.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_intf.h"
#include "nlbapi.h"

using namespace std;
using namespace google::protobuf;

namespace srpc {

/**
 * @brief 网络适配基类
 */
class CNetAdpt
{
public:
    CNetAdpt() {}
    virtual ~CNetAdpt() {}

    virtual int32_t SendRecv() = 0;

};

/**
 * @brief SRPC网络适配类
 */
class CRpcNetAdpt: public CNetAdpt
{
public:
    CRpcNetAdpt() {}
    CRpcNetAdpt(const string &service_name, const string &method_name, const CRpcHead &head,
                Message *request, Message *response, int32_t timeout = 800, int32_t proto = PORT_TYPE_ALL);
    virtual ~CRpcNetAdpt() {}

    /* 初始化请求报文消息头部 */
    void InitReqHead();

    /* 收发包 */
    virtual int32_t SendRecv(void);

private:
    uint64_t    m_seq;          // 本次网络收发的sequence
    string      m_service_name; // 业务名
    string      m_method_name;  // 方法名
    CRpcHead    m_req_head;     // 请求RPC头
    CRpcHead    m_rep_head;     // 回复RPC头
    Message *   m_response;     // 回复
    Message *   m_request;      // 请求
    struct timeval m_begin;     // 请求开始时间
    struct timeval m_end;       // 请求结束时间
    struct sockaddr_in m_addr;  // 后端服务器地址
    NLB_PORT_TYPE m_type;       // 协议类型，NLB_PORT_TYPE
    int32_t       m_timeout;    // 本次请求的超时时间
    int32_t       m_proto;      // 业务设置的协议
};

}

#endif


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
 * @filename srpc_channel.cpp
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "mt_api.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_ctrl.h"
#include "srpc_channel.h"

using namespace NS_MICRO_THREAD;
using namespace std;
using namespace srpc;
using namespace google::protobuf;

// 构造与析构函数
CRpcChannel::CRpcChannel(const string& endpoint)
{
    m_endpoint  = endpoint;
    m_timeout   = 1000;
    m_rsp_buf   = NULL;
    m_req_seq   = newseq();
}

CRpcChannel::~CRpcChannel()
{
    if (m_rsp_buf != NULL) {
        free(m_rsp_buf);
        m_rsp_buf = NULL;
    }
}

// 解析endpoint地址信息
int32_t CRpcChannel::ParseEndpoint(struct sockaddr_in* address)
{
    if (NULL == address)
    {
        return -1;
    }
    memset(address, 0, sizeof(*address));
    
    size_t colon_pos = m_endpoint.rfind(':');
    if (colon_pos == string::npos)
    {
        return -2;
    }
    address->sin_family      = AF_INET;
    address->sin_addr.s_addr = inet_addr(m_endpoint.substr(0, colon_pos).c_str());
    address->sin_port        = htons((uint16_t)atoi(m_endpoint.substr(colon_pos+1).c_str()));
    return 0;
}


// 继承自父类的接口函数
void CRpcChannel::CallMethod(const MethodDescriptor* method,
                             RpcController* controller,
                             const Message* request,
                             Message* response,
                             Closure* done)
{
    char*   req_buf  = NULL;
    int32_t msg_len;
    int32_t ret;
    
    CRpcCtrl* ctrl= dynamic_cast<CRpcCtrl*>(controller);

    // 1. 构建消息请求头信息 可选按method fullname寻址
    CRpcHead stub_head;
    uint64_t color_id = gen_colorid(method->full_name().c_str());
    stub_head.set_method_name(method->full_name());
    stub_head.set_sequence(m_req_seq);
    stub_head.set_coloring(0);
    stub_head.set_color_id(color_id);
    stub_head.set_flow_id(m_req_seq);

    // 2. 封装请求包信息
    ret = SrpcPackPkg(&req_buf, &msg_len, &stub_head, request);
    if (ret != SRPC_SUCCESS)
    {
        ctrl->SetFailed(ret);
        goto EXIT_LABEL;
    }

    // 3. 发送报文接收应答
    ret = this->SendAndRecv(req_buf, msg_len, &m_rsp_buf, &m_rsp_len, (int32_t)m_timeout);
    if (ret < 0)
    {
        ctrl->SetFailed(ret);
        goto EXIT_LABEL;
    }

    // 4. 解析应答消息
    ret = SrpcUnpackPkg(m_rsp_buf, m_rsp_len, &m_rsp_head, response);
    if (ret < 0)
    {
        ctrl->SetFailed(ret);
        goto EXIT_LABEL;
    }

    if (m_rsp_head.sequence() != m_req_seq)
    {
        ctrl->SetFailed(SRPC_ERR_INVALID_SEQUENCE);
        goto EXIT_LABEL;
    }

    if (m_rsp_head.err_msg().size() != 0)
    {
        ctrl->SetFailed(m_rsp_head.err_msg());
        goto EXIT_LABEL;
    }

    if (m_rsp_head.err() != 0)
    {
        ctrl->SetFailed(m_rsp_head.err());
        goto EXIT_LABEL;
    }

    // 5. 返回处理结果
    ctrl->SetSuccess();

EXIT_LABEL:

    if (req_buf)
    {
        free(req_buf);
    }

    if (m_rsp_buf)
    {
        free(m_rsp_buf);
        m_rsp_buf = NULL;
    }

    if (done != NULL)
    {
        done->Run();
    }

    return;
}


// 微线程异步收发接口定义
int32_t CRpcUdpChannel::SendAndRecv(char* req_buf, int32_t req_len, char **rsp_buf, int32_t *rsp_len, int32_t timeout)
{
    int32_t ret = 0;
    char err_msg[256];
    
    if ((NULL == req_buf) || (req_len <= 0) || (NULL == rsp_buf) || (NULL == rsp_len))
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 1. 解析地址信息 ip:port
    struct sockaddr_in service_addr;
    ret = this->ParseEndpoint(&service_addr);
    if (ret != 0)
    {
        snprintf(err_msg, sizeof(err_msg), "%s invalid", m_endpoint.c_str());
        m_err_msg = err_msg;
        return SRPC_ERR_INVALID_ENDPOINT;
    }

    // 2. 分配内存接收UDP包，UDP最大64k
    int32_t buf_size = 64*1024;
    char *buf = (char *)malloc(buf_size);
    if (NULL == buf)
    {
        snprintf(err_msg, sizeof(err_msg), "no memory");
        m_err_msg = err_msg;
        return SRPC_ERR_NO_MEMORY;
    }

    // 3. 与后端网络交互
    ret = mt_udpsendrcv(&service_addr, req_buf, req_len, buf, buf_size, timeout);
    if (ret < 0)
    {
        free(buf);
        snprintf(err_msg, sizeof(err_msg), "sendrecv(%s:%d) ret: %d error: %s",
                 inet_ntoa(service_addr.sin_addr), ntohs(service_addr.sin_port), ret, strerror(errno)); 
        m_err_msg = err_msg;
        return SRPC_ERR_SEND_RECV_FAILED;
    }

    *rsp_buf = buf;
    *rsp_len = buf_size;
    
    return SRPC_SUCCESS;
}

// 微线程异步收发接口定义
int32_t CRpcTcpChannel::SendAndRecv(char* req_buf, int32_t req_len, char **rsp_buf, int32_t *rsp_len, int32_t timeout)
{
    int32_t ret = 0;
    char err_msg[256];
    
    if ((NULL == req_buf) || (req_len <= 0) || (NULL == rsp_buf) || (NULL == rsp_len))
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 1. 解析地址信息 ip:port
    struct sockaddr_in service_addr;
    ret = this->ParseEndpoint(&service_addr);
    if (ret != 0)
    {
        snprintf(err_msg, sizeof(err_msg), "%s invalid", m_endpoint.c_str());
        m_err_msg = err_msg;
        return SRPC_ERR_INVALID_ENDPOINT;
    }
    
    // 2. 创建socket 发送请求
    int32_t buf_size;
    ret = mt_tcpsendrcv_v2(&service_addr, req_buf, req_len, (void **)rsp_buf, buf_size, timeout, SrpcCheckPkgLen);
    if (ret < 0)
    {
        snprintf(err_msg, sizeof(err_msg), "sendrecv(%s:%d) ret: %d error: %s",
                 inet_ntoa(service_addr.sin_addr), ntohs(service_addr.sin_port), ret, strerror(errno)); 
        m_err_msg = err_msg;
        return SRPC_ERR_SEND_RECV_FAILED;
    }

    // 3. 成功返回
    *rsp_len = buf_size;
    
    return SRPC_SUCCESS;
}




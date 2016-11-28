
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
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <string>
#include "srpc.pb.h"
#include "nlbapi.h"
#include "srpc_intf.h"
#include "mt_api.h"
#include "srpc_comm.h"
#include "srpc_comm.h"
#include "srpc_proto.h"

using namespace std;
using namespace srpc;
using namespace NS_MICRO_THREAD;

namespace srpc
{

/**
 * @brief 构造与析构函数
 */
CProxyBase::CProxyBase(const string &endpoint)
{
    m_type      = ENDPOINT_TYPE_UNKOWN;
    m_endpoint  = endpoint;
    m_port_type = PORT_TYPE_ALL;
    ParseEndpoint(endpoint);

    return;
}

CProxyBase::~CProxyBase()
{
}

/**
 * @brief 设置后端地址
 */
void CProxyBase::SetEndPoint(const string &endpoint)
{
    m_type      = ENDPOINT_TYPE_UNKOWN;
    m_endpoint  = endpoint;
    ParseEndpoint(endpoint);

    return;
}

/**
 * @brief 获取后端地址
 */
const string &CProxyBase::GetEndPoint(void)
{
    return m_endpoint;
}

/**
 * @brief 设置调用方业务名(本地业务名)
 */
void CProxyBase::SetCaller(const string &caller)
{
    m_caller = caller;
}

/**
 * @brief 获取本地业务名
 */
const string &CProxyBase::GetCaller(void)
{
    return m_caller;
}

/**
 * @brief 设置协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
 * @info  如果不设置，采用NLB获取的协议方式，优先使用TCP
 */
void CProxyBase::SetProtoType(int proto)
{
    m_port_type = proto;
}

/**
 * @brief 获取协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
 */
int CProxyBase::GetProtoType(void)
{
    return m_port_type;
}

/**
 * @brief 通过业务名获取地址信息
 * @info  外部业务需要将路由信息导入NLB
 */
int32_t CProxyBase::GetRoute(struct sockaddr_in &addr, int32_t &type)
{
    int32_t ret;
    struct routeid id;

    if (m_type == ENDPOINT_TYPE_UNKOWN)
    {
        return -1;
    }

    if (m_type == ENDPOINT_TYPE_ADDR)
    {
        memcpy(&addr, &m_address, sizeof(addr));
        type = m_port_type;
        return 0;
    }

    // 通过nlb获取路由
    ret = getroutebyname(m_service_name.c_str(), &id);
    if (ret < 0)
    {
        return -2;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = id.ip;
    addr.sin_port           = htons(id.port);

    if (id.type == NLB_PORT_TYPE_UDP)
    {
        type = PORT_TYPE_UDP;
    }
    else if (id.type == NLB_PORT_TYPE_TCP)
    {
        type = PORT_TYPE_TCP;
    }
    else
    {
        if (m_port_type == PORT_TYPE_TCP)
        {
            type = PORT_TYPE_TCP;
        }
        else if (m_port_type == PORT_TYPE_UDP)
        {
            type = PORT_TYPE_UDP;
        }
        else
        {
            type = PORT_TYPE_TCP;
        }
    }

    return 0;
}

/**
 * @brief 更新路由信息(回包统计)
 */
void CProxyBase::UpdateRoute(struct sockaddr_in &addr, int32_t failed, int32_t cost)
{
    if (m_type == ENDPOINT_TYPE_NLB)
    {
        updateroute(m_service_name.c_str(), addr.sin_addr.s_addr, failed, cost);
    }
}

// 解析endpoint地址信息
void CProxyBase::ParseEndpoint(const string &endpoint)
{
    // "Login.ptlogin"
    size_t ait_pos = endpoint.rfind('@');
    if (ait_pos == string::npos)
    {
        size_t point_pos = endpoint.rfind(".");
        if (point_pos == string::npos)
        {
            return;
        }

        m_type         = ENDPOINT_TYPE_NLB;
        m_service_name = endpoint;
    }

    // "127.0.0.1:8888@udp"
    size_t colon_pos = endpoint.rfind(':');
    if (colon_pos == string::npos)
    {
        return;
    }

    if (endpoint.substr(ait_pos+1) == "udp")
    {
        m_port_type = PORT_TYPE_UDP;
    }
    else if (endpoint.substr(ait_pos+1) == "tcp")
    {
        m_port_type = PORT_TYPE_TCP;
    }
    else
    {
        m_port_type = PORT_TYPE_TCP;
    }

    m_address.sin_family      = AF_INET;
    m_address.sin_addr.s_addr = inet_addr(endpoint.substr(0, colon_pos).c_str());
    m_address.sin_port        = htons((uint16_t)atoi(endpoint.substr(colon_pos+1, ait_pos).c_str()));
    m_type                    = ENDPOINT_TYPE_ADDR;
}

/**
 * @brief 构造与析构函数
 */
CSrpcProxy::CSrpcProxy(const string &endpoint):CProxyBase(endpoint)
{
    m_check_cb = SrpcCheckPkgLen;
    m_coloring = false;
    m_sequence = 0;
}

CSrpcProxy::~CSrpcProxy()
{
}

/**
 * @brief 设置调用方法名
 */
void CSrpcProxy::SetMethod(const string &method)
{
    m_method_name = method;
}

/**
 * @brief 设置日志是否染色
 */
void CSrpcProxy::SetColoring(bool coloring)
{
    m_coloring = coloring;
}

/**
 * @brief 获取调用结果
 */
void CSrpcProxy::GetResult(int32_t &fret, int32_t &sret)
{
    fret = (int32_t)m_rsp_head.err();
    sret = (int32_t)m_rsp_head.result();
}

/**
 * @brief 获取是否失败
 */
bool CSrpcProxy::Failed(void)
{
    return (m_rsp_head.err() != 0);
}

/**
 * @brief 获取错误字符串描述
 */
void CSrpcProxy::GetErrText(string &err_text)
{
    char err[128];
    snprintf(err, sizeof(err), "recevice from back-end: %s, method return: %u", errmsg(m_rsp_head.err()), m_rsp_head.result());
    err_text = err;
}

/**
 * @brief 获取错误字符串描述
 */
const char *CSrpcProxy::GetErrText(void)
{
    snprintf(m_errtext, sizeof(m_errtext), "recevice from back-end: %s, method return: %u", errmsg(m_rsp_head.err()), m_rsp_head.result());
    return m_errtext;
}

/**
 * @brief 序列化请求报文
 * @return !=SRPC_SUCCESS   错误
 *          =SRPC_SUCCESS   成功
 */
int32_t CSrpcProxy::Serialize(char* &pkg, int32_t &len, uint64_t &sequence, const Message &request)
{
    uint64_t color_id = gen_colorid(m_method_name.c_str());
    CRpcHead req_head;

    if (!sequence)
        sequence = newseq();
    m_sequence = sequence;
    req_head.set_sequence(m_sequence);
    req_head.set_coloring((uint32_t)m_coloring);
    req_head.set_color_id(color_id);
    req_head.set_flow_id(color_id ^ m_sequence);
    req_head.set_caller(this->GetCaller());
    req_head.set_method_name(m_method_name);

    return SrpcPackPkg(&pkg, &len, &req_head, &request);
}

/**
 * @brief 序列化请求报文
 * @return !=SRPC_SUCCESS   错误
 *          =SRPC_SUCCESS   成功
 */
int32_t CSrpcProxy::Serialize(uint64_t &sequence, const Message &request, string &out)
{
    int   ret;
    int   len;
    char *pkg;
    
    ret = Serialize(pkg, len, sequence, request);
    if (ret != SRPC_SUCCESS) {
        return ret;
    }

    out.assign(pkg, len);
    free(pkg);

    return SRPC_SUCCESS;
}

/**
 * @brief 反序列化回复报文
 * @return !=SRPC_SUCCESS   错误
 *          =SRPC_SUCCESS   成功
 */
int32_t CSrpcProxy::DeSerialize(const char *pkg, int32_t len, Message &response, uint64_t &sequence, bool check_seq)
{
    int32_t  ret;

    ret = SrpcUnpackPkg(pkg, len, &m_rsp_head, &response);
    if (ret != SRPC_SUCCESS)
    {
        return ret;
    }

    sequence = m_rsp_head.sequence();
    if (check_seq && (m_sequence != sequence))
    {
        return SRPC_ERR_INVALID_SEQUENCE;
    }

    if (m_rsp_head.err() != 0) {
        return SRPC_ERR_BACKEND;
    }

    return SRPC_SUCCESS;
}

/**
 * @brief 反序列化回复报文
 * @return !=SRPC_SUCCESS   错误
 *          =SRPC_SUCCESS   成功
 */
int32_t CSrpcProxy::DeSerialize(const string &in, Message &response, uint64_t &sequence, bool check_seq)
{
    return DeSerialize(in.c_str(), (int32_t)in.size(), response, sequence, check_seq);
}


/**
 * @brief  检查报文是否完整
 * @return <0  报文格式错误
 * @       =0  报文不完整
 * @       >0  报文有效长度
 */
int32_t CSrpcProxy::CheckPkgLen(void *pkg, int32_t len)
{
    return SrpcCheckPkgLen(pkg, len);
}

/**
 * @brief 设置检查报文长度回调函数
 * @info  用于第三方协议检查报文完整性
 */
void CSrpcProxy::SetThirdCheckCb(CheckPkgLenFunc cb)
{
    m_check_cb = cb;
}


/**
 * @brief 第三方法调用接口
 * @info  第三方协议调用接口
 * @param request    请求报文buffer
 *        req_len    请求报文长度
 *        response   回复报文buffer，接口malloc申请，调用者需要free释放
 *        rsp_len    回复报文长度
 *        timeout    超时时间
 * @return SRPC_SUCCESS 成功
 *         其它         失败
 */
int32_t CSrpcProxy::CallMethod(const char *request, int32_t req_len, char* &response, int32_t &rsp_len, int32_t timeout)
{
    int32_t ret;
    int32_t type;
    int32_t cost;
    struct timeval begin, end;
    struct sockaddr_in dst;

    // 1. 获取目标地址
    ret = GetRoute(dst, type);
    if (ret < 0)
    {
        return SRPC_ERR_GET_ROUTE_FAILED;
    }

    // 2. 网络收发包
    char *rsp = NULL;
    gettimeofday(&begin, NULL);
    if (type == PORT_TYPE_UDP)
    {
        rsp = (char *)malloc(64*1024);
        if (NULL == rsp)
        {
            return SRPC_ERR_NO_MEMORY;
        }

        rsp_len = 64*1024;
        ret = mt_udpsendrcv(&dst, (void *)request, req_len, rsp, rsp_len, timeout);
    }
    else
    {
        ret = mt_tcpsendrcv_v2(&dst, (void *)request, req_len, (void **)&rsp, rsp_len, timeout, m_check_cb);
    }

    // 3. 回包统计
    if (ret)
    {
        if (rsp)
            free(rsp);

        // 超时更新路由状态
        if (ret == -3 && errno == ETIME)
        {
            UpdateRoute(dst, 1, 0);
            return SRPC_ERR_RECV_TIMEOUT;
        }
        return SRPC_ERR_SEND_RECV_FAILED;
    }

    gettimeofday(&end, NULL);
    cost = (int32_t)((end.tv_sec*1000 + end.tv_usec/1000) - (begin.tv_sec*1000 + begin.tv_usec/1000));
    UpdateRoute(dst, 0, cost);
    response = rsp;

    return SRPC_SUCCESS;
}


/**
 * @brief SRPC方法调用接口
 * @param request    请求报文，业务自定义的pb格式
 *        response   回复报文，业务自定义的pb格式
 *        rsp_len    回复报文长度
 *        timeout    超时时间
 * @return  SRPC_SUCCESS 成功
 *          其它         失败
 */
int32_t CSrpcProxy::CallMethod(const Message &request, Message &response, int32_t timeout)
{
    int32_t ret = SRPC_SUCCESS;
    int32_t req_len;
    int32_t rsp_len;
    uint64_t oseq, nseq;
    char *  req_pkg = NULL;
    char *  rsp_pkg = NULL;

    ret = Serialize(req_pkg, req_len, oseq, request);
    if (ret != SRPC_SUCCESS)
    {
        return ret;
    }

    ret = this->CallMethod(req_pkg, req_len, rsp_pkg, rsp_len, timeout);
    if (ret != SRPC_SUCCESS)
    {
        goto EXIT;
    }

    ret = DeSerialize(rsp_pkg, rsp_len, response, nseq, true);
    if (ret != SRPC_SUCCESS)
    {
        goto EXIT;
    }

    if (oseq != nseq)
    {
        ret = SRPC_ERR_INVALID_SEQUENCE;
        goto EXIT;
    }

EXIT:
    if (req_pkg)
        free(req_pkg);
    if (rsp_pkg)
        free(rsp_pkg);
    return ret;
    
}



}



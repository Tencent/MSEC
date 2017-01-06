
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


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <string>

#include "monitor.h"
#include "nlbapi.h"
#include "mt_api.h"
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc_network.h"
#include "srpc_log.h"

using namespace SPP_SYNCFRAME;
using namespace NS_MICRO_THREAD;
using namespace srpc;

namespace srpc {

/**
 * @brief 通过业务名获取地址信息
 * @info  外部业务需要将路由信息导入NLB
 */
static int32_t GetRoute(const string &service, struct sockaddr_in &addr, NLB_PORT_TYPE &type)
{
    int32_t ret;
    struct routeid id;
    ret = getroutebyname(service.c_str(), &id);
    if (ret < 0)
    {
        SF_LOG(LOG_ERROR, "getroutebyname failed, ret [%d]", ret);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family         = AF_INET;
    addr.sin_addr.s_addr    = id.ip;
    addr.sin_port           = htons(id.port);

    type = id.type;
    return 0;
}

/**
 * @brief 更新路由信息(回包统计)
 */
static void UpdateRoute(const string &service, struct sockaddr_in &addr, int32_t failed, int32_t cost)
{
    int32_t ret;

    ret = updateroute(service.c_str(), addr.sin_addr.s_addr, failed, cost);
    if (ret < 0)
    {
        SF_LOG(LOG_DEBUG, "updateroute failed, ret [%d]", ret);
    }
}

// 构造函数
CRpcNetAdpt::CRpcNetAdpt(const string &service_name, const string &method_name, 
                         const CRpcHead &head, Message *request,
                         Message *response, int32_t timeout, int32_t proto)
{
    m_seq           = newseq();
    m_req_head      = head;
    m_service_name  = service_name;
    m_method_name   = method_name;
    m_request       = request;
    m_response      = response;
    m_timeout       = timeout;
    m_proto         = proto;
    gettimeofday(&m_begin, NULL);
}

// 初始化请求报文消息头部
void CRpcNetAdpt::InitReqHead()
{
    m_req_head.set_sequence(m_seq);
    // head.set_coloring(); 这个不需要设置
    m_req_head.set_color_id(gen_colorid(m_method_name.c_str()));
    m_req_head.set_method_name(m_method_name);
    m_req_head.set_caller(SrpcServiceName());
    //*(m_req_head.add_caller_stack()) = m_service_name;
    if (!m_req_head.has_flow_id() || !m_req_head.flow_id()) { // 如果客户端没有设置flow id，就默认使用当前的seq作为flow id
        m_req_head.set_flow_id(m_seq);
    }
}

/**
 * @brief RPC收发包处理主函数
 */
int32_t CRpcNetAdpt::SendRecv(void)
{
    int32_t cost;
    int32_t ret, result = 0;
    int32_t slen, rlen;
    char *send_pkg = NULL;
    char *recv_pkg = NULL;
    char attr[256];

    // 1. 获取路由地址
    ret = GetRoute(m_service_name, m_addr, m_type);
    if (ret < 0)
    {
        snprintf(attr, sizeof(attr), "%s [%s]", errmsg(SRPC_ERR_GET_ROUTE_FAILED), m_service_name.c_str());
        RPC_REPORT(attr);
        NGLOG_ERROR("get route (%s) failed, ret [%d]", m_service_name.c_str(), ret);
        result = SRPC_ERR_GET_ROUTE_FAILED;
        goto EXIT_LABEL;
    }

    // 2. 初始化请求头
    InitReqHead();

    // 3. 打包
    ret = SrpcPackPkg(&send_pkg, &slen, &m_req_head, m_request);
    if (ret < 0)
    {
        NGLOG_ERROR("pack request failed, ret [%d]", ret);
        result = ret;
        goto EXIT_LABEL;
    }

    // 4. 收发包
    if ((m_type == NLB_PORT_TYPE_UDP)
        || ((m_type = NLB_PORT_TYPE_ALL) && (m_proto == PORT_TYPE_UDP)))
    {
        rlen = 64*1024;
        recv_pkg = (char *)malloc(rlen);
        if (NULL == recv_pkg)
        {
            NGLOG_ERROR("no memory");
            result = SRPC_ERR_NO_MEMORY;
            goto EXIT_LABEL;
        }

        ret = mt_udpsendrcv(&m_addr, send_pkg, slen, recv_pkg, rlen, m_timeout);
    }
    else
    {
        ret = mt_tcpsendrcv_v2(&m_addr, send_pkg, slen, (void **)&recv_pkg, rlen, m_timeout, SrpcCheckPkgLen);
    }

    if (ret < 0)
    {
        result = SRPC_ERR_SEND_RECV_FAILED;
        UpdateRoute(m_service_name.c_str(), m_addr, 1, 0);
        NGLOG_ERROR("sendrecv (%s:%u) failed, ret [%d] [%m]", inet_ntoa(m_addr.sin_addr), ntohs(m_addr.sin_port), ret);
        goto EXIT_LABEL;
    }

    // 5. 计算本次请求时延，回包统计
    gettimeofday(&m_end, NULL);
    cost = (int32_t)((m_end.tv_sec*1000 + m_end.tv_usec/1000) - (m_begin.tv_sec*1000 + m_begin.tv_usec/1000));
    UpdateRoute(m_service_name.c_str(), m_addr, 0, cost);

    // 6. 解回包
    ret = SrpcUnpackPkg(recv_pkg, rlen, &m_rep_head, m_response);
    if (ret < 0)
    {
        NGLOG_ERROR("unpack reponse failed, ret [%d]", ret);
        result = ret;
        goto EXIT_LABEL;
    }

    if (ret == 1)
    {
        NGLOG_ERROR("recevice response head [%s]", m_rep_head.ShortDebugString().c_str());
        result = SRPC_ERR_NO_BODY;
        goto EXIT_LABEL;
    }

    if (m_rep_head.sequence() != m_seq)
    {
        NGLOG_DEBUG("invalid reponse sequence [%s]", m_rep_head.ShortDebugString().c_str());
        NGLOG_ERROR("invalid reponse sequence");
        result = SRPC_ERR_INVALID_SEQUENCE;
        goto EXIT_LABEL;
    }

    if (m_rep_head.err() != SRPC_SUCCESS)
    {
        NGLOG_ERROR("error form back-end: %s", errmsg(m_rep_head.err()));
        result = (int32_t)m_rep_head.err();
        goto EXIT_LABEL;
    }

    snprintf(attr, sizeof(attr), "call [%s] cost [%s]", m_service_name.c_str(), Cost2TZ(cost));
    RPC_REPORT(attr);

EXIT_LABEL:
    if (send_pkg)
        free(send_pkg);
    if (recv_pkg)
        free(recv_pkg);
    return result;
}

}


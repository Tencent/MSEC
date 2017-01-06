
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


#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <string>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include "nlbapi.h"
#include "monitor.h"
#include "srpc.pb.h"
#include "srpc_proto.h"
#include "srpc_log.h"
#include "srpc_service.h"
#include "srpc_network.h"
#include "srpc_intf.h"
#include "http_support.h"

using namespace SPP_SYNCFRAME;
using namespace NS_MICRO_THREAD;
using namespace srpc;
using namespace google::protobuf;

#define SRPC_PACK_PKG(pkg, len, head, body) ({ \
        int32_t _ret; \
        if ((NULL == body) || (body->ByteSize() == 0)) { \
            _ret = SrpcPackPkgNoBody(pkg, len, head); \
        } else { \
            _ret = SrpcPackPkg(pkg, len, head, body); \
        } \
        _ret; \
    })

namespace srpc
{

/**
 * @brief  框架内部调用方法接口
 * @param  service_name  业务名   一级业务名.二级业务名 "Login.ptlogin"
 *         method_name   方法名   被调方方法名的pb全称  "echo.EchoService.Echo"
 *         request       请求报文
 *         response      回复报文
 *         timeout       超时时间
 *         proto         协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
 * @return SRPC_SUCCESS  成功
 *         其它          失败
 */
int32_t CallMethod(const std::string &service_name,
                   const std::string &method_name,
                   const Message &request,
                   Message &response,
                   int32_t timeout,
                   int32_t proto)
{
    int32_t  ret;
    CRpcHead head;
    std::string attr;

    // 获取当前的上下文环境
    CSyncMsg *msg = CSyncFrame::Instance()->GetCurrentMsg();
    if (msg)
    {
        head = ((CRpcMsgBase *)msg)->GetRpcHead();
    }

    // 上报
    attr = "call [" + service_name + ":" + method_name + "]";
    RPC_REPORT(attr.c_str());

    // RPC网络收发
    CRpcNetAdpt handler(service_name, method_name, head, (Message *)&request, (Message *)&response, timeout, proto);
    ret = handler.SendRecv();
    if (ret < 0)
    {
        attr = attr + " fail:" + errmsg(ret);
        RPC_REPORT(attr.c_str());
        NGLOG_ERROR("CRpcNetAdpt sendrecv failed, [%s]", errmsg(ret));
    }
    else
    {
        attr = attr + " success";
        RPC_REPORT(attr.c_str());
        NGLOG_DEBUG("CRpcNetAdpt sendrecv success");
    }

    return ret;
}

// msg基类构造函数
CRpcMsgBase::CRpcMsgBase()
{
    m_method_info   = NULL;
    m_request       = NULL;
    m_response      = NULL;
    m_proto_type    = PROTO_TYPE_PB;
}

// msg基类析构函数
CRpcMsgBase::~CRpcMsgBase()
{
    if (m_request)
        delete m_request;
    if (m_response)
        delete m_response;
}

void CRpcMsgBase::SetMethodInfo(CMethodInfo * method_info)
{
    m_method_info   = method_info;
    m_request       = m_method_info->m_request->New();
    m_response      = m_method_info->m_response->New();
}

void CRpcMsgBase::SetProtoType(int type)
{
    m_proto_type = type;
}

int CRpcMsgBase::GetProtoType(void)
{
    return m_proto_type;
}


// 消息处理过程
int CRpcMsgBase::HandleProcess()
{
    int32_t msg_len;
    char*   rsp_buff;
    std::string rsp_str;
    std::string body_str;
    blob_type resblob;

    // 1. 新建请求和回复报文
    m_service   = m_method_info->m_service;
    m_method    = m_method_info->m_method;

    // 2. 记录流水信息
    //GetLogOption().Set("Coloring", "1");
    //NGLOG_INFO("srpc system water");
    //GetLogOption().Set("Coloring", "0");

    // 3. 解析请求报文
    int32_t ret = SrpcUnpackPkg((char *)this->GetReqPkg().data(), (int)this->GetReqPkg().size(), &m_head, m_request);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        m_head.set_err(ret);
        NGLOG_ERROR("unpack request package failed, [%s]", errmsg(ret));
        goto EXIT_LABEL;
    }

    if (!m_head.has_flow_id() || !m_head.flow_id())
    {
        m_head.set_flow_id(newseq());
        GetLogOption().Set("ReqID", m_head.flow_id());
    }

    GetLogOption().Set("Coloring", m_head.coloring());

    NGLOG_DEBUG("process request: %s", m_request->DebugString().c_str());

    // 4. 回调处理
    m_service->CallMethod(m_method, this, m_request, m_response, NULL);
    if (Failed())
    {
        RPC_REPORT(errmsg(SRPC_ERR_SERVICE_IMPL_FAILED));
        m_head.set_err(SRPC_ERR_SERVICE_IMPL_FAILED);
        NGLOG_ERROR("msg process failed, method %s", m_method_info->m_method->full_name().c_str());
        goto EXIT_LABEL;
    }

    NGLOG_DEBUG("msg process ok, method %s",  m_method_info->m_method->full_name().c_str());
        
EXIT_LABEL:
    if (GetProtoType() == PROTO_TYPE_PB)
    {
        goto PB_RET;
    }
    else
    {
        goto JSON_RET;
    }

JSON_RET:
    try {
        body_str = CHttpHelper::Pb2Json(*m_response);
    } catch (std::exception &e){
        body_str = "pb2json failed";
    }

    CHttpHelper::GenJsonResponse((int)m_head.err(), body_str.c_str(), body_str.size(), rsp_str);
    resblob.data = (char *)rsp_str.data();
    resblob.len  = (int)rsp_str.size();
    this->SendToClient(resblob);

    return 0;

PB_RET:
    // 5. 组包并回包
    ret = SRPC_PACK_PKG(&rsp_buff, &msg_len, &m_head, m_response);
    if (ret < 0) {
        RPC_REPORT(errmsg(ret));
        NGLOG_ERROR("pack package failed, method %s, [%s]",  m_method_info->m_method->full_name().c_str(), errmsg(ret));
        return -1;
    }

    resblob.data = rsp_buff;
    resblob.len  = msg_len;
    this->SendToClient(resblob);

    free(rsp_buff);

    return 0;  // 只要成功回包, 都返回0
}

// 构造与析构函数
CMethodManager* CMethodManager::m_instance = new CMethodManager();
CMethodManager::CMethodManager()
{
}

CMethodManager::~CMethodManager()
{
    while (!m_method_name_map.empty())
    {
        delete (m_method_name_map.begin())->second;
        m_method_name_map.erase(m_method_name_map.begin());
    }
}


// 注册服务器管理
int32_t CMethodManager::RegisterService(Service *service, CRpcMsgBase* msg)
{
    const ServiceDescriptor* rpc_descriptor = service->GetDescriptor();
    const MethodDescriptor*  rpc_method;
    const Message *rpc_request;
    const Message *rpc_response;

    NGLOG_DEBUG("register service [%s]", rpc_descriptor->full_name().c_str());

    // 注册方法对象
    for (int i = 0; i < rpc_descriptor->method_count(); i++)
    {
        rpc_method   = rpc_descriptor->method(i);
        rpc_request  = &service->GetRequestPrototype(rpc_method);
        rpc_response = &service->GetResponsePrototype(rpc_method);

        CMethodInfo* method_info = new CMethodInfo(service, msg, rpc_request, rpc_response, rpc_method);
        if (m_method_name_map[rpc_method->full_name()])
        {
            NGLOG_ERROR("register method [%s] repeated", rpc_method->full_name().c_str());
            return -1;
        }

        m_method_name_map[rpc_method->full_name()] = method_info;
        NGLOG_DEBUG("register method [%s]", rpc_method->full_name().c_str());
    }

    return 0;
}

// 根据消息头, 创建消息信息
CMethodInfo* CMethodManager::GetMethodInfo(const string &method_name)
{
    CMethodNameMap::iterator it_name = m_method_name_map.find(method_name);
    if (it_name != m_method_name_map.end())
    {
        return it_name->second;
    }

    return NULL;
}


// 根据消息头, 创建消息信息
CRpcMsgBase* CMethodManager::CreateMsgObj(const string &method_name)
{
    CMethodInfo* method_info  = GetMethodInfo(method_name);
    if (NULL == method_info)
    {
        return NULL;
    }

    CRpcMsgBase* msg = method_info->m_msg->New();
    if (NULL == msg)
    {
        return NULL;
    }

    msg->SetMethodInfo(method_info);

    return msg;
}

}



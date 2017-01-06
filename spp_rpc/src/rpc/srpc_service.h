
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


#ifndef __SRPC_SERVICE_H__
#define __SRPC_SERVICE_H__

#include <stdint.h>
#include <string>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include "SyncMsg.h"
#include "srpc_ctrl.h"
#include "srpc_intf.h"
#include "srpc.pb.h"

using namespace srpc;
using namespace google::protobuf;
using namespace SPP_SYNCFRAME;

namespace srpc {

class CMethodInfo;

/**
 * @brief  框架内部调用方法接口
 * @param  service_name  业务名   一级业务名.二级业务名 "Login.ptlogin"
 *         method_name   方法名   被调方方法名的pb全称  "echo.EchoService.Echo"
 *         request       请求报文
 *         response      回复报文
 *         timeout       超时时间
 *         proto         协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
 * @return =0  成功
 *         !=0 失败
 */
int32_t CallMethod(const std::string &service_name,
                   const std::string &method_name,
                   const Message &request,
                   Message &response,
                   int32_t timeout=800,
                   int32_t proto=PORT_TYPE_ALL);
enum {
    PROTO_TYPE_PB   = 1,
    PROTO_TYPE_JSON = 2,
};

/**
 * @brief RPC的消息基类定义
 */
class CRpcMsgBase: public CSyncMsg, public CRpcCtrl
{
public:

    // 构造与析构函数
    CRpcMsgBase();
    virtual ~CRpcMsgBase();

    // 消息处理过程
    virtual int HandleProcess();

    // 对象注册克隆用法
    virtual CRpcMsgBase* New() = 0;

    // 设置协议类型 PROTO_TYPE_PB/PROTO_TYPE_JSON
    void SetProtoType(int type = PROTO_TYPE_PB);

    // 获取协议类型
    int GetProtoType(void);

    // 设置method信息
    void SetMethodInfo(CMethodInfo *method_info);

    // 获取日志选项
    CLogOption& GetLogOption() {
        return m_log_options;
    }

    // 获取请求RPC头部
    CRpcHead& GetRpcHead() {
        return m_head;
    }

    // 设置请求RPC头部
    void SetRpcHead(const CRpcHead &head) {
        m_head = head;
    }

public:
    CLogOption      m_log_options;
    CRpcHead        m_head;
    CMethodInfo*    m_method_info;
    Message*        m_request;
    Message*        m_response;
    Service*        m_service;
    const MethodDescriptor* m_method;
    int             m_proto_type;
};

/**
 * @brief RPC的方法管理单元定义
 */
class CMethodInfo
{
public:

    // 构造函数定义    
    CMethodInfo(Service *service, CRpcMsgBase *msg, const Message *request,
                const Message *response, const MethodDescriptor *method)
    {
        m_service   = service;
        m_msg       = msg;
        m_request   = request;
        m_response  = response;
        m_method    = method;
    }

    Service*                m_service;
    CRpcMsgBase*            m_msg;
    const Message*          m_request;
    const Message*          m_response;
    const MethodDescriptor* m_method;
};

typedef std::map<std::string, CMethodInfo*> CMethodNameMap;

/**
 * @brief RPC的方法全局注册管理定义
 */
class CMethodManager 
{
public:

    // 构造与析构定义
    CMethodManager();
    ~CMethodManager();

    // 静态的单例指针获取
    static CMethodManager* Instance()   {
        return m_instance;
    }

    // 注册服务器管理
    int32_t RegisterService(Service *service, CRpcMsgBase* msg);

    // 根据消息头, 创建消息信息
    CMethodInfo* GetMethodInfo(const std::string& method_name);

    // 根据消息头, 创建消息信息
    CRpcMsgBase* CreateMsgObj(const std::string& method_name);

private:

    CMethodNameMap m_method_name_map;
    static CMethodManager* m_instance;
};

}

#endif


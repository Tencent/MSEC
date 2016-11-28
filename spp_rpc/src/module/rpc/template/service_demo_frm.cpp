
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
 *  @filename service_demo.cpp
 *  @info     demo模板
 */
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "syncincl.h"
#include "srpcincl.h"
#include "service_$(MODULE_REPLACE)_impl.hpp"

using namespace srpc;

/**
 * @brief 业务模块初始化插件接口(proxy,worker)
 * @param conf -业务配置文件信息
 * @param server -业务进程信息
 * @return 0 - 成功, 其它失败
 */
extern "C" int spp_handle_init(void* arg1, void* arg2)
{
    int32_t ret = 0;
    const char* etc = (const char*)arg1;
    CServerBase* base = (CServerBase*)arg2;
    NGLOG_DEBUG("spp_handle_init, config:%s, servertype:%d", etc, base->servertype());

    if (base->servertype() == SERVER_TYPE_WORKER) // WORKER进程初始化
    {
        // 注册RPC服务与消息信息
        $(CODE_BEGIN)$(REGIST_BEGIN)ret += CMethodManager::Instance()->RegisterService(new CRpc$(SERVICE_REPLACE)Impl, new C$(SERVICE_REPLACE)Msg);$(REGIST_END)$(CODE_END)
        if (ret != 0)
        {
            NGLOG_ERROR("service regist failed, ret %d", ret);
            return -1;
        }
    }

    // TODO: 业务初始化
    
    return 0;
}


/**
 * @brief 业务模块检查报文合法性与分包接口(proxy)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return ==0  数据包还未完整接收,继续等待
 *         > 0  数据包已经接收完整, 返回包长度
 *         < 0  数据包非法, 连接异常, 将断开TCP连接
 */
extern "C" int spp_handle_input(unsigned flow, void* arg1, void* arg2)
{
    int32_t ret;
    blob_type* blob = (blob_type*)arg1;
    TConnExtInfo* extinfo = (TConnExtInfo*)blob->extdata;

    NGLOG_DEBUG("spp_handle_input flow:%d, buffer len:%d, client ip:%s",
                 flow,
                 blob->len,
                 inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    ret = SrpcCheckPkgLen((void*)blob->data, blob->len);

    // 如果UDP包，一次包没有收完，表示错误
    if ((extinfo->type_ == SOCK_TYPE_UDP) && (ret == 0))
    {
        NGLOG_ERROR("spp_handle_input udp package failed: flow:%d, buffer len:%d, client ip:%s",
                    flow,
                    blob->len,
                    inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));
        return SRPC_ERR_INVALID_PKG;
    }

    // 如果解包失败，打印错误日志
    if (ret < 0)
    {
        NGLOG_ERROR("spp_handle_input failed: flow:%d, buffer len:%d, client ip:%s, error msg: %s",
                    flow,
                    blob->len,
                    inet_ntoa(*(struct in_addr*)&extinfo->remoteip_),
                    errmsg(ret));
    }

    return ret;
}


/**
 * @brief 业务模块报文按worker组分发接口(proxy)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return 处理该报文的组id信息
 */
extern "C" int spp_handle_route(unsigned flow, void* arg1, void* arg2)
{
    // NGSE只有一个worker组，固定路由到组1
    return 1;
}

/**
 * @brief 业务模块报文, worker组的处理接口(worker)
 * @param block -消息指针块信息
 * @param server -业务进程信息
 * @return 0 处理成功, 其它为失败
 */
extern "C" int spp_handle_process(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob         = (blob_type*)arg1;
    CTCommu* commu          = (CTCommu*)blob->owner;
    TConnExtInfo* extinfo   = (TConnExtInfo*)blob->extdata;
    CServerBase* base       = (CServerBase*)arg2;
    CRpcMsgBase* msg        = NULL;
    CMethodManager* method_mng = CMethodManager::Instance();
    char attr[256];

    NGLOG_DEBUG("spp_handle_process flow:%d, buffer len:%d, client ip:%s",
                flow,
                blob->len,
                inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));

    // 1. 解析消息头信息
    CRpcHead  rpc_head;
    int32_t ret = SrpcUnpackPkgHead((char*)blob->data, blob->len, &rpc_head);
    if (ret != SRPC_SUCCESS)
    {
        RPC_REPORT(errmsg(ret));
        rpc_head.set_err(ret);
        NGLOG_ERROR("%s", ret);
        goto EXIT_LABEL;
    }

    NGLOG_DEBUG("parse msg head: %s", rpc_head.DebugString().c_str());

    // 2. 根据消息头获取动态msg
    msg = method_mng->CreateMsgObj(rpc_head.method_name());
    if (NULL == msg)
    {
        RPC_REPORT(errmsg(SRPC_ERR_INVALID_METHOD_NAME));
        rpc_head.set_err(SRPC_ERR_INVALID_METHOD_NAME);
        NGLOG_ERROR("invalid request methodname %s", rpc_head.method_name().c_str());
        goto EXIT_LABEL;
    }
    
    // 3. 派发消息, 等待被调度
    msg->SetServerBase(base);
    msg->SetTCommu(commu);
    msg->SetFlow(flow);
    msg->SetMsgTimeout(1000);
    msg->SetReqPkg(blob->data, blob->len);
    msg->GetLogOption().Set("ReqID", rpc_head.flow_id());
    msg->GetLogOption().Set("ClientIP", inet_ntoa(*(struct in_addr*)&extinfo->remoteip_));
    msg->GetLogOption().Set("ServerIP", inet_ntoa(*(struct in_addr*)&extinfo->localip_));
    msg->GetLogOption().Set("RPCName", rpc_head.method_name().c_str());
    msg->GetLogOption().Set("Caller", rpc_head.caller().c_str());
    msg->GetLogOption().Set("ServiceName", base->servicename().c_str());
    //msg->GetLogOption().Set("ColorID", rpc_head.color_id());
    msg->GetLogOption().Set("Coloring", "0");

    // 4. 上报monitor收到调用方请求
    snprintf(attr, sizeof(attr), "caller [%s]", rpc_head.caller().c_str());
    RPC_REPORT(attr);

    CSyncFrame::Instance()->Process(msg);

    return 0;

EXIT_LABEL:

    // 5. 处理失败，组包并回包
    int32_t msg_len;
    char*   rsp_buff;
    ret = SrpcPackPkgNoBody(&rsp_buff, &msg_len, &rpc_head);
    if (ret != SRPC_SUCCESS) {
        RPC_REPORT(errmsg(ret));
        NGLOG_ERROR("pack package failed: %s",  errmsg(ret));
        return -1;
    }

    blob_type resblob;
    resblob.data = rsp_buff;
    resblob.len  = msg_len;
    commu->sendto(flow, &resblob, arg2);

    free(rsp_buff);

    return 0;  // 只要成功回包, 都返回0
}


/**
 * @brief 业务服务终止接口函数(proxy/worker)
 * @param server -业务进程信息
 */
extern "C" void spp_handle_fini(void* arg1, void* arg2)
{
    CServerBase* base = (CServerBase*)arg2;
    NGLOG_DEBUG("spp_handle_fini");

    if (base->servertype() == SERVER_TYPE_WORKER )
    {
        CSyncFrame::Instance()->Destroy();
    }
}



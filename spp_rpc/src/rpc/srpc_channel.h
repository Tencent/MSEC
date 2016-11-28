
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
 * @filename srpc_channel.h
 */
#ifndef __SRPC_CHANNEL_H__
#define __SRPC_CHANNEL_H__

#include <stdint.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>

using namespace google::protobuf;
using namespace srpc;

namespace srpc {

/**
 * @brief CRpcChannel定义
 */
class CRpcChannel : public RpcChannel 
{
 public:

    // 构造与析构函数
    CRpcChannel(const string& endpoint);
    virtual ~CRpcChannel();

    // 解析endpoint地址信息
    int32_t ParseEndpoint(struct sockaddr_in* address);

    // 设置超时时间
    void SetTimeout(uint32_t timeout) {
        m_timeout = timeout;
    };

    // 获取应答的头信息
    CRpcHead& GetResponseHead() {
        return m_rsp_head;
    };

    /**
     * @brief  网络收发接口
     * @info   业务可继承该类，实现自己的网络收发接口
     * @param  req_buf   [入参] 请求报文buffer
     *         req_len   [入参] 请求报文长度
     *         rsp_buf   [出参] 回复报文buffer，需要调用者释放
     *         rsp_len   [出参] 回复报文长度
     *         timout    [入参] 超时时间
     * @return SRPC_SUCCESS 成功
     *         其它         失败
     */
    virtual int32_t SendAndRecv(char* req_buf, int32_t req_len, char **rsp_buf, int32_t *rsp_len, int32_t timeout) = 0;


    /**
     * @brief 继承自RpcChannel的接口函数
     */
    void CallMethod(const MethodDescriptor* method,
                    RpcController* controller,
                    const Message* request,
                    Message* response,
                    Closure* done);

protected:

    uint64_t       m_req_seq;           // 请求包携带的seq信息
    uint32_t       m_timeout;           // 默认的超时时间MS 默认1000
    char*          m_rsp_buf;           // 临时的buff指针
    int32_t        m_rsp_len;           // 实际的应答长度
    string         m_endpoint;          // 地址信息
    string         m_err_msg;           // 异常消息记录
    CRpcHead       m_rsp_head;          // 请求应答头信息
};


/**
 * @brief 基于微线程的udp异步rpc通道
 */
class CRpcUdpChannel : public CRpcChannel
{
public:

    // 构造与析构
    CRpcUdpChannel(const string& endpoint) : CRpcChannel(endpoint) {}
    virtual ~CRpcUdpChannel(){}

    // 发送与接收接口函数
    int32_t SendAndRecv(char* req_buf, int32_t req_len, char **rsp_buf, int32_t *rsp_len, int32_t timeout);
};

/**
 * @brief 基于微线程的tcp异步rpc通道
 */
class CRpcTcpChannel : public CRpcChannel
{
public:

    // 构造与析构
    CRpcTcpChannel(const string& endpoint) : CRpcChannel(endpoint) {}
    virtual ~CRpcTcpChannel(){}

    // 发送与接收接口函数
    int32_t SendAndRecv(char* req_buf, int32_t req_len, char **rsp_buf, int32_t *rsp_len, int32_t timeout);
};

};

#endif


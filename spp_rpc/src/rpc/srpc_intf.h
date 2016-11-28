
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


#ifndef __SRPC_INTF_H__
#define __SRPC_INTF_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>

#include "srpc.pb.h"

using namespace google::protobuf;
using namespace std;

namespace srpc
{

enum {
    ENDPOINT_TYPE_NLB    = 1,       // 后端通过NLB寻址
    ENDPOINT_TYPE_ADDR   = 2,       // 后端直接直接地址
    ENDPOINT_TYPE_UNKOWN = 100,     // 未知类型
};

enum {
    PORT_TYPE_UDP = 1,  // UDP
    PORT_TYPE_TCP = 2,  // TCP
    PORT_TYPE_ALL = 3,  // ALL
};

class CProxyBase
{
public:
    CProxyBase(const string &endpoint); // "Login.ptlogin"  or "127.0.0.1:8888@udp"
    CProxyBase() {}
    virtual ~CProxyBase();

    void SetEndPoint(const string &endpoint);
    const string &GetEndPoint(void);

    /**
     * @brief 设置调用方业务名
     */
    void SetCaller(const string &caller);

    /**
     * @brief 获取本地业务名
     */
    const string &GetCaller(void);

    /**
     * @brief 设置协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
     * @info  如果不设置，采用NLB获取的协议方式，优先使用TCP
     */
    void SetProtoType(int proto);

    /**
     * @brief 获取协议(PORT_TYPE_UDP/PORT_TYPE_TCP/PORT_TYPE_ALL)
     */
    int GetProtoType(void);

    /**
     * @brief 获取路由
     */
    int32_t GetRoute(struct sockaddr_in &addr, int32_t &type);

    /**
     * @brief 更新路由信息(回包统计)
     */
    void UpdateRoute(struct sockaddr_in &addr, int32_t failed, int32_t cost);

private:
    /**
     * @brief 解析endpoint地址信息
     */
    void ParseEndpoint(const string &endpoint);

private:

    int32_t m_type;                 // 后端寻址类型
    int32_t m_port_type;            // 端口类型
    string  m_endpoint;             // endpoint
    string  m_caller;               // 本地业务名
    string  m_service_name;         // NLB方式业务名
    struct sockaddr_in m_address;   // ADDR方式地址
};

/**
 * @brief 检测报文是否接收完整的回调函数定义
 * @param buf 报文保存缓冲区
 * @param len 已经接收的长度
 * @return >0 实际的报文长度; 0 还需要等待接收; <0 报文异常
 */
typedef int32_t (*CheckPkgLenFunc)(void *buf, int32_t len);

/**
 * @brief SRPC代理类
 */
class CSrpcProxy: public CProxyBase
{
public:
    CSrpcProxy(const string &endpoint);
    CSrpcProxy() {}
    ~CSrpcProxy();

    /**
     * @brief 设置调用方法名
     */
    void SetMethod(const string &method); // 调用SRPC方法需要设置

    /**
     * @brief 设置日志是否染色
     */
    void SetColoring(bool coloring);

    /**
     * @brief 获取调用结果
     */
    void GetResult(int32_t &fret, int32_t &sret);

    /**
     * @brief 获取是否失败
     */
    bool Failed(void);

    /**
     * @brief 获取错误字符串描述
     */
    void GetErrText(string &err_text);

    /**
     * @brief 获取错误字符串描述
     */
    const char *GetErrText(void);

    /**
     * @brief  序列化请求报文
     * @info   1. 业务需要自己free(pkg)
     *         2. 如果传入的sequence为0，接口内部会自动生成一个随机sequence
     * @param  pkg     [IN]    打包后的报文buffer，业务负责free
     *         len     [IN]    打包后的报文buffer长度
     *         sequence[INOUT] 报文唯一标识符
     *         request [OUT]   业务请求包体
     * @return SRPC_SUCCESS     成功
     *         其它             通过errmsg(err)获取错误详细信息
     */
    int32_t Serialize(char* &pkg, int32_t &len, uint64_t &sequence, const Message &request);

    /**
     * @brief  序列化请求报文
     * @info   如果传入的sequence为0，接口内部会自动生成一个随机sequence
     * @param  sequence[INOUT] 报文唯一标识符
     *         request [IN]    业务请求包体
     *         out     [OUT]   业务报文
     * @return SRPC_SUCCESS     成功
     *         其它             通过errmsg(ret)获取错误详细信息
     */
    int32_t Serialize(uint64_t &sequence, const Message &request, string &out);

    /**
     * @brief  反序列化回复报文
     * @info   业务可以选择接口内部检查seqence或者自己做校验
     *         接口内部检查sequence时，需要保证一次打解包过程中，没有其它人使用
     * @param  pkg       [IN]  报文buffer
     *         len       [IN]  报文长度
     *         reponse   [OUT] 解包后的包体
     *         sequence  [OUT] 报文唯一标识符
     *         check_seq [IN]  是否需要检查seq
     * @return SRPC_SUCCESS     成功
     *         SRPC_ERR_BACKEND 失败，GetErrText获取详细信息
     *         其它             通过errmsg(ret)获取错误详细信息
     */
    int32_t DeSerialize(const char *pkg, int32_t len, Message &response, uint64_t &sequence, bool check_seq = false);

    /**
     * @brief  反序列化回复报文
     * @info   业务可以选择接口内部检查seqence或者自己做校验
     *         接口内部检查sequence时，需要保证一次打解包过程中，没有其它人使用
     * @param  in        [IN]  报文buffer
     *         reponse   [OUT] 解包后的包体
     *         sequence  [OUT] 报文唯一标识符
     *         check_seq [IN]  是否需要检查seq
     * @return SRPC_SUCCESS     成功
     *         SRPC_ERR_BACKEND 失败，GetErrText获取详细信息
     *         其它             通过errmsg(ret)获取错误详细信息
     */
    int32_t DeSerialize(const string &in, Message &response, uint64_t &sequence, bool check_seq = false);

    /**
     * @brief  检查报文是否完整
     * @return <0  报文格式错误
     * @       =0  报文不完整
     * @       >0  报文有效长度
     */
    int32_t CheckPkgLen(void *pkg, int32_t len);

    /**
     * @brief 设置检查报文长度回调函数
     * @info  用于第三方协议检查报文完整性
     */
    void SetThirdCheckCb(CheckPkgLenFunc cb);

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
    int32_t CallMethod(const char *request, int32_t req_len, char* &response, int32_t &rsp_len, int32_t timeout);

    /**
     * @brief SRPC方法调用接口
     * @param request    请求报文，业务自定义的pb格式
     *        response   回复报文，业务自定义的pb格式
     *        rsp_len    回复报文长度
     *        timeout    超时时间
     * @return  SRPC_SUCCESS 成功
     *          其它         失败
     */
    int32_t CallMethod(const Message &request, Message &response, int32_t timeout);

private:

    string   m_method_name;     // SRPC方法名
    bool     m_coloring;        // 设置是否日志染色
    uint64_t m_sequence;        // 网络收发sequence
    CRpcHead m_rsp_head;        // SRPC消息头
    uint64_t reserved;          // 回复报文sequence存放
    char     m_errtext[128];    // 错误描述信息
    CheckPkgLenFunc m_check_cb; // 第三方协议检查报文函数
};


}

#endif


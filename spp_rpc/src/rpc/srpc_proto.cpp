
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
 * @filename srpc_proto.cpp
 */

#include <stdint.h>
#include <arpa/inet.h>
#include "srpc.pb.h"
#include "srpc_comm.h"
#include "srpc_proto.h"

using namespace google::protobuf;
using namespace srpc;

namespace srpc {

/**
 * @brief  SRPC报文打包函数
 * @info   [注意] 打包后的报文为出参，函数内部分配内存，调用者需要free
 * @param  pkg  [出参]打包完后报文的buffer
 *         len  [出参]打包完后，报文的长度
 *         head [入参]报文头,PB格式
 *         body [入参]报文体,PB格式
 * @return =0   打包成功
 * @       <0   打包失败
 */
int32_t SrpcPackPkg(char** pkg, int32_t *len, const CRpcHead *head, const Message *body)
{
    char *buf;

    // 参数检查
    if (NULL == head || NULL == pkg || NULL == len || NULL == body)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 初始化报文长度
    int32_t head_len = head->ByteSize();
    int32_t body_len = body->ByteSize();
    int32_t msg_len  = PROTO_HEAD_MIN_LEN + head_len + body_len;

    // 检查包头是否初始化
    if (!head->IsInitialized() || !head_len)
    {
        return SRPC_ERR_HEADER_UNINIT;
    }

    // 检查包体是否初始化
    if (!body->IsInitialized() || !body_len)
    {
        return SRPC_ERR_BODY_UNINIT;
    }

    // 分配内存
    buf = (char *)malloc(msg_len);
    if (NULL == buf)
    {
        return SRPC_ERR_NO_MEMORY;
    }

    // 组装报文
    // 格式 "(head_len body_len head body)"
    char *pos = buf;
    *pos++ = PROTO_RPC_STX;
    *(int32_t *)pos = htonl(head_len);
    pos += sizeof(int32_t);
    *(int32_t *)pos = htonl(body_len);
    pos += sizeof(int32_t);

    // 序列化包头
    if (!head->SerializeToArray(pos, head_len))
    {
        free(buf);
        return SRPC_ERR_INVALID_PKG_HEAD;
    }
    pos += head_len;

    // 序列化包体
    if (body_len && !body->SerializeToArray(pos, body_len))
    {
        free(buf);
        return SRPC_ERR_INVALID_PKG_BODY;
    }
    pos += body_len;
    *pos = PROTO_RPC_ETX;

    // 设置出参
    *pkg = buf;
    *len = msg_len;

    return SRPC_SUCCESS;
}

/**
 * @brief  SRPC报文打包函数
 * @info   [注意] 打包后的报文为出参，函数内部分配内存，调用者需要free
 *         [注意] 包体为二进制
 * @param  pkg  [出参]打包完后报文的buffer
 *         len  [出参]打包完后，报文的长度
 *         head [入参]报文头,PB格式
 *         body [入参]报文体,二进制
 * @return =SRPC_SUCCESS    打包成功
 * @       !=SRPC_SUCCESS   打包失败
 */
int32_t SrpcPackPkg(char** pkg, int32_t *len, const CRpcHead *head, const string& body)
{
    char *buf;

    // 参数检查
    if (NULL == head || NULL == pkg || NULL == len)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 初始化报文长度
    int32_t head_len = head->ByteSize();
    int32_t body_len = body.size();
    int32_t msg_len  = PROTO_HEAD_MIN_LEN + head_len + body_len;

    // 检查包头是否初始化
    if (!head->IsInitialized() || !head_len)
    {
        return SRPC_ERR_HEADER_UNINIT;
    }

    // 分配内存
    buf = (char *)malloc(msg_len);
    if (NULL == buf)
    {
        return SRPC_ERR_NO_MEMORY;
    }

    // 组装报文
    // 格式 "(head_len body_len head body)"
    char *pos = buf;
    *pos++ = PROTO_RPC_STX;
    *(int32_t *)pos = htonl(head_len);
    pos += sizeof(int32_t);
    *(int32_t *)pos = htonl(body_len);
    pos += sizeof(int32_t);

    // 序列化包头
    if (!head->SerializeToArray(pos, head_len))
    {
        free(buf);
        return SRPC_ERR_INVALID_PKG_HEAD;
    }
    pos += head_len;

    // 序列化包体
    if (body_len)
    {
        memcpy(pos, body.data(), body_len);
    }
    pos += body_len;
    *pos = PROTO_RPC_ETX;

    // 设置出参
    *pkg = buf;
    *len = msg_len;

    return SRPC_SUCCESS;
}


/**
 * @brief  SRPC报文打包函数[没有包体]
 * @info   [注意] 打包后的报文为出参，函数内部分配内存，调用者需要free
 * @param  pkg  [出参]打包完后报文的buffer
 *         len  [出参]打包完后，报文的长度
 *         head [入参]报文头,PB格式
 * @return =0   打包成功
 * @       <0   打包失败
 */
int32_t SrpcPackPkgNoBody(char** pkg, int32_t *len, const CRpcHead *head)
{
    char *buf;

    // 参数检查
    if (NULL == head || NULL == pkg || NULL == len)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 初始化报文长度
    int32_t head_len = head->ByteSize();
    int32_t msg_len  = PROTO_HEAD_MIN_LEN + head_len;

    // 检查包头是否初始化
    if (!head->IsInitialized() || !head_len)
    {
        return SRPC_ERR_HEADER_UNINIT;
    }

    // 分配内存
    buf = (char *)malloc(msg_len);
    if (NULL == buf)
    {
        return SRPC_ERR_NO_MEMORY;
    }

    // 组装报文
    // 格式 "(head_len body_len head body)"
    char *pos = buf;
    *pos++ = PROTO_RPC_STX;
    *(int32_t *)pos = htonl(head_len);
    pos += sizeof(int32_t);
    *(int32_t *)pos = 0;
    pos += sizeof(int32_t);

    // 序列化包头
    if (!head->SerializeToArray(pos, head_len))
    {
        free(buf);
        return SRPC_ERR_INVALID_PKG_HEAD;
    }
    pos += head_len;
    *pos = PROTO_RPC_ETX;

    // 设置出参
    *pkg = buf;
    *len = msg_len;

    return SRPC_SUCCESS;
}

/**
 * @brief  SRPC报文解包函数
 * @info   [注意] 必须为完整的一个包才能调用
 *         [注意] 只获取body的序列化后的二进制
 * @param  buff 报文buffer
 *         len  报文长度
 *         body 消息体
 * @return !=SRPC_SUCCESS   解包失败
 * @       =SRPC_SUCCESS    解包成功
 */
int32_t SrpcGetPkgBodyString(const char *buff, int32_t len, string &body)
{
    if (NULL == buff || len <= 0)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文合法性检查
    if ((len <= PROTO_HEAD_MIN_LEN)
        || (*buff != PROTO_RPC_STX)
        || (*(buff+len-1) != PROTO_RPC_ETX))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(buff + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(buff + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if (body_len < 0 || head_len <= 0 || msg_len != len)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    body.assign(buff + PROTO_HEAD_OFFSET + head_len, body_len);

    return SRPC_SUCCESS;
}

/**
 * @brief  SRPC报文解包函数
 * @info   [注意] 必须为完整的一个包才能调用
 *         [注意] 报文可能没有包体
 * @param  buff 报文buffer
 *         len  报文长度
 *         head 消息头
 *         body 消息体
 * @return <0   解包失败
 * @       =0   解包成功
 */
int32_t SrpcUnpackPkg(const char *buff, int32_t len, CRpcHead *head, Message *body)
{
    // 入参检查
    if (NULL == buff || len <= 0 || NULL == head || NULL == body)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文合法性检查
    if ((len <= PROTO_HEAD_MIN_LEN)
        || (*buff != PROTO_RPC_STX)
        || (*(buff+len-1) != PROTO_RPC_ETX))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(buff + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(buff + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if (body_len < 0 || head_len <= 0 || msg_len != len)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    // 解包头
    if (!head->ParseFromArray(buff + PROTO_HEAD_OFFSET, head_len))
    {
        return SRPC_ERR_INVALID_PKG_HEAD;
    }

    // 解包体
    if (body_len && !body->ParseFromArray(buff + PROTO_HEAD_OFFSET + head_len, body_len))
    {
        return SRPC_ERR_INVALID_PKG_BODY;
    }

    return SRPC_SUCCESS;
}


/**
 * @brief  SRPC报文解包函数(解析包头)
 * @info   [注意] 必须为完整的一个包才能调用
 * @param  buff 报文buffer
 *         len  报文长度
 *         head 消息头
 * @return <0   解包失败
 * @       =0   解包成功
 */
int32_t SrpcUnpackPkgHead(const char *buff, int32_t len, CRpcHead *head)
{
    // 入参检查
    if (NULL == buff || NULL == head || len <= 0)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文合法性检查
    if ((len <= PROTO_HEAD_MIN_LEN)
        || (*buff != PROTO_RPC_STX)
        || (*(buff+len-1) != PROTO_RPC_ETX))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(buff + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(buff + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if (body_len < 0 || head_len < 0 || msg_len != len)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    // 解析包头
    if (!head->ParseFromArray(buff + PROTO_HEAD_OFFSET, head_len))
    {
        return SRPC_ERR_INVALID_PKG_HEAD;
    }

    return SRPC_SUCCESS;
}


/**
 * @brief  SRPC报文解包函数(解析包体)
 * @info   [注意] 必须为完整的一个包才能调用
 *         [注意] 可能没有包体
 * @param  buff 报文buffer
 *         len  报文长度
 *         body 消息体
 * @return <0   解包失败
 * @       =0   解包成功
 */
int32_t SrpcUnpackPkgBody(const char *buff, int32_t len, Message *body)
{
    // 参数检查
    if (NULL == buff || NULL == body || len <= 0)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文合法性检查
    if ((len <= PROTO_HEAD_MIN_LEN)
        || (*buff != PROTO_RPC_STX)
        || (*(buff+len-1) != PROTO_RPC_ETX))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(buff + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(buff + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if (body_len < 0 || head_len < 0 || msg_len != len)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    // 解析包体
    if (body_len && !body->ParseFromArray(buff + PROTO_HEAD_OFFSET + head_len, body_len))
    {
        return SRPC_ERR_INVALID_PKG_BODY;
    }

    return 0;
}


/**
 * @brief  检查报文是否完整
 * @return <0  报文非法
 * @       =0  报文不完整
 * @       >0  报文有效长度
 */
int32_t SrpcCheckPkgLen(void *buff, int32_t len)
{
    char *pkg = (char *)buff;

    // 入参检查
    if (NULL == pkg || len <= 0)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文还没有接收完成
    if (len <= PROTO_HEAD_MIN_LEN)
    {
        return 0;
    }

    // 报文合法性检查
    if (*pkg != PROTO_RPC_STX)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(pkg + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(pkg + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if ((head_len < 0) || (body_len < 0) || (msg_len <= PROTO_HEAD_MIN_LEN))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    // 报文还未收完整
    if (len < msg_len)
    {
        return 0;
    }

    // 报文合法性检查
    if (*(pkg + msg_len - 1) != PROTO_RPC_ETX)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    return msg_len;
}


}


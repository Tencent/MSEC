
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
 * @filename srpc_proto.h
 */

#ifndef __SRPC_PROTO_H__
#define __SRPC_PROTO_H__

#include <stdint.h>
#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include "srpc.pb.h"
#include "srpc_comm.h"

using namespace google::protobuf;

namespace srpc {

/**
 * @brief 报文格式控制宏
 * @info  报文格式 "(head_len body_len head body)"
 */
#define PROTO_RPC_STX           0x28
#define PROTO_RPC_ETX           0x29
#define PROTO_HEAD_MIN_LEN      10
#define PROTO_HEAD_OFFSET       (PROTO_HEAD_MIN_LEN-1)
#define PROTO_BODY_LEN_OFFSET   (1+4)
#define PROTO_HEAD_LEN_OFFSET   (1)


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
int32_t SrpcPackPkg(char** pkg, int32_t *len, const CRpcHead *head, const Message *body);

/**
 * @brief  SRPC报文打包函数[没有包体]
 * @info   [注意] 打包后的报文为出参，函数内部分配内存，调用者需要free
 * @param  pkg  [出参]打包完后报文的buffer
 *         len  [出参]打包完后，报文的长度
 *         head [入参]报文头,PB格式
 * @return =0   打包成功
 * @       <0   打包失败
 */
int32_t SrpcPackPkgNoBody(char** pkg, int32_t *len, const CRpcHead *head);

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
int32_t SrpcPackPkg(char** pkg, int32_t *len, const CRpcHead *head, const string& body);


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
int32_t SrpcGetPkgBodyString(const char *buff, int32_t len, string &body);

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
int32_t SrpcUnpackPkg(const char *buff, int32_t len, CRpcHead *head, Message *body);

/**
 * @brief  SRPC报文解包函数(解析包头)
 * @info   [注意] 必须为完整的一个包才能调用
 * @param  buff 报文buffer
 *         len  报文长度
 *         head 消息头
 * @return <0   解包失败
 * @       =0   解包成功
 */
int32_t SrpcUnpackPkgHead(const char *buff, int32_t len, CRpcHead *head);

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
int32_t SrpcUnpackPkgBody(const char *buff, int32_t len, Message *body);

/**
 * @brief  检查报文是否完整
 * @return <0  报文格式错误
 * @       =0  报文不完整
 * @       >0  报文有效长度
 */
int32_t SrpcCheckPkgLen(void *buff, int32_t len);

}

#endif


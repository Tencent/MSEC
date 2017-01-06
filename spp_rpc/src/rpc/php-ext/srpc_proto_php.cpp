
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


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include "srpc_comm.h"
#include "srpc_proto.h"
#include "srpc.pb.h"
#include "srpc_proto_php.h"

using namespace google::protobuf;
using namespace srpc;

static CRpcHead *g_header;
string g_srpc_service_name = "default.php";

int srpc_php_set_header(void *header)
{
    g_header = (CRpcHead *)header;
    return 0;
}

void srpc_php_set_service_name(const char *sname, int len)
{
    if (NULL == sname || len < 3)
    {
        return;
    }

    g_srpc_service_name.assign(sname, len);
}

int srpc_php_header_init(CRpcHead &head, const char *method_name, uint64_t seq)
{
    if (NULL == method_name || method_name[0] == '\0')
    {
        return SRPC_ERR_PARA_ERROR;
    }

    uint64_t color_id = gen_colorid(method_name);

    head.set_sequence(seq);
    head.set_color_id(color_id);
    head.set_caller(g_srpc_service_name);
    head.set_method_name(method_name);

    if (NULL == g_header)
    {
        head.set_coloring(0);
        head.set_flow_id(color_id ^ seq);
    }
    else
    {
        head.set_coloring(g_header->coloring());
        if (!g_header->has_flow_id() || !g_header->flow_id()) { // 如果客户端没有设置flow id，就默认使用当前的seq作为flow id
            head.set_flow_id(seq);
        }
        else
        {
            head.set_flow_id(g_header->flow_id());
        }
    }

    return SRPC_SUCCESS;
}

int srpc_php_pack(const char *method_name, uint64_t seq, const void *body, int body_len, char *buf, int32_t *plen)
{
    int32_t ret;
    CRpcHead head;

    if (NULL == method_name || NULL == buf || NULL == plen || *plen <= PROTO_HEAD_MIN_LEN)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    ret = srpc_php_header_init(head, method_name, seq);
    if (ret < 0)
    {
        return ret;
    }

    if (NULL == body)
    {
        body_len = 0;
    }

    int32_t head_len = head.ByteSize();
    int32_t msg_len  = PROTO_HEAD_MIN_LEN + head_len + body_len;

    if (*plen < msg_len)
    {
        return SRPC_ERR_NO_MEMORY;
    }

    char *pos = buf;
    *pos++ = PROTO_RPC_STX;
    *(int32_t *)pos = htonl(head_len);
    pos += sizeof(int32_t);
    *(int32_t *)pos = htonl(body_len);
    pos += sizeof(int32_t);

    // 序列化包头
    if (!head.SerializeToArray(pos, head_len))
    {
        return SRPC_ERR_INVALID_PKG_HEAD;
    }
    pos += head_len;

    // 序列化包体
    if (body_len)
    {
        memcpy(pos, body, body_len);
    }

    pos    += body_len;
    *pos    = PROTO_RPC_ETX;
    *plen   = msg_len;

    return SRPC_SUCCESS;
}

int srpc_php_unpack(const char *buf, int buf_len, char *body, int *plen, uint64_t *seq, int32_t *err)
{
    CRpcHead head;

    // 入参检查
    if (NULL == buf || buf_len <= 0 || NULL == body || NULL == plen
        || *plen <= PROTO_HEAD_MIN_LEN || NULL == seq || NULL == err)
    {
        return SRPC_ERR_PARA_ERROR;
    }

    // 报文合法性检查
    if ((buf_len <= PROTO_HEAD_MIN_LEN)
        || (*buf != PROTO_RPC_STX)
        || (*(buf+buf_len-1) != PROTO_RPC_ETX))
    {
        return SRPC_ERR_INVALID_PKG;
    }

    int32_t head_len = htonl(*(int *)(buf + PROTO_HEAD_LEN_OFFSET));
    int32_t body_len = htonl(*(int *)(buf + PROTO_BODY_LEN_OFFSET));
    int32_t msg_len  = head_len + body_len + PROTO_HEAD_MIN_LEN;

    // 报文合法性检查
    if (body_len < 0 || head_len <= 0 || msg_len != buf_len)
    {
        return SRPC_ERR_INVALID_PKG;
    }

    // 解包头
    if (!head.ParseFromArray(buf + PROTO_HEAD_OFFSET, head_len))
    {
        return SRPC_ERR_INVALID_PKG_HEAD;
    }

    // 拷贝包体
    if (body_len)
    {
        memcpy(body, buf + PROTO_HEAD_OFFSET + head_len, body_len);
    }

    *plen = body_len;
    *seq  = (uint64_t)head.sequence();
    *err  = (int32_t)head.err();

    return SRPC_SUCCESS;
}

/**
 * @brief  检查报文是否完整
 * @return <0  报文非法
 * @       =0  报文不完整
 * @       >0  报文有效长度
 */
int srpc_php_check_pkg_len(void *buff, int len)
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


const char *srpc_php_err_msg(int err)
{
    return errmsg(err);
}


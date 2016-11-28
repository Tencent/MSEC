
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
 * @filename routeproto.c
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include "nlbapi.h"

/**
 * @brief 打包路由请求包
 * @info  格式  "len name"
 *        len不包含自己的长度
 */
int32_t serialize_route_request(const char *service_name, char *buff, int32_t len)
{
    int32_t rlen;

    rlen = snprintf(buff+4, len-4, "%s", service_name);
    if (rlen >= len) {
        return -1;
    }

    *(uint32_t *)buff = htonl((uint32_t)rlen);
    return rlen+4;
}

/**
 * @brief 解路由请求包
 * @info  格式  "len name"
 *        len不包含自己的长度
 */
int32_t deserialize_route_request(const char *buff, int32_t blen, char *service_name, int32_t slen)
{
    int32_t rlen;

    if (blen <= 4) {
        return -1;
    }

    rlen = (int32_t)ntohl(*(uint32_t *)buff);
    if (rlen >= slen || rlen < 0) {
        return -2;
    }

    memcpy(service_name, buff+4, rlen);
    service_name[rlen] = '\0';

    return 0;
}

/**
 * @brief 打包路由回复包
 * @info  格式  "len result ip port type"
 *        len不包含自己的长度
 *        result非零，后面的ip port type就没有
 */
int32_t serialize_route_response(int32_t result, const struct routeid *id, char *buff, int32_t len)
{
    if (NULL == buff || len < 20) {
        return -1;
    }

    *(uint32_t *)(buff + 4) = htonl((uint32_t)result);

    if (result) {
        *(uint32_t *)buff = htonl(4);
        return 8;
    }

    *(uint32_t *)(buff + 8)  = htonl(id->ip);
    *(uint16_t *)(buff + 12) = htons(id->port);
    *(uint16_t *)(buff + 14) = htons((uint16_t)id->type);
    *(uint32_t *)buff        = htonl(12);

    return 16;
}

/**
 * @brief 解路由回复包
 * @info  格式  "len result ip port type"
 *        len不包含自己的长度
 *        result非零，后面的ip port type就没有
 */
int32_t deserialize_route_response(const char *buff, int32_t blen, int32_t *result, struct routeid *id)
{
    int32_t rlen;

    if (blen < 8) {
        return -1;
    }

    rlen    = (int32_t)ntohl(*(uint32_t *)buff);
    *result = (int32_t)ntohl(*(uint32_t *)(buff + 4));

    if (*result) {
        if (rlen != 4) {
            return -2;
        }

        id->ip = 0;
        return 0;
    }

    if (rlen != 12) {
        return -3;
    }

    id->ip   = ntohl(*(uint32_t *)(buff+8));
    id->port = ntohs(*(uint16_t *)(buff + 12));
    id->type = (NLB_PORT_TYPE)ntohs(*(uint16_t *)(buff + 14));

    return 0;
}




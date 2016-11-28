
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
 * @filename routeproto.h
 */

#ifndef _ROUTEPROTO_H_
#define _ROUTEPROTO_H_

#include <stdint.h>
#include "nlbapi.h"

/**
 * @brief 打包路由请求包
 * @info  格式  "len name"
 *        len不包含自己的长度
 */
int32_t serialize_route_request(const char *service_name, char *buff, int32_t len);

/**
 * @brief 解路由请求包
 * @info  格式  "len name"
 *        len不包含自己的长度
 */
int32_t deserialize_route_request(const char *buff, int32_t blen, char *service_name, int32_t slen);

/**
 * @brief 打包路由回复包
 * @info  格式  "len result ip port type"
 *        len不包含自己的长度
 *        result非零，后面的ip port type就没有
 */
int32_t serialize_route_response(int32_t result, const struct routeid *id, char *buff, int32_t len);


/**
 * @brief 解路由回复包
 * @info  格式  "len result ip port type"
 *        len不包含自己的长度
 *        result非零，后面的ip port type就没有
 */
int32_t deserialize_route_response(const char *buff, int32_t blen, int32_t *result, struct routeid *id);

#endif



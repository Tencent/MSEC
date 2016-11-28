
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jansson.h"
#include "commstruct.h"
#include "comm.h"
#include "policy.h"

int32_t json_parse_server(json_t *json, struct server_info *server)
{
    size_t index;
    const char *type;
    json_t *val, *aval;

    /* 权重信息 */
    val = json_object_get(json, "w");
    if (NULL == val || !json_is_integer(val)) {
        return -101; 
    } else {
        server->weight_static = (uint16_t)json_integer_value(val);
    }

    /* 端口类型 */
    val = json_object_get(json, "t");
    if (NULL == val || !json_is_string(val)) {
        return -102; 
    }

    type = json_string_value(val);
    if (!strcmp(type, "udp")) {
        server->port_type = 1;
    } else if (!strcmp(type, "tcp")) {
        server->port_type = 2;
    } else if (!strcmp(type, "all")) {
        server->port_type = 3;
    } else {
        return -103;
    }

    /* IP地址 */
    val = json_object_get(json, "IP");
    if (NULL == val || !json_is_string(val)) {
        return -104;
    }

    inet_aton(json_string_value(val), (struct in_addr *)&server->server_ip);

    /* 端口列表 */
    aval = json_object_get(json, "ports");
    if (NULL == val || !json_is_array(aval)) {
        return -105;
    }

    if (json_array_size(aval) > NLB_PORT_MAX) {
        return -106;
    }

    json_array_foreach(aval, index, val)
    {
        if (!json_is_integer(val)) {
            return -107;
        }

        server->port[index] = (uint16_t)json_integer_value(val);
    }

    server->port_num = json_array_size(aval);

    return 0;
}

int32_t json_parse_service(const char *json_buf, int32_t buf_len, struct shm_servers **svrs)
{
    int32_t result;
    int32_t len;
    int32_t policy  = NLB_POLICY_STANDARD;
    size_t  index;
    json_t *json = NULL;
    json_t *val, *aval;
    json_error_t error;
    const char *buff = json_buf;
    struct shm_servers *shm_svrs = NULL;

    /* 确认json内容的长度 */
    if (buff[buf_len - 1] == '\0') {
        len = strlen(buff);
    } else {
        len = buf_len;
    }

    /* 加载json字符串到json对象 */
    json = json_loadb(buff, len, 0, &error);
    if (!json) {
        result = -2;
        goto ERR_RET;
    }

    /* 获取策略 */
    val = json_object_get(json, "Policy");
    if (val) { 
        if (!json_is_string(val)) {
            result = -3;
            goto ERR_RET;
        }

        policy = str2policy(json_string_value(val));
    }

    /* 获取IPInfo json对象 */
    aval = json_object_get(json, "IPInfo");
    if (NULL == aval || !json_is_array(aval)) {
        result = -4;
        goto ERR_RET;
    }

    if (json_array_size(aval) == 0 || json_array_size(aval) >= NLB_SERVER_MAX) {
        result = -5;
        goto ERR_RET;
    }

    shm_svrs = calloc(1, sizeof(struct shm_servers) + json_array_size(aval)*sizeof(struct server_info));
    if (NULL == shm_svrs) {
        result = -6;
        goto ERR_RET;
    }

    /* 循环获取所有IP信息 */
    json_array_foreach(aval, index, val)
    {
        if (json_typeof(val) != JSON_OBJECT) {
            result = -7;
            goto ERR_RET;
        }
    
        result = json_parse_server(val, &shm_svrs->svrs[index]);
        if (result) {
            goto ERR_RET;
        }
    }

    shm_svrs->server_num    = json_array_size(aval);
    shm_svrs->policy        = policy;
    *svrs = shm_svrs;
    json_decref(json);

    return 0;

ERR_RET:

    if (json)
        json_decref(json);

    if (shm_svrs)
        free(shm_svrs);

    return result;
}

#if 0
int main()
{
    int32_t ret;
    struct shm_servers *servers;
    char json_str[] = "{\"IPInfo\":[{\"IP\": \"1.1.1.1\", \"ports\": [1,2,3], \"t\":\"all\", \"w\":100}, {\"IP\":\"2.2.2.2\",\"ports\":[11],\"t\":\"all\", \"w\":200}]}";
    ret = json_parse_service(json_str, sizeof(json_str), &servers);

    printf("%d\n", ret);

    return 0;
}
#endif

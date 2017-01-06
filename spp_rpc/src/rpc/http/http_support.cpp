
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


#include <string.h>
#include <stdio.h>
#include <iostream>
#include "http_support.h"
#include "srpc_comm.h"
#include "json2pb.h"

using namespace srpc;
using namespace std;

void CHttpHelper::GenJsonResponse(int err, const char *data, int len, std::string &response)
{
    int body_len;
    char body_head[256];
    char content_length[100];
    const char *head_prefix = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: application/json; charset=UTF-8\r\n"
                              "Content-Encoding: UTF-8\r\n";

    if (NULL == data)
        len = 0;
    body_len = len;

    if (data)
        body_len += snprintf(body_head, sizeof(body_head), "{\"ret\":%d, \"errmsg\":\"%s\", \"resultObj\": ", err, ((err == SRPC_SUCCESS) ? "" : errmsg(err)));
    else
        body_len += snprintf(body_head, sizeof(body_head), "{\"ret\":%d, \"errmsg\":\"%s\" ", err, ((err == SRPC_SUCCESS) ? "" : errmsg(err)));
    body_len += 1;
    snprintf(content_length, sizeof(content_length), "Content-Length: %d\r\n\r\n", body_len);

    response.assign(head_prefix);
    response.append(content_length);
    response.append(body_head);
    if (data)
        response.append(data, len);    
    response.append("}");
}

bool CHttpHelper::IsSupportHttpRequest(const char *data, int len)
{
    if (!strncmp(data, "POST", 4))
    {
        return true;
    }

    return false;
}

void CHttpHelper::Json2Pb(google::protobuf::Message &msg, const char *buf, int size)
{
    json2pb(msg, buf, size);
}

std::string CHttpHelper::Pb2Json(const google::protobuf::Message &msg)
{
    return pb2json(msg);
}

std::string &CHttpHelper::GetMethodName(void)
{
    return m_method_name;
}

std::string &CHttpHelper::GetBody(void)
{
    return m_body;
}

CHttpHelper::CHttpHelper(HTTP_HELPER_T type)
{
    m_type = type;
}

CHttpHelper::~CHttpHelper()
{

}

int CHttpHelper::http_url_cb(http_parser *parser, const char *at, size_t len)
{
    std::string url;

    if (NULL == at || len <= 0)
    {
        return -1;
    }

    url.assign(at, len);

    std::size_t found = url.find("methodName=");
    if (found == std::string::npos)
    {
        // TODO: LOG
        return -2;
    }

    std::size_t begin = found + strlen("methodName=");
    std::string tmp   = url.substr(begin);
    std::size_t end   = tmp.find("&");
    std::string mtd   = tmp.substr(0, end);

    CHttpHelper *helper = (CHttpHelper *)parser->data;
    helper->m_method_name = mtd;

    return 0;
}

int CHttpHelper::http_body_cb(http_parser *parser, const char *at, size_t len)
{
    if (NULL == at || len <= 0)
    {
        return -1;
    }

    CHttpHelper *helper = (CHttpHelper *)parser->data;
    helper->m_body.assign(at, len);

    return 0;
}

int CHttpHelper::http_header_field_cb(http_parser *parser, const char *at, size_t len)
{
    if (NULL == at || len <= 0)
    {
        return -1;
    }

    if (!strncmp(at, "Content-Type", 12))
    {
        CHttpHelper *helper = (CHttpHelper *)parser->data;
        helper->m_check_content_type = true;
        return 0;
    }

    if (!strncmp(at, "caller", 6))
    {
        CHttpHelper *helper = (CHttpHelper *)parser->data;
        helper->m_is_caller_head = true;
        return 0;
    }

    return 0;
}

int CHttpHelper::http_header_value_cb(http_parser *parser, const char *at, size_t len)
{
    if (NULL == at || len <= 0)
    {
        return -1;
    }

    CHttpHelper *helper = (CHttpHelper *)parser->data;
    if (helper->m_check_content_type)
    {
        helper->m_check_content_type = false;
        
        if (!strncmp(at, "application/json", 16))
        {
            // TODO: LOG
            return -2;
        }

        return 0;
    }

    if (helper->m_is_caller_head)
    {
        helper->m_is_caller_head = false;
        helper->m_caller.assign(at, len);
        return 0;
    }

    return 0;
}


int CHttpHelper::http_message_complete_cb(http_parser *parser)
{
    CHttpHelper *helper = (CHttpHelper *)parser->data;
    http_parser_pause(parser, 1);
    helper->m_complete = true;
    return 0;
}

std::string &CHttpHelper::GetCaller(void)
{
    return m_caller;
}

int CHttpHelper::Parse(const char *data, int len)
{
    int ret;

    if (NULL == data || len <= 0)
    {
        return -1;
    }

    m_parser.data = this;
    http_parser_init(&m_parser, HTTP_REQUEST);
    http_parser_settings_init(&m_settings);

    m_settings.on_header_field = http_header_field_cb;
    m_settings.on_header_value = http_header_value_cb;
    m_settings.on_message_complete = http_message_complete_cb;
    m_check_content_type = false;
    m_complete           = false;
    m_is_caller_head     = false;
    m_caller             = "http.json";

    if (m_type == HTTP_HELPER_TYPE_WORKER)
    {
        m_settings.on_url = http_url_cb;
        m_settings.on_body = http_body_cb;
    }

    ret = http_parser_execute(&m_parser, &m_settings, data, (size_t)len);

    if (ret <=0 || (m_parser.http_errno != HPE_OK && m_parser.http_errno != HPE_PAUSED))
    {
        return -2;
    }

    if (m_complete)
    {
        return ret;
    }

    return 0;
}



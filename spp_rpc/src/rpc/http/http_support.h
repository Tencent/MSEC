
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


#ifndef __HTTP_SUPPORT_H__
#define __HTTP_SUPPORT_H__

#include <string>
#include <sys/types.h>
#include <google/protobuf/message.h>
#include "http_parser.h"

typedef enum {
    HTTP_HELPER_TYPE_PROXY  = 1,
    HTTP_HELPER_TYPE_WORKER = 2,
} HTTP_HELPER_T;

class CHttpHelper
{
public:
    CHttpHelper(HTTP_HELPER_T type = HTTP_HELPER_TYPE_PROXY);
    ~CHttpHelper();

    std::string &GetMethodName(void);
    std::string &GetBody(void);
    std::string &GetCaller(void);
    int Parse(const char *data, int len);
    static void *Json2SrpcPkg(const char *method_name, const char *json_data, int len);
    static void *SrpcPkg2Json(const char *srpc_data, int len);
    static bool IsSupportHttpRequest(const char *data, int len);
    static void GenJsonResponse(int err, const char *data, int len, std::string &response);
    static void Json2Pb(google::protobuf::Message &msg, const char *buf, int size);
    static std::string Pb2Json(const google::protobuf::Message &msg);

private:
    static int http_url_cb(http_parser *parser, const char *at, size_t len);
    static int http_body_cb(http_parser *parser, const char *at, size_t len);
    static int http_header_field_cb(http_parser *parser, const char *at, size_t len);
    static int http_header_value_cb(http_parser *parser, const char *at, size_t len);
    static int http_message_complete_cb(http_parser *parser);

private:
    bool            m_complete;
    HTTP_HELPER_T   m_type;
    http_parser     m_parser;
    http_parser_settings m_settings;

    std::string     m_method_name;
    std::string     m_body;
    std::string     m_caller;
    bool            m_check_content_type;
    bool            m_is_caller_head;
};

#endif


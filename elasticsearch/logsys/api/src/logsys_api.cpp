
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


#include "logsys_api.h"

#include <iostream>
#include <sys/types.h>         
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdarg.h>

#include "inifile.h"
#include "flume_event.pb.h"

using namespace std;
using namespace flume;
using namespace inifile;

namespace msec 
{

#define GROUP_LOG  "LOG"
#define GROUP_COLOR  "COLOR"
#define MAXDATASIZE  (1<<13)

LogsysApi*  LogsysApi::s_logsys_api_ptr = NULL;
const char* LogsysApi::s_level_list[] = {
        "TRACE", "DEBUG", "INFO", "ERROR", "FATAL", "NONE"
    };
char g_send_buf[MAXDATASIZE];

LogsysApi::~LogsysApi()
{
    if (m_socket != -1)
    {
        close(m_socket);
        m_socket = -1;
    }
}

int LogsysApi::Init(const char* conf_file)
{
    int i = 0;

    IniFile  inifile;
    int ret = inifile.load(conf_file);
    if (ret != RET_OK)
    {
        std::cerr << "Read config failed: " << conf_file << endl;
        return -1;
    }

    //读取日志级别配置
    std::string log_level = inifile.getStringValue(GROUP_LOG, "Level", ret);
    for (i = 0; i < (int)(sizeof(s_level_list) / sizeof(s_level_list[0])); ++i)
    {
        if (0 == strcasecmp(s_level_list[i], log_level.c_str()))
        {
            m_log_level = i;
            break;
        }
    }
    //均不匹配，设置默认的日志级别
    if (i >= (int)(sizeof(s_level_list) / sizeof(s_level_list[0])))
        m_log_level = ERROR;

    //读取染色配置
    m_color_conditions.clear();
    char color_conf_name[32];
    std::string color_field_name;
    std::string color_field_value;
    for (i = 0; i < 1000; ++i)
    {
        color_field_name.clear();
        color_field_value.clear();
        snprintf(color_conf_name, sizeof(color_conf_name), "FieldName%d", i);
        color_field_name = inifile.getStringValue(GROUP_COLOR, color_conf_name, ret);

        if (color_field_name.empty())
            break;

        snprintf(color_conf_name, sizeof(color_conf_name), "FieldValue%d", i);
        color_field_value = inifile.getStringValue(GROUP_COLOR, color_conf_name, ret);

        if (!color_field_name.empty() && !color_field_value.empty())
        {
            m_color_conditions.insert(make_pair(std::string(color_field_name), std::string(color_field_value)));
        }
    }

    //Coloring=1
    m_color_conditions.insert(make_pair(std::string("Coloring"), std::string("1")));
    return 0;
}

bool LogsysApi::IsLogWritable(int level, const std::map<std::string, std::string>&  headers)
{
    bool ignore = true;
    if (level >= m_log_level)
    {
        ignore = false;
    }
    else
    {
        //检查染色条件
        std::map<std::string, std::string>::const_iterator mi;
        std::pair<std::string, std::string>  entry;
        for (mi = headers.begin(); mi != headers.end(); ++mi)
        {
            entry.first = mi->first;
            entry.second = mi->second;
            if (m_color_conditions.find(entry) != m_color_conditions.end())
            {
                ignore = false;
                break;
            }
        }
    }

    return !ignore;
}

bool LogsysApi::IsLogWritable(int level)
{
    return IsLogWritable(level, m_inner_headers);
}

int LogsysApi::Log(int level, const std::map<std::string, std::string>&  headers,
    const std::string& body)
{
    bool ignore = true;
    if (level >= m_log_level)
    {
        ignore = false;
    }
    else
    {
        //检查染色条件
        std::map<std::string, std::string>::const_iterator mi;
        std::pair<std::string, std::string>  entry;
        for (mi = headers.begin(); mi != headers.end(); ++mi)
        {
            entry.first = mi->first;
            entry.second = mi->second;
            if (m_color_conditions.find(entry) != m_color_conditions.end())
            {
                ignore = false;
                break;
            }
        }
    }

    if (ignore) return 0;

    int ret = 0;
    if (m_socket < 0)
    {
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(44443);
        inet_aton("127.0.0.1", &server.sin_addr);

        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket < 0)
            return ERROR_CREATE_SOCKET;

        ret = connect(m_socket, (struct sockaddr *)&server, sizeof(struct sockaddr));
        if (ret != 0)
        {
            close(m_socket);
            m_socket = -1;
            return ERROR_CONNECT_SERVER;
        }
    }

    //compose the packet
    string data;
    flume::FlumeEvent event;
    flume::FlumeEventHeader* header;

    std::map<std::string, std::string>::const_iterator mi;
    for (mi = headers.begin(); mi != headers.end(); ++mi)
    {
        header = event.add_headers();
        header->set_key(mi->first);
        header->set_value(mi->second);
    }

    //Add Level header
    int level_max = sizeof(s_level_list) / sizeof(s_level_list[0]) - 1;
    header = event.add_headers();
    header->set_key("Level");
    if (level >= 0 && level <= level_max)
    {
        header->set_value(s_level_list[level]);
    }
    else
    {
        header->set_value(s_level_list[level_max]);
    }
    if (body.length() >= sizeof(g_send_buf) - 1024)
    {
        event.set_body(body.substr(0, sizeof(g_send_buf) - 1024));
    }
    else
    {
        event.set_body(body);
    }

    int length = event.ByteSize();
    memcpy(g_send_buf, &length, sizeof(length));
    if (!event.SerializeToArray((void*)(g_send_buf + 4), sizeof(g_send_buf) - 4))
    {
        return ERROR_COMPOSE_PACKET;
    }

    ret = send(m_socket, g_send_buf, length + 4, 0);
    if (ret != length + 4)
    {
        close(m_socket);
        m_socket = -1;
        return ERROR_SEND;
    }

    return 0;
}

int LogsysApi::Log(int level, const std::string& body)
{
    //清空inner headers
    int ret = Log(level, m_inner_headers, body);
    m_inner_headers.clear();
    return ret;
}

LogsysApi& LogsysApi::LogSetHeader(const std::string& key, const std::string& value)
{
    m_inner_headers[key] = value;
    return *this;
}

bool LogIsWritable(int level)
{
    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    return  log_api->IsLogWritable(level);
}

bool LogIsWritable(int level, const std::map<std::string, std::string>&  headers)
{
    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    return  log_api->IsLogWritable(level, headers);
}

//Use inner headers
void LOG(int level, const char* format, ...)
{
    char buff[1024 * 8];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buff, sizeof(buff), format, ap);
    va_end(ap);

    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    log_api->Log(level, buff);
}

//Use outter headers
void LOG(int level, const std::map<std::string, std::string>&  headers, const char* format, ...)
{
    char buff[1024 * 8];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buff, sizeof(buff), format, ap);
    va_end(ap);

    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    log_api->Log(level, headers, buff);
}

void LOG(int level, const std::map<std::string, std::string>&  headers, const char* format, va_list ap)
{
    char buff[1024 * 8];
    vsnprintf(buff, sizeof(buff), format, ap);

    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    log_api->Log(level, headers, buff);
}

LogsysApi& LogSetHeader(const std::string& key, const std::string& value)
{
    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    return log_api->LogSetHeader(key, value);
}

}


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


#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include "log.h"
#include "commtype.h"

enum {
    SERVER_MODE = 1,
    CLIENT_MODE = 2,
    MIX_MODE    = 3,
};

struct config {
    bool     quit;           /* 退出程序标记 */
    uint16_t port;           /* CLIENT_MODE监听端口 */
    uint32_t local_ip;       /* 本机IP地址 */
    int32_t  mode;           /* agent工作模式 */
    int32_t  timeout;        /* zookeeper超时时间 */
    int32_t  log_level;      /* 日志级别 */
    char *   host;           /* zookeeper服务器列表 */
    char *   plugin;         /* agent插件，获取进程信息 */
};

extern struct config g_agent_config;

/* 获取agent工作模式 */
static inline int32_t get_worker_mode(void) {
    return g_agent_config.mode;
}

/* 获取zookeeper超时时间 */
static inline int32_t get_zk_timeout(void) {
    return g_agent_config.timeout;
}

/* 获取zookeeper服务器列表 */
static inline const char *get_zk_host(void) {
    return g_agent_config.host;
}

/* 获取业务插件路径 */
static inline const char *get_nlb_plugin(void) {
    return g_agent_config.plugin;
}

/* 获取监听端口 */
static inline uint16_t get_listen_port(void) {
    return g_agent_config.port;
}

/* 获取本机IP地址 */
static inline uint32_t get_local_ip(void) {
    return g_agent_config.local_ip;
}

/* 获取日志级别 */
static inline int32_t get_log_level(void) {
    return g_agent_config.log_level;
}

/* 设置退出标记 */
static inline void set_quit(void) {
    g_agent_config.quit = true;
}

/* 检查是否退出程序 */
static inline bool quit(void) {
    return g_agent_config.quit;
}

/**
 * @brief 处理命令行参数
 */
void parse_args(int32_t argc, char **argv);

#endif


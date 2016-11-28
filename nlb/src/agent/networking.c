
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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include "log.h"
#include "config.h"
#include "nlbapi.h"
#include "networking.h"
#include "zkplugin.h"
#include "utils.h"
#include "nlbtime.h"
#include "routeprocess.h"

/* 网络管理数据结构 */
struct netmng {
    uint64_t timeout;
    int32_t  zk_revents;
    int32_t  listen_fd;
    int32_t  listen_revents;
};

static struct netmng net_mng = {
    .timeout        = 10,        /* 默认10毫秒超时 */
    .zk_revents     = 0,         /* ZK fd事件 */
    .listen_fd      = -1,        /* agent监听fd */
    .listen_revents = 0,         /* 监听fd事件 */
};

/* 获取agent监听套接字 */
int32_t get_listen_fd(void)
{
    return net_mng.listen_fd;
}

/**
 * @brief  网络初始化
 * @return =0 成功 <0 失败
 */
int32_t network_init(void)
{
    int32_t  ret;
    int32_t  fd;

    /* 初始化zookeeper */
    ret = nlb_zk_init(get_zk_host(), get_zk_timeout());
    if (ret < 0) {
        NLOG_ERROR("Nlb zookeeper init failed, ret [%d]", ret);
        return -1;
    }

    /* 服务器模式不需要bind UDP端口 */
    if (get_worker_mode() == SERVER_MODE) {
        net_mng.listen_fd = -1;
        return 0;
    }

    /* 创建UDP套接字，用于接收路由请求 */
    fd = create_udp_socket();
    if (fd < 0) {
        NLOG_ERROR("Create udp socket failed, ret [%d]", ret);
        return -2;
    }

    ret = bind_port(fd, "127.0.0.1", get_listen_port());
    if (ret < 0) {
        NLOG_ERROR("Bind port failed, ret [%d]", ret);
        return -3;
    }

    net_mng.listen_fd = fd;

    return 0;
}

/**
 * @brief  监听网络事件
 * @info   监听zookeeper和路由请求
 */
int32_t network_poll(void)
{
    int32_t  ret;
    int32_t  maxfd, zkfd, listen_fd = net_mng.listen_fd;
    int32_t  zk_events, listen_events;
    uint64_t zk_timeout, timeout;
    fd_set   rfds, wfds;
    struct timeval tv;

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);

    /* 获取zookeeper关注事件 */
    ret = nlb_zk_poll_events(&zkfd, &zk_events, &zk_timeout);
    if (ret < 0) {
        NLOG_ERROR("Nlb zookeeper poll events failed, ret [%d]", ret);
        return -1;
    }

    if (zkfd != -1) {
        if (zk_events & NLB_POLLIN) {
            FD_SET(zkfd, &rfds);
        }

        if (zk_events & NLB_POLLOUT) {
            FD_SET(zkfd, &wfds);
        }
    }

    if (listen_fd != -1) {
        FD_SET(listen_fd, &rfds);
    }

    listen_events = 0;
    zk_events = 0;
    maxfd     = max(listen_fd, zkfd) + 1;
    timeout   = min(zk_timeout, net_mng.timeout);
    covert_ms_2_tv(timeout, &tv);

    /* 监听事件 */
    if (select(maxfd, &rfds, &wfds, NULL, &tv) > 0) {
        if (zkfd != -1) {
            if (FD_ISSET(zkfd, &rfds)) {
                zk_events |= NLB_POLLIN;
            }

            if (FD_ISSET(zkfd, &wfds)) {
                zk_events |= NLB_POLLOUT;
            }
        }

        if ((listen_fd != -1) && FD_ISSET(listen_fd, &rfds)) {
            listen_events |= NLB_POLLIN;
        }
    }

    /* 设置获取事件 */
    net_mng.listen_revents = listen_events;
    net_mng.zk_revents     = zk_events;

    return 0;
}

/**
 * @brief 网络事件处理主函数
 */
int32_t network_process(void)
{
    int32_t zk_revents = net_mng.zk_revents;
    int32_t l_revents  = net_mng.listen_revents;
    int32_t listen_fd  = net_mng.listen_fd;

    nlb_zk_process(zk_revents);

    if ((listen_fd != -1) && (l_revents & NLB_POLLIN))
        process_route_request(listen_fd);

    return 0;
}


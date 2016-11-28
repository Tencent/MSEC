
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
 * @filename utils.c
 */
 
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include "nlbtime.h"

/**
 * @brief 将所有数据写入fd
 */
int32_t write_all(int32_t fd, const char *buff, int32_t len)
{
    int32_t write_len = 0;
    int32_t ret;

    if (!buff || !len || fd < 0) {
        return -1;
    }

    do {
        ret = write(fd, buff + write_len, len - write_len);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -2;
        }
        write_len += ret;
    } while (write_len != len);

    return 0;
}

/**
 * @brief 将所有数据写入文件
 */
int32_t write_all_2_file(const char *path, const char *data, int32_t len)
{
    int32_t fd, ret;

    if (!path || !data || !len) {
        return -1;
    }

    fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        return -2;
    }

    ret = write_all(fd, data, len);
    if (ret < 0) {
        close(fd);
        return -3;
    }

    close(fd);

    return 0;
}

/**
 * @brief  通过网络接口名，获取接口IP地址
 * @info   默认采用eth0接口地址
 * @return =0   没有该接口或者接口ip地址为0.0.0.0
 *         其它 获取成功
 */
uint32_t get_ip(const char* intf)
{
    int32_t  fd, intrface;
    uint32_t ip = 0;
    char ifname[IFNAMSIZ];
    struct ifreq  ifr[128];
    struct ifconf ifc;

    /* 默认使用eth0 */
    if (NULL == intf) {
        strncpy(ifname, "eth0", sizeof(ifname) - 1);
    } else {
        strncpy(ifname, intf, sizeof(ifname) - 1);
    }

    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return 0;
    }

    ifc.ifc_len = sizeof(ifr);
    ifc.ifc_buf = (caddr_t)ifr;

    /* 获取所有网络接口名 */
    if (ioctl(fd, SIOCGIFCONF, (char*)&ifc) == -1) {
        close(fd);
        return 0;
    }

    /* 查找网络接口，并获取接口地址 */
    intrface = ifc.ifc_len / sizeof(struct ifreq);
    while (intrface-- > 0) {
        if (strcmp(ifr[intrface].ifr_name, ifname) == 0) {
            if (!(ioctl(fd, SIOCGIFADDR, (char *)&ifr[intrface])))
                ip = (uint32_t)((struct sockaddr_in *)(&ifr[intrface].ifr_addr))->sin_addr.s_addr;
            break;
        }
    }
    close(fd);

    return ip;
}

/**
 * @brief  创建UDP套接字,并设置为非阻塞
 * @return >=0 返回套接字描述符 <0 错误
 */
int32_t create_udp_socket(void)
{
    int32_t flags, ret;
    int32_t fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        return -1;
    }

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return -2;
    }

    ret = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    if (ret == -1) {
        return -3;
    }

    return fd;
}

/**
 * @brief IP+port转换成Linux网络地址
 */
void make_inet_addr(const char *ip, uint16_t port, struct sockaddr_in *addr)
{
    memset(addr, 0, sizeof(*addr));
    addr->sin_family        = AF_INET;
    addr->sin_addr.s_addr   = inet_addr(ip);
    addr->sin_port          = htons(port);
}

/**
 * @brief  网络套接字绑定端口
 * @return <0 失败 =0 成功
 */
int32_t bind_port(int32_t fd, const char *ip, uint16_t port)
{
    int32_t ret;
    struct sockaddr_in addr;

    if (fd < 0) {
        return -1;
    }

    make_inet_addr(ip, port, &addr);

    ret = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr));
    if (ret == -1) {
        return -2;
    }

    return 0;
}

/**
 * @brief  发送UDP包
 * @return =0 成功 <0 失败
 */
int32_t udp_send(int32_t fd, struct sockaddr_in *addr, const char *buff, int32_t len)
{
    int32_t ret;

    if ((fd < 0) || (NULL == buff) || (len <= 0) || (NULL == addr)) {
        return -1;
    }

    ret = sendto(fd, buff, len, 0, (struct sockaddr *)addr, sizeof(*addr));
    if (ret == -1) {
        return -2;
    }

    return 0;
}

/**
 * @brief  接收UDP回包
 * @return >0 接收长度 <0 失败
 */
int32_t udp_recv(int32_t fd, char *buff, int32_t len, int32_t timeout)
{
    int32_t ret = 0;
    struct pollfd pollfd;

    if ((fd < 0) || (NULL == buff) || (len <= 0) || (timeout < 0)) {
        return -1;
    }

    pollfd.fd       = fd;
    pollfd.events   = POLLIN;
    pollfd.revents  = 0;

    if (poll(&pollfd, 1, timeout) != 1) {
        return -2;
    }

    if (pollfd.revents & POLLIN) {
        ret = recvfrom(fd, buff, len, 0, NULL, NULL);
        if (ret == -1) {
            return -3;
        }

        return ret;
    }

    return -4;
}


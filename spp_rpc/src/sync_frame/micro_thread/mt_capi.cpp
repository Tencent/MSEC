
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
 *  @filename mt_sys_call.cpp
 *  @info  微线程封装系统api, 同步调用微线程API，实现异步调度
 */

#include "mt_api.h"
#include "mt_capi.h"

using namespace NS_MICRO_THREAD;


/**
 * @brief 微线程主动sleep接口, 单位ms
 * @info  业务需要主动让出CPU时使用
 */
void mtc_sleep(int ms)
{
    mt_sleep(ms);
}

/**
 * @brief 微线程获取系统时间，单位ms
 */
unsigned long long mtc_time_ms(void)
{
    return mt_time_ms();
}

/**
 * @brief 设置当前微线程的私有变量
 * @info  只保存指针，内存需要业务分配
 */
void mtc_set_private(void *data)
{
    mt_set_msg_private(data);
}

/**
 * @brief  获取当前微线程的私有变量
 * @return 私有变量指针
 */
void* mtc_get_private(void)
{
    return mt_get_msg_private();
}

/**
 * @brief  微线程框架初始化
 * @info   业务不使用spp，裸用微线程，需要调用该函数初始化框架；
 *         使用spp，直接调用SyncFrame的框架初始化函数即可
 * @return <0 初始化失败  0 初始化成功
 */
int mtc_init_frame(void)
{
    if (mt_init_frame())
    {
        return 0;
    }

    return -1;
}

/**
 * @brief 设置微线程独立栈空间大小
 * @info  非必须设置，默认大小为128K
 */
void mtc_set_stack_size(unsigned int bytes)
{
    mt_set_stack_size(bytes);
}

/**
 * @brief 微线程包裹的系统IO函数 recvfrom
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param len 接收消息缓冲区长度
 * @param from 来源地址的指针
 * @param fromlen 来源地址的结构长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
int mtc_recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen, int timeout)
{
    return mtc_recvfrom(fd, buf, len, flags, from, fromlen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 sendto
 * @param fd 系统socket信息
 * @param msg 待发送的消息指针
 * @param len 待发送的消息长度
 * @param to 目的地址的指针
 * @param tolen 目的地址的结构长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
int mtc_sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, int tolen, int timeout)
{
    return mt_sendto(fd, msg, len, flags, to, tolen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 connect
 * @param fd 系统socket信息
 * @param addr 指定server的目的地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
int mtc_connect(int fd, const struct sockaddr *addr, int addrlen, int timeout)
{
    return mt_connect(fd, addr, addrlen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 accept
 * @param fd 监听套接字
 * @param addr 客户端地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >=0 accept的socket描述符, <0 失败
 */
int mtc_accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int timeout)
{
    return mt_accept(fd, addr, addrlen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 read
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param nbyte 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mtc_read(int fd, void *buf, size_t nbyte, int timeout)
{
    return mt_read(fd, buf, nbyte, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 write
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mtc_write(int fd, const void *buf, size_t nbyte, int timeout)
{
    return mt_write(fd, buf, nbyte, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 recv
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param len 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mtc_recv(int fd, void *buf, int len, int flags, int timeout)
{
    return mt_recv(fd, buf, len, flags, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 send
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mtc_send(int fd, const void *buf, size_t nbyte, int flags, int timeout)
{
    return mt_send(fd, buf, nbyte, flags, timeout);
}

/**
 * @brief 微线程等待epoll事件的包裹函数
 * @param fd 系统socket信息
 * @param events 等待的事件 IN/OUT
 * @param timeout 最长等待时间, 毫秒
 * @return >0 到达的事件, <0 失败
 */
int mtc_wait_events(int fd, int events, int timeout)
{
    return mt_wait_events(fd, events, timeout);
}


/**
 * @brief 创建微线程
 * @param entry   入口函数指针，类型见ThreadStart
 * @param args    入口函数参数
 * @return   0 创建成功  <0 创建失败
 */
int mtc_start_thread(void* entry, void* args)
{
    if (mt_start_thread(entry, args)) {
        return 0;
    }

    return -1;
}

/**
 * @brief 从TCP连接池中获取连接，如果没有，则新创建
 */
void *mtc_get_keep_conn(struct sockaddr_in* dst, int *sock)
{
    return mt_get_keep_conn(dst, sock);
}

/**
 * @brief  释放TCP连接到连接池
 * @param  conn   mt_get_keep_conn返回的连接对象指针
 * @param  force  1  -> 强制释放连接，不会放入连接池
 * @param         0  -> 释放到连接池，连接池满了，关闭连接
 */
void mtc_free_keep_conn(void *conn, int force)
{
    mt_free_keep_conn(conn, (bool)force);
}




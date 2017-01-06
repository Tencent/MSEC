
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
 *  @filename mt_api.h
 *  @info  微线程封装系统api, 同步调用微线程API，实现异步调度
 */
 
#ifndef __MT_API_H__
#define __MT_API_H__
 
#include <netinet/in.h>
#include <vector>

using std::vector;

namespace NS_MICRO_THREAD {

/******************************************************************************/
/*  微线程用户接口定义: UDP短连接收发接口                                     */
/******************************************************************************/

/**
 * @brief 采用随机端口的socket收发接口, 由socket来决定上下文, 业务来保证上下文
 *        [注意] UDP发送buff, 不推荐static变量, 有一定风险导致上下文错乱[重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param rcv_buf -接收应答包的buff
 * @param buf_size -modify-接收应答包的buff大小, 成功返回时, 修改为应答包长度
 * @param timeout -超时时间, 单位ms
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -3 接收应答失败, 可打印errno
 */
int mt_udpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout);


/******************************************************************************/
/*  微线程用户接口定义: TCP连接池收发接口                                     */
/******************************************************************************/

/**
 * @brief TCP检测报文是否接收完整的回调函数定义
 * @param buf 报文保存缓冲区
 * @param len 已经接收的长度
 * @return >0 实际的报文长度; 0 还需要等待接收; <0 报文异常
 */
typedef int (*MtFuncTcpMsgLen)(void* buf, int len);

/**
 * @brief TCP会采用连接池的方式复用IP/PORT连接, 连接保持默认10分钟
 *        [注意] tcp接收发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param rcv_buf -接收应答包的buff
 * @param buf_size -modify-接收应答包的buff大小, 成功返回时, 修改为应答包长度
 * @param timeout -超时时间, 单位ms
 * @param check_func -检测报文是否成功到达函数
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -3 接收应答失败, 
 *          -4 连接失败, -5 检测报文失败, -6 接收空间不够, -7 后端主动关闭连接，-10 参数无效
 */
int mt_tcpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, 
                  int timeout, MtFuncTcpMsgLen chek_func);

/**
 * @brief TCP会采用连接池的方式复用IP/PORT连接, 连接保持默认10分钟
 *        [注意] tcp接收发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 *        [注意] 修改接口，请注意不要随便修改返回值，并保证和mt_tcpsendrcv_ex返回值匹配 [重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param rcv_buf -接收应答包的buff
 * @param buf_size -modify-接收应答包的buff大小, 成功返回时, 修改为应答包长度
 * @param timeout -超时时间, 单位ms
 * @param check_func -检测报文是否成功到达函数
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -3 接收应答失败, 
 *          -4 连接失败, -5 检测报文失败, -6 接收空间不够, -7 后端主动关闭连接，-10 参数无效, -11内存分配失败
 */
int mt_tcpsendrcv_v2(struct sockaddr_in* dst, void* pkg, int len, void** rcv_buf, int& buf_size,
                      int timeout, MtFuncTcpMsgLen func);


enum MT_TCP_CONN_TYPE
{
    MT_TCP_SHORT         = 1, /// 短连接
    MT_TCP_LONG          = 2, /// 长连接
    MT_TCP_SHORT_SNDONLY = 3, /// 短连接只发
    MT_TCP_LONG_SNDONLY  = 4, /// 长连接只发
    MT_TCP_BUTT
};

/**
 * @brief TCP收发接口，可以选择后端保持连接或者短连接
 *        [注意] tcp接收发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param rcv_buf -接收应答包的buff，只发不收可以设置为NULL
 * @param buf_size -modify-接收应答包的buff大小, 成功返回时, 修改为应答包长度，只发不收，设置为NULL
 * @param timeout -超时时间, 单位ms
 * @param check_func -检测报文是否成功到达函数
 * @param type - 连接类型
 *               MT_TCP_SHORT: 一收一发短连接；
 *               MT_TCP_LONG : 一发一收长连接；
 *               MT_TCP_LONG_SNDONLY : 只发不收长连接； 
 *               MT_TCP_SHORT_SNDONLY: 只发不收短连接；
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -3 接收应答失败, 
 *          -4 连接失败, -5 检测报文失败, -6 接收空间不够, -7 后端主动关闭连接, -10 参数无效
 */
int mt_tcpsendrcv_ex(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int* buf_size,
                     int timeout, MtFuncTcpMsgLen func, MT_TCP_CONN_TYPE type = MT_TCP_LONG);


/******************************************************************************/
/*  微线程用户接口定义: 微线程Task多路并发模型接口定义                        */
/******************************************************************************/

/**
 * @brief  微线程子任务对象定义
 */
class IMtTask
{
public:

    /**
     * @brief  微线程任务类的处理流程入口函数
     * @return 0 -成功, < 0 失败 
     */
    virtual int Process() { return -1; };

    /**
     * @brief 设置task执行结果
     * @info  即Process返回值
     */
    void SetResult(int rc)
    {
        _result = rc;
    }

    /**
     * @brief 获取task执行结果
     * @info  即Process返回值
     */
    int GetResult(void)
    {
        return _result;
    }

    /**
     * @brief 设置task类型
     */
    void SetTaskType(int type)
    {
        _type = type;
    }

    /**
     * @brief  获取task类型
     * @info   如果业务有多种task，可以使用该字段区分不同的task类型
     * @return 获取task类型
     */
    int GetTaskType(void)
    {
        return _type;
    }
 
    /**
     * @brief  微线程任务基类构造与析构
     */
    IMtTask() {};
    virtual ~IMtTask() {};

protected:

    int _type;      // task类型，多种类型task，业务可以自定义类型，方便从基类转换
    int _result;    // task执行结果，即Process返回值
};

typedef vector<IMtTask*>  IMtTaskList;

/**
 * @brief 多路IO并发, Task-fork-wait模式接口
 * @param req_list -task list 封装独立api的task列表
 * @return  0 成功, -1 创建子线程失败
 */
int mt_exec_all_task(IMtTaskList& req_list);


/******************************************************************************/
/*  微线程用户接口定义: 微线程封装系统接口                                    */
/******************************************************************************/

/**
 * @brief 微线程主动sleep接口, 单位ms
 * @info  业务需要主动让出CPU时使用
 */
void mt_sleep(int ms);

/**
 * @brief 微线程获取系统时间，单位ms
 */
unsigned long long mt_time_ms(void);

/******************************************************************************/
/*  微线程用户接口定义: 微线程用户私有数据接口                                */
/******************************************************************************/

/**
 * @brief 设置当前IMtMsg的私有变量
 * @info  只保存指针，内存需要业务分配
 */
void mt_set_msg_private(void *data);

/**
 * @brief  获取当前IMtMsg的私有变量
 * @return 私有变量指针
 */
void* mt_get_msg_private();


/******************************************************************************/
/*  微线程用户接口定义: 微线程封装系统接口(不推荐使用)                        */
/******************************************************************************/

/**
 * @brief  微线程框架初始化
 * @info   业务不使用spp，裸用微线程，需要调用该函数初始化框架；
 *         使用spp，直接调用SyncFrame的框架初始化函数即可
 * @return false:初始化失败  true:初始化成功
 */
bool mt_init_frame(void);

/**
 * @brief 设置微线程独立栈空间大小
 * @info  非必须设置，默认大小为128K
 */
void mt_set_stack_size(unsigned int bytes);

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
int mt_recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen, int timeout);

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
int mt_sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, int tolen, int timeout);


/**
 * @brief 微线程包裹的系统IO函数 connect
 * @param fd 系统socket信息
 * @param addr 指定server的目的地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
int mt_connect(int fd, const struct sockaddr *addr, int addrlen, int timeout);

/**
 * @brief 微线程包裹的系统IO函数 accept
 * @param fd 监听套接字
 * @param addr 客户端地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >=0 accept的socket描述符, <0 失败
 */
int mt_accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int timeout);

/**
 * @brief 微线程包裹的系统IO函数 read
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param nbyte 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mt_read(int fd, void *buf, size_t nbyte, int timeout);

/**
 * @brief 微线程包裹的系统IO函数 write
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mt_write(int fd, const void *buf, size_t nbyte, int timeout);

/**
 * @brief 微线程包裹的系统IO函数 recv
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param len 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mt_recv(int fd, void *buf, int len, int flags, int timeout);

/**
 * @brief 微线程包裹的系统IO函数 send
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mt_send(int fd, const void *buf, size_t nbyte, int flags, int timeout);


/**
 * @brief 微线程等待epoll事件的包裹函数
 * @param fd 系统socket信息
 * @param events 等待的事件 IN/OUT
 * @param timeout 最长等待时间, 毫秒
 * @return >0 到达的事件, <0 失败
 */
int mt_wait_events(int fd, int events, int timeout);

/**
 * @brief 创建微线程
 * @param entry   入口函数指针，类型见ThreadStart
 * @param args    入口函数参数
 * @return   MicroThread指针  NULL 失败
 */
void* mt_start_thread(void* entry, void* args);

/**
 * @brief 从TCP连接池中获取连接，如果没有，则新创建
 * @param dst  目的地址
 * @param sock socket文件描述符
 * @return 返回微线程TCP连接对象
 */
void *mt_get_keep_conn(struct sockaddr_in* dst, int *sock);


/**
 * @brief  释放TCP连接到连接池
 * @param  conn   mt_get_keep_conn返回的连接对象指针
 * @param  force  true  -> 强制释放连接，不会放入连接池
 * @param         false -> 释放到连接池，连接池满了，关闭连接
 */
void mt_free_keep_conn(void *conn, bool force);



}

#endif

 

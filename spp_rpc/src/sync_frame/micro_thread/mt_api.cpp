
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

#include "epoll_proxy.h"
#include "micro_thread.h"
#include "mt_connection.h"
#include "mt_api.h"
#include "mt_monitor.h"

namespace NS_MICRO_THREAD {

/**
 * @brief 采用随机端口的socket收发接口, 由socket来决定上下文, 业务来保证上下文
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param rcv_buf -接收应答包的buff
 * @param buf_size -modify-接收应答包的buff大小, 成功返回时, 修改为应答包长度
 * @param timeout -超时时间, 单位ms
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -3 接收应答失败, 可打印errno
 */
int mt_udpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout)
{
    int ret = 0;
    int rc  = 0;
    int flags = 1;
    struct sockaddr_in from_addr = {0};
    int addr_len = sizeof(from_addr);
    
    int sock = socket(PF_INET, SOCK_DGRAM, 0);
    if ((sock < 0) || (ioctl(sock, FIONBIO, &flags) < 0))
    {
        MT_ATTR_API(320842, 1); // socket失败
        MTLOG_ERROR("mt_udpsendrcv new sock failed, sock: %d, errno: %d", sock, errno);
        ret = -1;
        goto EXIT_LABEL;
    }
    
    rc = MtFrame::sendto(sock, pkg, len, 0, (struct sockaddr*)dst, (int)sizeof(*dst), timeout);
    if (rc < 0)
    {
        MT_ATTR_API(MONITOR_MT_SEND_ERR, 1); // 发送失败
        MTLOG_ERROR("mt_udpsendrcv send failed, rc: %d, errno: %d", rc, errno);
        ret = -2;
        goto EXIT_LABEL;
    }

    rc = MtFrame::recvfrom(sock, rcv_buf, buf_size, 0, (struct sockaddr*)&from_addr, (socklen_t*)&addr_len, timeout);
    if (rc < 0)
    {
        MT_ATTR_API(MONITOR_MT_RECV_FAIL, 1); // 接收未完全成功
        MTLOG_ERROR("mt_udpsendrcv recv failed, rc: %d, errno: %d", rc, errno);
        ret = -3;
        goto EXIT_LABEL;
    }
    buf_size = rc;

EXIT_LABEL:

    if (sock > 0)
    {
        close(sock);
        sock = -1;
    }

    return ret; 
}

/**
 * @brief  创建TCP套接字，并设置为非阻塞
 * @return >=0 成功, <0 失败
 */
int mt_tcp_create_sock(void)
{
    int fd;
    int flag;

    // 创建socket
    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        MTLOG_ERROR("create tcp socket failed, error: %m");
        return -1;
    }

    // 设置socket非阻塞
    flag = fcntl(fd, F_GETFL, 0);
    if (flag == -1)
    {
        ::close(fd);
        MTLOG_ERROR("get fd flags failed, error: %m");
        return -2;
    }

    if (flag & O_NONBLOCK)
        return fd;

    if (fcntl(fd, F_SETFL, flag | O_NONBLOCK | O_NDELAY) == -1)
    {
        ::close(fd);
        MTLOG_ERROR("set fd flags failed, error: %m");
        return -3;
    }

    return fd;
}

/**
 * @brief TCP获取连接与通知对象与socket
 */
static TcpKeepConn* mt_tcp_get_keep_conn(struct sockaddr_in* dst, int& sock)
{
    // 1. 获取线程通知注册对象
    EpollerObj* ntfy_obj = NtfyObjMgr::Instance()->GetNtfyObj(NTFY_OBJ_THREAD, 0);
    if (NULL == ntfy_obj)
    {
        MTLOG_ERROR("get notify failed, logit");
        return NULL;
    }

    // 2. 获取连接对象, 关联通知信息
    TcpKeepConn* conn = dynamic_cast<TcpKeepConn*>(ConnectionMgr::Instance()->GetConnection(OBJ_TCP_KEEP, dst));
    if (NULL == conn)
    {
        MTLOG_ERROR("get connection failed, dst[%p]", dst);
        NtfyObjMgr::Instance()->FreeNtfyObj(ntfy_obj);
        return NULL;
    }
    conn->SetNtfyObj(ntfy_obj);

    // 3. 创建或复用socket句柄
    int osfd = conn->CreateSocket();
    if (osfd < 0)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, true);
        MTLOG_ERROR("create socket failed, ret[%d]", osfd);
        return NULL;
    }

    // 4. 成功返回数据
    sock = osfd;
    return conn;
}

/**
 * @brief TCP循环接收, 直到数据OK或超时
 *       [注意] 开发者不要随意修改函数返回值，保证不要和mt_tcpsendrcv等调用接口冲突 [重要]
 */
static int mt_tcp_check_recv(int sock, char* rcv_buf, int &len, int flags, int timeout, MtFuncTcpMsgLen func)
{
    int recv_len = 0;
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    do
    {
        utime64_t cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
        if (cost_time > (utime64_t)timeout)
        {
            errno = ETIME;            
            MTLOG_ERROR("tcp socket[%d] recv not ok, timeout", sock);
            return -3;
        }

        int rc = MtFrame::recv(sock, (rcv_buf + recv_len), (len - recv_len), 0, (timeout - (int)cost_time));
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] recv failed ret[%d][%m]", sock, rc);
            return -3;
        }
		else if (rc == 0)
        {
        	len = recv_len;
            MTLOG_ERROR("tcp socket[%d] remote close", sock);
            return -7;
        }
        recv_len += rc;

        /* 检查报文完整性 */
        rc = func(rcv_buf, recv_len);
        if (rc < 0)
        {
            MTLOG_ERROR("tcp socket[%d] user check pkg error[%d]", sock, rc);
            return -5;
        }
        else if (rc == 0) // 报文未收完整
        { 
            if (len == recv_len) // 没空间再接收了, 报错
            {
                MTLOG_ERROR("tcp socket[%d] user check pkg not ok, but no more buff", sock);
                return -6;
            }
            continue;
        }
        else    // 成功计算报文长度
        { 
            if (rc > recv_len) // 报文还未收全
            {
                continue;
            }
            else
            {
                len = rc;
                break;
            }
        }
    } while (true);

    return 0;
}

/**
 * @brief TCP循环接收, 直到数据OK或超时
 *       [注意] 开发者不要随意修改函数返回值，保证不要和mt_tcpsendrcv等调用接口冲突 [重要]
 */
static int mt_tcp_check_recv_v2(int sock, char** rcv_buf, int &len, int flags, int timeout, MtFuncTcpMsgLen func)
{
#define DEFAULT_BUFFER_SIZE 64*1024

    int recv_len = 0;
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    char *buf = (char *)malloc(DEFAULT_BUFFER_SIZE);
    char *new_buf;
    int   blen = DEFAULT_BUFFER_SIZE;
    int   new_size;

    if (buf == NULL) {
        errno = ENOMEM;
        MTLOG_ERROR("alloc recv_buf failed", sock);
        return -11;
    }

    do {
        utime64_t cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
        if (cost_time >= (utime64_t)timeout)
        {
            free(buf);
            errno = ETIME;            
            MTLOG_ERROR("tcp socket[%d] recv not ok, timeout", sock);
            return -3;
        }

        int rc = MtFrame::recv(sock, (buf + recv_len), (blen - recv_len), 0, (timeout - (int)cost_time));
        if (rc < 0)
        {
            free(buf);
            MTLOG_ERROR("tcp socket[%d] recv failed ret[%d][%m]", sock, rc);
            return -3;
        }
        else if (rc == 0)
        {
            free(buf);
        	blen = recv_len;
            MTLOG_ERROR("tcp socket[%d] remote close", sock);
            return -7;
        }
        recv_len += rc;

        /* 检查报文完整性 */
        rc = func(buf, recv_len);
        if (rc < 0)
        {
            free(buf);
            MTLOG_ERROR("tcp socket[%d] user check pkg error[%d]", sock, rc);
            return -5;
        }

        /* 报文没有接收完成，且buff不够，需要realloc内存 */
        if (rc == 0 && blen == recv_len) // 没有空间接受报文了
        {
            new_size = (rc > blen) ? rc : 2*blen;
            new_buf  = (char *)realloc(buf, new_size);
            if (NULL == new_buf)
            {
                errno = ENOMEM;            
                MTLOG_ERROR("realloc recv_buf failed");
                free(buf);
                return -11;
            }

            blen = new_size;
            buf  = new_buf;
            continue;
        }

        if ((rc > 0) && (rc <= recv_len)) // 报文接收完成
        {
            *rcv_buf = buf;
            len      = recv_len;
            break;
        }
    } while (true);

    return 0;
}


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
 *          -4 连接失败, -5 检测报文失败, -6 接收空间不够, -7 后端主动关闭连接，-10 参数无效
 */
int mt_tcpsendrcv(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout, MtFuncTcpMsgLen func)
{
    if (!dst || !pkg || !rcv_buf || !func) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p]",
            dst, pkg, rcv_buf, func);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. 获取TCP连接池对象, 挂接通知对象
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. 尝试检测或新建连接
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. 发送数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 4. 接收数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv(sock, (char*)rcv_buf, buf_size, 0, time_left, func);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // 失败则强制释放连接, 否则定时保活
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}

/**
 * @brief TCP会采用连接池的方式复用IP/PORT连接, 连接保持默认10分钟
 *        [注意] tcp接收发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 *        [注意] 修改接口，请注意不要随便修改返回值 [重要]
 *        [注意] 报文的接收buffer为接口malloc申请，需要调用者释放 [重要]
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
int mt_tcpsendrcv_v2(struct sockaddr_in* dst, void* pkg, int len, void** rcv_buf, int& buf_size, int timeout, MtFuncTcpMsgLen func)
{
    if (!dst || !pkg || !rcv_buf || !func) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p]",
            dst, pkg, rcv_buf, func);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. 获取TCP连接池对象, 挂接通知对象
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. 尝试检测或新建连接
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. 发送数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 4. 接收数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv_v2(sock, (char**)rcv_buf, buf_size, 0, time_left, func);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // 失败则强制释放连接, 否则定时保活
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}


/**
 * @brief TCP短连接收发报文
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
 *          -4 连接失败, -5 检测报文失败, -6 接收空间不够, -7 后端主动关闭连接，-10 参数无效
 */
int mt_tcpsendrcv_short(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int& buf_size, int timeout, MtFuncTcpMsgLen func)
{
    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. 参数检查
    if (!dst || !pkg || !rcv_buf || !func) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p]",
                    dst, pkg, rcv_buf, func);
        return -10;
    }

    // 2. 创建TCP socket
    int sock;
    sock = mt_tcp_create_sock();
    if (sock < 0)
    {
        MTLOG_ERROR("create tcp socket failed, ret: %d", sock);
        return -1;
    }

    // 3. 尝试检测或新建连接
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 4. 发送数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    // 5. 接收数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = mt_tcp_check_recv(sock, (char*)rcv_buf, buf_size, 0, time_left, func);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] rcv failed, ret[%d][%m]", sock, rc);
        ret = rc;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:
    if (sock >= 0)
        ::close(sock);

    return ret;
}


/**
 * @brief TCP会采用连接池的方式复用IP/PORT连接, 连接保持默认10分钟
 *        [注意] tcp发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 *        [注意] 修改接口，请注意不要随便修改返回值，并保证和mt_tcpsendrcv_ex返回值匹配 [重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param timeout -超时时间, 单位ms
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -4 连接失败, -10 参数无效
 */
int mt_tcpsend(struct sockaddr_in* dst, void* pkg, int len, int timeout)
{
    if (!dst || !pkg || len < 1) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p]", dst, pkg);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 1. 获取TCP连接池对象, 挂接通知对象
    int sock = -1;
    TcpKeepConn* conn = mt_tcp_get_keep_conn(dst, sock);
    if ((conn == NULL) || (sock < 0))
    {
        MTLOG_ERROR("socket[%d] get conn failed, ret[%m]", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. 尝试检测或新建连接
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. 发送数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    // 失败则强制释放连接, 否则定时保活
    if (conn != NULL)
    {
        ConnectionMgr::Instance()->FreeConnection(conn, (ret < 0));
    }

    return ret;
}

/**
 * @brief TCP短连接只发不收接口
 *        [注意] tcp发送buff, 不可以是static变量, 否则会上下文错乱 [重要]
 *        [注意] 修改接口，请注意不要随便修改返回值，并保证和mt_tcpsendrcv_ex返回值匹配 [重要]
 * @param dst -请求发送的目的地址
 * @param pkg -请求包封装的包体
 * @param len -请求包封装的包体长度
 * @param timeout -超时时间, 单位ms
 * @return  0 成功, -1 打开socket失败, -2 发送请求失败, -4 连接失败, -10 参数无效
 */
int mt_tcpsend_short(struct sockaddr_in* dst, void* pkg, int len, int timeout)
{
    // 1. 入参检查
    if (!dst || !pkg) 
    {
        MTLOG_ERROR("input params invalid, dst[%p], pkg[%p]", dst, pkg);
        return -10;
    }

    int ret = 0, rc = 0;
    int addr_len = sizeof(struct sockaddr_in);
    utime64_t start_ms = MtFrame::Instance()->GetLastClock();
    utime64_t cost_time = 0;
    int time_left = timeout;

    // 2. 创建TCP socket
    int sock = -1;
    sock = mt_tcp_create_sock();
    if (sock < 0)
    {
        MTLOG_ERROR("create tcp socket failed, ret: %d", sock);
        ret = -1;
        goto EXIT_LABEL;
    }

    // 2. 尝试检测或新建连接
    rc = MtFrame::connect(sock, (struct sockaddr *)dst, addr_len, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] connect failed, ret[%d][%m]", sock, rc);
        ret = -4;
        goto EXIT_LABEL;
    }

    // 3. 发送数据处理
    cost_time = MtFrame::Instance()->GetLastClock() - start_ms;
    time_left = (timeout > (int)cost_time) ? (timeout - (int)cost_time) : 0;
    rc = MtFrame::send(sock, pkg, len, 0, time_left);
    if (rc < 0)
    {
        MTLOG_ERROR("socket[%d] send failed, ret[%d][%m]", sock, rc);
        ret = -2;
        goto EXIT_LABEL;
    }

    ret = 0;
    
EXIT_LABEL:

    if (sock >= 0)
        ::close(sock);

    return ret;
}


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
int mt_tcpsendrcv_ex(struct sockaddr_in* dst, void* pkg, int len, void* rcv_buf, int* buf_size, int timeout, MtFuncTcpMsgLen func, MT_TCP_CONN_TYPE type)
{
    switch (type)
    {
        // TCP长连接单发单收
        case MT_TCP_LONG:
        {
            return mt_tcpsendrcv(dst, pkg, len, rcv_buf, *buf_size, timeout, func);
        }

        // TCP长连接只发不收
        case MT_TCP_LONG_SNDONLY:
        {
            return mt_tcpsend(dst, pkg, len, timeout);
        }

        // TCP短连接单发单收
        case MT_TCP_SHORT:
        {
            return mt_tcpsendrcv_short(dst, pkg, len, rcv_buf, *buf_size, timeout, func);
        }

        // TCP短连接只发不收
        case MT_TCP_SHORT_SNDONLY:
        {
            return mt_tcpsend_short(dst, pkg, len, timeout);
        }

        default:
        {
            MTLOG_ERROR("input params invalid, dst[%p], pkg[%p], rcv_buf[%p], fun[%p], type[%d]",
                        dst, pkg, rcv_buf, func, type);
            return -10;
        }
    }

    return 0;
}



/**
 * @brief 子任务回调函数定义
 */
static void mt_task_process(void* arg)
{
    int rc = 0;
    IMtTask* task = (IMtTask*)arg;
    if (!task)
    {
        MTLOG_ERROR("Invalid arg, error");
        return;
    }

    rc = task->Process();
    if (rc != 0)
    { 
        MTLOG_DEBUG("task process failed(%d), log", rc);
    }

    task->SetResult(rc);

    return;
};

/**
 * @brief 多路IO的处理, 开启多个线程管理
 * @param req_list - 任务列表
 * @return 0 成功, <0失败
 */
int mt_exec_all_task(IMtTaskList& req_list)
{
    MtFrame* mtframe    = MtFrame::Instance();
    MicroThread* thread = mtframe->GetActiveThread();
    IMtTask* task       = NULL;
    MicroThread* sub    = NULL;
    MicroThread* tmp    = NULL;
    int rc              = -1;

    MicroThread::SubThreadList list;
    TAILQ_INIT(&list);

    // 防止没有task，导致微线程一直被挂住
    if (0 == req_list.size())
    {
        MTLOG_DEBUG("no task for execult");
        return 0;
    }

    // 1. 创建线程对象
    for (IMtTaskList::iterator it = req_list.begin(); it != req_list.end(); ++it)
    {
        task = *it;
        sub = MtFrame::CreateThread(mt_task_process, task, false);
        if (NULL == sub) 
        {
            MTLOG_ERROR("create sub thread failed");
            goto EXIT_LABEL;
        }
        
        sub->SetType(MicroThread::SUB_THREAD);
        TAILQ_INSERT_TAIL(&list, sub, _sub_entry);
    }

    // 2. 并发执行任务
    TAILQ_FOREACH_SAFE(sub, &list, _sub_entry, tmp)
    {
        TAILQ_REMOVE(&list, sub, _sub_entry);
        thread->AddSubThread(sub);
        mtframe->InsertRunable(sub);
    }

    // 3. 等待子线程执行结束
    thread->Wait();
    rc = 0;
    
EXIT_LABEL:

    TAILQ_FOREACH_SAFE(sub, &list, _sub_entry, tmp)
    {
        TAILQ_REMOVE(&list, sub, _sub_entry);
        mtframe->FreeThread(sub);
    }

    return rc;

}

/**
 * @brief 设置当前IMtMsg的私有变量
 * @info  只保存指针，内存需要业务分配
 */
void mt_set_msg_private(void *data)
{
    MicroThread *msg_thread = MtFrame::Instance()->GetRootThread();
    if (msg_thread != NULL)
        msg_thread->SetPrivate(data);
}

/**
 * @brief  获取当前IMtMsg的私有变量
 * @return 私有变量指针
 */
void* mt_get_msg_private()
{
    MicroThread *msg_thread = MtFrame::Instance()->GetRootThread();
    if (NULL == msg_thread)
    {
        return NULL;
    }

    return msg_thread->GetPrivate();
}

/**
 * @brief  微线程框架初始化
 * @info   业务不使用spp，裸用微线程，需要调用该初始化函数
 * @return false:初始化失败  true:初始化成功
 */
bool mt_init_frame(void)
{
    return MtFrame::Instance()->InitFrame();
}

/**
 * @brief 设置微线程独立栈空间大小
 * @info  非必须设置，默认大小为128K
 */
void mt_set_stack_size(unsigned int bytes)
{
    ThreadPool::SetDefaultStackSize(bytes);
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
int mt_recvfrom(int fd, void *buf, int len, int flags, struct sockaddr *from, socklen_t *fromlen, int timeout)
{
    return MtFrame::recvfrom(fd, buf, len, flags, from, fromlen, timeout);
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
int mt_sendto(int fd, const void *msg, int len, int flags, const struct sockaddr *to, int tolen, int timeout)
{
    return MtFrame::sendto(fd, msg, len, flags, to, tolen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 connect
 * @param fd 系统socket信息
 * @param addr 指定server的目的地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
int mt_connect(int fd, const struct sockaddr *addr, int addrlen, int timeout)
{
    return MtFrame::connect(fd, addr, addrlen, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 accept
 * @param fd 监听套接字
 * @param addr 客户端地址
 * @param addrlen 地址的长度
 * @param timeout 最长等待时间, 毫秒
 * @return >=0 accept的socket描述符, <0 失败
 */
int mt_accept(int fd, struct sockaddr *addr, socklen_t *addrlen, int timeout)
{
    return MtFrame::accept(fd, addr, addrlen, timeout);
}


/**
 * @brief 微线程包裹的系统IO函数 read
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param nbyte 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mt_read(int fd, void *buf, size_t nbyte, int timeout)
{
    return MtFrame::read(fd, buf, nbyte, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 write
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mt_write(int fd, const void *buf, size_t nbyte, int timeout)
{
    return MtFrame::write(fd, buf, nbyte, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 recv
 * @param fd 系统socket信息
 * @param buf 接收消息缓冲区指针
 * @param len 接收消息缓冲区长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功接收长度, <0 失败
 */
ssize_t mt_recv(int fd, void *buf, int len, int flags, int timeout)
{
    return MtFrame::recv(fd, buf, len, flags, timeout);
}

/**
 * @brief 微线程包裹的系统IO函数 send
 * @param fd 系统socket信息
 * @param buf 待发送的消息指针
 * @param nbyte 待发送的消息长度
 * @param timeout 最长等待时间, 毫秒
 * @return >0 成功发送长度, <0 失败
 */
ssize_t mt_send(int fd, const void *buf, size_t nbyte, int flags, int timeout)
{
    return MtFrame::send(fd, buf, nbyte, flags, timeout);
}

/**
 * @brief 微线程主动sleep接口, 单位ms
 */
void mt_sleep(int ms)
{
    MtFrame::sleep(ms); 
}

/**
 * @brief 微线程获取系统时间，单位ms
 */
unsigned long long mt_time_ms(void)
{
    return MtFrame::Instance()->GetLastClock();
}

/**
 * @brief 微线程等待epoll事件的包裹函数
 */
int mt_wait_events(int fd, int events, int timeout)
{
    return MtFrame::Instance()->WaitEvents(fd, events, timeout);
}

void* mt_start_thread(void* entry, void* args)
{
    return MtFrame::Instance()->CreateThread((ThreadStart)entry, args, true);
}

/**
 * @brief 从TCP连接池中获取连接，如果没有，则新创建
 */
void *mt_get_keep_conn(struct sockaddr_in* dst, int *sock)
{
    if (NULL == dst || NULL == sock) {
        return NULL;
    }

    return mt_tcp_get_keep_conn(dst, *sock);
}

/**
 * @brief  释放TCP连接到连接池
 * @param  conn   mt_get_keep_conn返回的连接对象指针
 * @param  force  true  -> 强制释放连接，不会放入连接池
 * @param         false -> 释放到连接池，连接池满了，关闭连接
 */
void mt_free_keep_conn(void *conn, bool force)
{
    if (conn) {
        ConnectionMgr::Instance()->FreeConnection((IMtConnection *)conn, force);;
    }
}


}




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


#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/stat.h>
#include "tsocket.h"

using namespace tbase::tcommu::tsockcommu;

//转换网址地址为字符串
string CSocketAddr::in_n2s(ip_4byte_t addr)
{
    char buf[INET_ADDRSTRLEN];
    const char* p = inet_ntop(AF_INET, &addr, buf, sizeof(buf));
    return p ? p : string();
}

//转换字符串为网络地址
int CSocketAddr::in_s2n(const string& addr, ip_4byte_t& addr_4byte)
{
    struct in_addr sinaddr;
    errno = 0;
    int ret = inet_pton(AF_INET, addr.c_str(), &sinaddr);

    if (ret < 0)
    {
        if (errno != 0)
            return 0 - errno;
        else
            return ret;
    }
    else if (ret == 0)
    {
        return -1;
    }
    else
    {
        addr_4byte = sinaddr.s_addr;
        return 0;	//ret;
    }
}

//创建socket
//	return >0, on success
//	return <=0, on -errno or unknown error
int CSocket::create(int sock_type)
{
    errno = 0;
    int fd = 0;

    switch (sock_type)
    {
    case TCP_SOCKET:
        fd =::socket(PF_INET, SOCK_STREAM, 0);
        break;
    case UDP_SOCKET:
        fd =::socket(PF_INET, SOCK_DGRAM, 0);
        break;
    case UNIX_SOCKET:
        fd =::socket(PF_UNIX, SOCK_STREAM, 0);
        break;
    default:
        break;
    }

    if (fd < 0)
    {
        return errno ? -errno : fd;
    }
    else
    {
        return fd;
    }
}

//关闭socket
void CSocket::close(int fd)
{
    ::close(fd);
}

//绑定端口
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::bind(int fd, const string &server_address, port_t port)
{
    ip_4byte_t ip = 0;
    int ret = CSocketAddr::in_s2n(server_address, ip);

    if (ret < 0) return ret;

    return bind(fd, ip, port);
}
int CSocket::bind(int fd, ip_4byte_t ip, port_t port)
{
    CSocketAddr addr;
    addr.set_family(AF_INET);
    addr.set_port(port);
    addr.set_numeric_ipv4(ip);

    errno = 0;
    int ret = ::bind(fd, addr.addr(), addr.length());
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//对端口port绑定任意ip
//	bind on *:port
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::bind_any(int fd, port_t port)
{
    CSocketAddr addr;
    addr.set_family(AF_INET);
    addr.set_port(port);
    addr.set_numeric_ipv4(htonl(INADDR_ANY));

    errno = 0;
    int ret = ::bind(fd, addr.addr(), addr.length());
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//绑定unix socket
int CSocket::bind(int fd, const string& path, bool isAbstract)
{
    struct sockaddr_un addr;
    bzero(&addr, sizeof(struct sockaddr_un));
    addr.sun_family = PF_UNIX;
    strncpy(addr.sun_path, path.c_str(), path.length());
    socklen_t addrlen = SUN_LEN(&addr);
    if (isAbstract)
    {
        addr.sun_path[0] = '\0';
    }

    errno = 0;
    int ret = ::bind(fd, (struct sockaddr *) & addr, addrlen);
    if (ret < 0)
        return (errno ? -errno : ret);
    if (!isAbstract)
    {
        ret = chmod(addr.sun_path, 0777);
    }
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//监听socket
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::listen(int fd, int backlog)
{
    errno = 0;
    int ret = ::listen(fd, backlog);
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//接收连接
//	accept a new connection, and attach it into the client_socket parameter
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::accept(int fd)
{
    errno = 0;
    int ret =::accept(fd, NULL, NULL);

    if (ret <= 0)
        return errno ? -errno : ret;
    else
    {
        return ret;
    }
}

//发起连接
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::connect(int fd, const string& address, port_t port)
{
    ip_4byte_t ip = 0;
    int ret = CSocketAddr::in_s2n(address, ip);

    if (ret < 0) return ret;

    return connect(fd, ip, port);
}

//发起连接
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::connect(int fd, ip_4byte_t address, port_t port)
{
    CSocketAddr addr;
    addr.set_family(AF_INET);
    addr.set_port(port);
    addr.set_numeric_ipv4(address);
    errno = 0;
    int ret = ::connect(fd, addr.addr(), addr.length());
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//unix socket连接
int CSocket::connect(int fd, const string& path)
{
    struct sockaddr_un addr;
    bzero(&addr, sizeof(struct sockaddr_un));
    addr.sun_family = PF_UNIX;
    strncpy(addr.sun_path, path.c_str(), path.length());
    socklen_t addrlen = SUN_LEN(&addr);
    addr.sun_path[0] = '\0';

    errno = 0;
    int ret = ::connect(fd, (struct sockaddr*) & addr, addrlen);
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//接收数据(TCP)
//	用来接收数据的缓冲区: buf/buf_size
//	已接收的数据长度: received_len
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::receive(int fd, void *buf, unsigned buf_size, unsigned& received_len, int flag /* = 0 */)
{
    errno = received_len = 0;
    int bytes =::recv(fd, buf, buf_size, flag);

    if (bytes < 0)
    {
        return errno ? -errno : bytes;
    }
    else
    {
        received_len = bytes;
        return 0;
    }
}

//接收数据(UDP)
//	用来接收数据的缓冲区: buf/buf_size
//	已接收的数据长度: received_len
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::receive(int fd, void* buf, unsigned buf_size, unsigned& received_len, CSocketAddr& addr)
{
    errno = received_len = 0;
    int bytes =::recvfrom(fd, buf, buf_size, MSG_TRUNC, addr.addr(), &(addr.length()));

    if (bytes < 0)
    {
        return errno ? -errno : bytes;
    }
    else
    {
        received_len = bytes;
        return 0;
    }
}

//发送数据(TCP)
//	用来发送数据的缓冲区及长度: buf/buf_size
//	已发送的数据长度: sent_len
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::send(int fd, const void *buf, unsigned buf_size, unsigned& sent_len, int flag /* = 0 */)
{
    errno = 0;
    int bytes = ::send(fd, buf, buf_size, flag);

    if (bytes < 0)
    {
        return errno ? -errno : bytes;
    }
    else
    {
        sent_len = bytes;
        return 0;
    }
}

//发送数据(UDP)
//	用来发送数据的缓冲区及长度: buf/buf_size
//	已发送的数据长度: sent_len
//	return 0, on success
//	return < 0, on -errno or unknown error
int CSocket::send(int fd, const void *buf, unsigned buf_size, unsigned& sent_len, CSocketAddr& addr)
{
    errno = 0;
    int bytes = ::sendto(fd, buf, buf_size, 0, addr.addr(), addr.length());

    if (bytes < 0)
    {
        return errno ? -errno : bytes;
    }
    else
    {
        sent_len = bytes;
        return 0;
    }
}

//关闭连接
int CSocket::shutdown(int fd)
{
    int ret = ::shutdown(fd, SHUT_RDWR);
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//获取对端地址端口
int CSocket::get_peer_name(int fd, ip_4byte_t& peer_address, port_t& peer_port)
{
    CSocketAddr addr;
    int ret = ::getpeername(fd, addr.addr(), &addr.length());

    if (ret < 0)
        return errno ? -errno : ret;

    peer_address = addr.get_numeric_ipv4();
    peer_port = addr.get_port();
    return 0;
}

int CSocket::get_peer_name(int fd, string & peer_address, port_t & peer_port)
{
    ip_4byte_t ip = 0;
    int ret = get_peer_name(fd, ip, peer_port);

    if (ret < 0) return ret;

    peer_address = CSocketAddr::in_n2s(ip);
    return 0;
}

//获取本地地址和端口
int CSocket::get_sock_name(int fd, ip_4byte_t& socket_address, port_t & socket_port)
{
    CSocketAddr addr;
    int ret = ::getsockname(fd, addr.addr(), &addr.length());

    if (ret < 0)
        return errno ? -errno : ret;

    socket_address = addr.get_numeric_ipv4();
    socket_port = addr.get_port();
    return 0;
}

int CSocket::get_sock_name(int fd, string & socket_address, port_t & socket_port)
{
    ip_4byte_t ip = 0;
    int ret = get_sock_name(fd, ip, socket_port);

    if (ret < 0)
        return ret;

    socket_address = CSocketAddr::in_n2s(ip);
    return 0;
}

//设置地址重用
int CSocket::set_reuseaddr(int fd)
{
    int optval = 1;
    unsigned optlen = sizeof(optval);
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//设置socket非阻塞
int CSocket::set_nonblock(int fd)
{
    int val = fcntl(fd, F_GETFL, 0);

    if (val == -1)
        return errno ? -errno : val;

    if (val & O_NONBLOCK)
        return 0;

    int ret = fcntl(fd, F_SETFL, val | O_NONBLOCK | O_NDELAY);
    return (ret < 0) ? (errno ? -errno : ret) : 0;
}

//设置接收和发送超时时间
void CSocket::set_timeout(int fd, int ms)
{
    struct timeval tv;
    tv.tv_sec = ms / 1000;
    tv.tv_usec = (ms % 1000) * 1000;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

int CSocket::set_recvbuf(int fd, int sz)
{
    int rcvbuf = 0;
    int ret = 0;
    socklen_t optlen = sizeof(rcvbuf);

    ret = getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &optlen);
    if (ret < 0)
    {
        return errno ? -errno : ret;
    }

    if (rcvbuf >= sz)
    {
        return 0;
    }

    ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, sizeof(sz));
    if (ret < 0) 
    {
        return errno ? -errno : ret;
    }

    return 0;
}

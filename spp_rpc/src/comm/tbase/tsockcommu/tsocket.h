
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


#ifndef _TBASE_TSOCKCOMMU_TSOCKET_H_
#define _TBASE_TSOCKCOMMU_TSOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <string.h>

//
//	return value 的规则，
//	如果操作成功，返回 0，
//	如果操作失败，返回一个负数，代表一个errno的相反数或者-1不代表任何意义
//	没有返回正数的情况，所有的输出数据，都以输出参数的形式。
//	例如accept，是否成功在返回值中表示，accept产生的新fd，以参数形式传出
//	再如recv，如果失败，以返回值表示，如果是负数，表示errno，比如-EAGAIN
//	如果在recv中收到对方关闭的消息，recv操作仍然返回0，但是收到的数据长度是0
//
#define TCP_SOCKET  0x1
#define UDP_SOCKET	0x2
#define UNIX_SOCKET 0x4
#define INVALID_SOCKET -1

using namespace std;

namespace tbase
{
namespace tcommu
{
namespace tsockcommu
{
typedef in_addr_t ip_4byte_t;	//	unsigned int
typedef uint16_t port_t;		//	unsigned short
typedef sa_family_t family_t;

//socket地址类
class CSocketAddr
{
public:
    CSocketAddr(): _len(sizeof(struct sockaddr_in))
    {
        memset(&_addr, 0, sizeof(struct sockaddr_in));
    }

    struct sockaddr * addr()
    {
        return (struct sockaddr *)(&_addr);
    }

    struct sockaddr_in * addr_in()
    {
        return &_addr;
    }

    socklen_t& length()
    {
        return _len;
    }

    ip_4byte_t get_numeric_ipv4()
    {
        return _addr.sin_addr.s_addr;
    }

    void set_numeric_ipv4(ip_4byte_t ip)
    {
        _addr.sin_addr.s_addr = ip;
    }

    port_t get_port()
    {
        return ntohs(_addr.sin_port);
    }

    void set_port(port_t port)
    {
        _addr.sin_port = htons(port);
    }

    family_t get_family()
    {
        return _addr.sin_family;
    }

    void set_family(family_t f)
    {
        _addr.sin_family = f;
    }

    static string in_n2s(ip_4byte_t addr);
    static int in_s2n(const string& addr, ip_4byte_t& addr_4byte);

private:
    struct sockaddr_in _addr;
    socklen_t _len;
};

//tcp/udp/unixsocket通讯类
class CSocket
{
public:
    static int create(int sock_type = TCP_SOCKET);
    static int bind(int fd, const string& server_address, port_t port);
    static int bind(int fd, ip_4byte_t ip, port_t port);
    static int bind_any(int fd, port_t port);
    static int bind(int fd, const string& path, bool isAbstract = true);

    static int listen(int fd, int backlog = 32);
    static int accept(int fd);

    static int connect(int fd, ip_4byte_t addr, port_t port);
    static int connect(int fd, const string& addr, port_t port);
    static int connect(int fd, const string& path);

    static int receive(int fd, void* buf, unsigned buf_size, unsigned& received_len, int flag = 0);
    static int receive(int fd, void* buf, unsigned buf_size, unsigned& received_len, CSocketAddr& addr);
    static int send(int fd, const void* buf, unsigned buf_size, unsigned& sent_len, int flag = 0);
    static int send(int fd, const void* buf, unsigned buf_size, unsigned& sent_len, CSocketAddr& addr);

    static int shutdown(int fd);
    static void close(int fd);
    static int set_nonblock(int fd);
    static int set_reuseaddr(int fd);
    static void set_timeout(int fd, int ms);
    static int set_recvbuf(int fd, int sz);
    static int get_peer_name(int fd, ip_4byte_t& peer_address, port_t& peer_port);
    static int get_sock_name(int fd, ip_4byte_t& socket_address, port_t& socket_port);

    static int get_peer_name(int fd, string& peer_address, port_t& peer_port);
    static int get_sock_name(int fd, string& socket_address, port_t& socket_port);
};
}
}
}
#endif

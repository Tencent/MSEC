
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


#include "tsockcommu.h"
#include "tsocket.h"
#include "tconnset.h"
#include "tepollflow.h"
#include "tmempool.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../misc.h"
#include "../../global.h"
#include "../tlog.h"
#include "../../singleton.h"
#include "../../timestamp.h"
#include "../tcommu.h"
#include "../notify.h"
#include "../monitor.h"
#include "../../benchapiplus.h"

#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

using namespace tbase::tlog;
using namespace tbase::tcommu;
using namespace tbase::notify;
using namespace spp::comm;
using namespace spp::singleton;
using namespace tbase::tcommu::tsockcommu;

struct CTSockCommu::TInternal
{
    CMemPool* mempool_;
    CConnSet* connset_;
    CEPollFlow* epollflow_;
};
CTSockCommu::CTSockCommu(): maxconn_(0), expiretime_(0), lastchecktime_(0), flow_(0)
{
    buff_blob_.len = 0;
    buff_blob_.data = NULL;
    buff_blob_.owner = NULL;
    buff_blob_.extdata = NULL;
    memset(&extinfo_, 0x0, sizeof(TConnExtInfo));

    ix_ = new TInternal;
    ix_->mempool_ = NULL;
    ix_->epollflow_ = NULL;
    ix_->connset_ = NULL;

    sendcache_limit_ = 0;
}
CTSockCommu::~CTSockCommu()
{
    fini();

    if (ix_ != NULL)
        delete ix_;
}

/* 析构资源 */
void CTSockCommu::fini()
{
    if (ix_)
    {
        if (ix_->connset_)
        {
            delete ix_->connset_;
            ix_->connset_ = NULL;
        }

        if (ix_->mempool_)
        {
            delete ix_->mempool_;
            ix_->mempool_ = NULL;
        }

        if (ix_->epollflow_)
        {
            delete ix_->epollflow_;
            ix_->epollflow_ = NULL;
        }
    }

    std::map<int, TSockBind>::iterator it;
    for (it = sockbind_.begin(); it != sockbind_.end(); ++it)
    {
        if (it->second.type_ != 0)
            close(it->first);
        if (it->second.type_ == SOCK_TYPE_NOTIFY
                || (it->second.type_ == SOCK_TYPE_UNIX && !it->second.isabstract_))
        {
            unlink(it->second.path_);
        }
    }
}

//not implemented
int CTSockCommu::clear()
{
    return 0;
}

/* 控制空闲连接超时 */
void CTSockCommu::check_expire(void)
{
    time_t now = get_time_s();
    if (0 != expiretime_ && unlikely(now - lastchecktime_ > 1))
    {
        unsigned flow;
        list<unsigned> timeout_list;
        ix_->connset_->check_expire(now - expiretime_, timeout_list);

        MONITOR_INC(MONITOR_TIMER_CLEAN_CONN, timeout_list.size());
        //有超时回调
        if (unlikely(func_list_[CB_TIMEOUT] != NULL))
        {
            buff_blob_.len = 0;
            buff_blob_.data = NULL;
            buff_blob_.extdata = NULL;

            for (list<unsigned>::iterator it = timeout_list.begin(); it != timeout_list.end(); it++)
            {
                flow = *it;

                ix_->connset_->dumpinfo(flow, LOG_DEBUG, "time out closeconn");
                ix_->connset_->closeconn(flow);

                func_list_[CB_TIMEOUT](flow, &buff_blob_, func_args_[CB_TIMEOUT]);
            }
        }
        //无超时回调
        else
        {
            for (list<unsigned>::iterator it = timeout_list.begin(); it != timeout_list.end(); it++)
            {
                ix_->connset_->dumpinfo(*it, LOG_DEBUG, "time out closeconn");
                ix_->connset_->closeconn(*it);
            }
        }

        lastchecktime_ = now;
    }
}

/* 创建listen socket */
int CTSockCommu::create_sock(const TSockBind& s)
{
    //建立tcp\udp\unixsocket侦听fd
    int fd = CSocket::create(s.type_);

    if (unlikely(fd < 0))
        return -1;

    CSocket::set_reuseaddr(fd);

    int ret = CSocket::set_nonblock(fd);

    if (unlikely(ret))
        return ret;

    if (s.type_ == SOCK_TYPE_UDP)
    {
        ret = CSocket::set_recvbuf(fd, 1024*1024);
        if (unlikely(ret))
        {
            printf("[ERROR]Set receive buffer failed! ErrMsg:[%m].\n");
            return ret;
        }
    }

    if (s.type_ != SOCK_TYPE_UNIX)
        ret = CSocket::bind(fd, s.ipport_.ip_, s.ipport_.port_);
    else
        ret = CSocket::bind(fd, s.path_, s.isabstract_);

    if (unlikely(ret))
    {
        printf("[ERROR]Bind port failed! ErrMsg:[%m].\n");

        if (s.type_ != SOCK_TYPE_UNIX)
        {
            struct in_addr ipbuf;
            ipbuf.s_addr = s.ipport_.ip_;
            printf("ip:%s port:%u\n", inet_ntoa(ipbuf), s.ipport_.port_);
        }
        else
        {
            printf("unix domain socket, path:%s\n", s.path_);
        }

        return ret;
    }

    if (s.type_ == SOCK_TYPE_UDP)
        return fd;

    ret = CSocket::listen(fd, 1024);

    if (likely(!ret))
        return fd;
    else
        return -1;
}

/* 接入客户端连接 */
void CTSockCommu::handle_accept(int fd)
{
    int ret = 0;

    //接受socket
    int clifd = 0;

    int accept_cnt_tcp = 50;

    // 每次poll 最多建立50个连接
    while (accept_cnt_tcp --)
    {
        ret = CSocket::accept(fd);

        if (ret < 0 && (errno == EAGAIN))
            break;
        else if (ret < 0 &&(errno == EINTR))
        {
            continue;
        }
        else if (ret < 0)
        {
            INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "accept error, return:%d,ErrMsg:[%m]\n", ret);
            break;
        }

        clifd = ret;

        ret = CSocket::set_nonblock(clifd);

        if (unlikely(ret))
        {
            close(clifd);
            continue;
        }

        //加入连接池
        //bug fix 3350521, by aoxu, 2010-03-09
        if (unlikely(++flow_ >= MAX_FLOW_NUM))
        {
            flow_ = 1;
        }

	    MONITOR(MONITOR_PROXY_ACCEPT_TCP);
        ret = ix_->connset_->addconn(flow_, clifd, sockbind_[fd].type_);

        if (unlikely(ret < 0))
        {
            close(clifd);

            if (func_list_[CB_OVERLOAD] != NULL)
            {
                buff_blob_.len = 0;
                buff_blob_.data = (char*)COMMU_ERR_OVERLOAD_CONN;
                buff_blob_.extdata = NULL;
                func_list_[CB_OVERLOAD](0, &buff_blob_, func_args_[CB_OVERLOAD]);
            }
	        MONITOR(MONITOR_PROXY_REJECT_TCP);
            break;
        }

        ConnCache* cc = ix_->connset_->getcc(flow_);
        //cc->_info.localip_ = sockbind_[fd].ipport_.ip_;
        //cc->_info.localport_ = sockbind_[fd].ipport_.port_;
        CSocket::get_sock_name(clifd, cc->_info.localip_, cc->_info.localport_);
        CSocket::get_peer_name(clifd, cc->_info.remoteip_, cc->_info.remoteport_);

        if(sockbind_[fd].TOS_ >= 0) // 0 <= TOS <= 255
            setsockopt(clifd, IPPROTO_IP, IP_TOS, &(sockbind_[fd].TOS_), sizeof(int));

        //连接建立回调
        if (func_list_[CB_CONNECTED] != NULL)
        {
            buff_blob_.len = 0;
            buff_blob_.data = NULL;

            if (likely(sockbind_[fd].type_ == SOCK_TYPE_TCP))  	//tcp客户端取其信息
            {
                buff_blob_.extdata = &cc->_info;
            }
            else  											//unix socket客户端
            {
                buff_blob_.extdata = NULL;
            }

            if (func_list_[CB_CONNECTED](flow_, &buff_blob_, func_args_[CB_CONNECTED]) != 0)
            {
                INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "deny client fd:%d\n", clifd);
                ix_->connset_->closeconn(flow_);
                continue;
            }
        }

        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "add conn, flow:%d,client fd:%d,sockbind type:%d\n", flow_, clifd, sockbind_[fd].type_);
        //加入epoll池
        ret = ix_->epollflow_->add(clifd, flow_, EPOLLET | EPOLLIN);

        if (unlikely(ret < 0))
        {
            ix_->connset_->closeconn(flow_);
            INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "epollflow add error, return:%d\n", ret);
            break;
        }
        MONITOR(MONITOR_PROXY_ACCEPT_TCP_SUSS);

        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "add epoll flow, flow:%d,client fd:%d\n", flow_, clifd);
    }
}
void CTSockCommu::handle_accept_udp(int fd)
{
    int accept_udp_cnt = 50;
    //每次poll 最多处理50个包
    while (accept_udp_cnt --)
    {
        //加入连接池
        //bug fix 3350521, by aoxu, 2010-03-09

        if (unlikely(++flow_ >= MAX_FLOW_NUM ))
        {
            flow_ = 1;
        }

        int ret = ix_->connset_->addconn(flow_, fd, UDP_SOCKET);

        if (unlikely(ret < 0))
        {
            if (func_list_[CB_OVERLOAD] != NULL)
            {
                buff_blob_.len = 0;
                buff_blob_.data = (char*)COMMU_ERR_OVERLOAD_CONN;
                buff_blob_.extdata = NULL;
                func_list_[CB_OVERLOAD](0, &buff_blob_, func_args_[CB_OVERLOAD]);
            }

        //drop current pkg
        MONITOR(MONITOR_PROXY_DROP_UDP);
#ifndef SPP_UNITTEST
            printf("overload\n");
#endif
            break;
        }

        //收数据
        ret = ix_->connset_->recv(flow_);

        if (likely(ret > 0))
        {
            MONITOR(MONITOR_PROXY_RECV_UDP);
            ConnCache* cc = ix_->connset_->getcc(flow_);
            buff_blob_.len = cc->_r.data_len();
            buff_blob_.data = cc->_r.data();
            /* fd , type 在addconn时已设置，recvtime在recv时设置 */
            cc->_info.localip_ = sockbind_[fd].ipport_.ip_;
            cc->_info.localport_ = sockbind_[fd].ipport_.port_;
            cc->_info.remoteip_ = cc->_addr.get_numeric_ipv4();
            cc->_info.remoteport_ = cc->_addr.get_port();
            buff_blob_.extdata = &cc->_info;

            if (func_list_[CB_CONNECTED] != NULL)
            {
                if (func_list_[CB_CONNECTED](flow_, &buff_blob_, func_args_[CB_CONNECTED]) != 0)
                {
                    ix_->connset_->dumpinfo(flow_, LOG_DEBUG, "CB_CONNECTED execult failed.");
                    ix_->connset_->closeconn(flow_);
                    continue;
                }
            }

            MONITOR(MONITOR_PROXY_PROC_UDP);
            //udp数据一次就可以收完整
            func_list_[CB_RECVDATA](flow_, &buff_blob_, func_args_[CB_RECVDATA]);
        }
        else
        {
            ix_->connset_->dumpinfo(flow_, LOG_DEBUG, "recevice udp package failed, close conn.");
            ix_->connset_->closeconn(flow_);
            break;
        }
    }
}

/* 解析配置，初始化*/
int CTSockCommu::init(const void* config)
{
    fini();

    TSockCommuConf* conf = (TSockCommuConf*)config;

    if (conf->maxconn_ <= 0 || conf->maxpkg_ < 0 || conf->expiretime_ < 0)
        return -1;

    set_sendcache_limit(conf->sendcache_limit_);

    //建立侦听fd
    int fd = 0;

    std::vector<TSockBind>& sockbind = conf->sockbind_;
    std::vector<TSockBind>::iterator it;

    for (it = sockbind.begin(); it != sockbind.end(); ++it)
    {
        fd = create_sock(*it);

        if (fd < 0)
        {
            fini();
            return -2;
        }

        if(it->TOS_ >= 0) // 0 <= TOS <= 255
            setsockopt(fd, IPPROTO_IP, IP_TOS, &it->TOS_, sizeof(int));

        //recv out of data in common tcp stream
        //for qzone client of android upload picture use MSG_OOB bug
        if(it->type_ == SOCK_TYPE_TCP && it->oob_ > 0) {
            int on = 1;
            setsockopt(fd, SOL_SOCKET, SO_OOBINLINE, &on, sizeof(on));
        }

        sockbind_[fd] = *it;
    }

    //最大连接数
    maxconn_ = conf->maxconn_;
    udpautoclose_ = conf->udpautoclose_;

    //创建内存池
    ix_->mempool_ = new CMemPool();
    //创建连接集合
    ix_->connset_ = new CConnSet(*ix_->mempool_, maxconn_);
    //创建epoll对象
    ix_->epollflow_ = new CEPollFlow();
    ix_->epollflow_->create(100000); //目前的epoll实现下，此参数无关紧要

    std::map<int, TSockBind>::iterator mapit;
    for (mapit = sockbind_.begin(); mapit != sockbind_.end(); ++mapit)
    {
        if (mapit->second.type_ == SOCK_TYPE_TCP || mapit->second.type_ == SOCK_TYPE_UNIX)
        {
            ix_->epollflow_->add(mapit->first, 0, EPOLLIN);
        }
        else if (mapit->second.type_ == SOCK_TYPE_UDP)
        {
            ix_->epollflow_->add(mapit->first, 0 , EPOLLIN);
        }
        else    //unused
        {}
    }

    //其他初始化
    expiretime_ = conf->expiretime_;
    buff_blob_.owner = this;
    buff_blob_.extdata = &extinfo_;
    memset(&extinfo_, 0x0, sizeof(TConnExtInfo)); //初始化扩展信息
    lastchecktime_ = time(NULL);//设置上次超时检查时间为当前时间
    flow_ = 0;					//序列号重置
    return 0;
}

int CTSockCommu::addnotify(int groupid)
{
    // add worker -> proxy notify
    char path[255] = {0};
    int notifyfd = CNotify::notify_init(/* comsumer key */groupid * 2 + 1, path, sizeof(path));
    if (notifyfd < 0)
    {
        return -1;
    }

    ix_->epollflow_->add(notifyfd, 0, EPOLLIN); // for recving
    sockbind_[notifyfd].type_ = SOCK_TYPE_NOTIFY;
    strncpy(sockbind_[notifyfd].path_, path, sizeof(sockbind_[notifyfd].path_) - 1);

    return 0;
}


int CTSockCommu::add_notify(int fd)
{
    return ix_->epollflow_->add(fd, 0, EPOLLIN); // for recving
}

int CTSockCommu::del_notify(int fd)
{
    return ix_->epollflow_->del(fd, 0, EPOLLIN); // for recving
}

void CTSockCommu::set_sendcache_limit(unsigned limit)
{
    sendcache_limit_ = limit;
}

bool CTSockCommu::check_over_sendcache_limit(void *cache)
{
    ConnCache* cc = (ConnCache*)cache;

    if (sendcache_limit_)
    {
        return cc->_w.data_len() > sendcache_limit_;
    }

    return false;
}

/**
 * @brief event driven logic
 *
 * @param block 0:在epoll_wait时立即返回，1:epoll_wait 1ms
 *
 * @return 0成功
 */
int CTSockCommu::poll(bool block)
{
    int timeout = 0;

    if (block)
    {
        timeout = 1000;
    }

    CEPollFlowResult result = ix_->epollflow_->wait(timeout);

    int fd = 0;
    unsigned flow = 0;
    int ret = 0;

    for (CEPollFlowResult::iterator it = result.begin(); it != result.end(); it++)
    {
        fd = it.fd();
        flow = it.flow();

        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "epoll event, flow:%d,fd:%d\n", flow, fd);

        //判断是否侦听fd
        if(sockbind_.end() != sockbind_.find(fd))
        {
            //新连接到达(tcp or unix)或者新数据到达(udp)
            if (likely(it->events & EPOLLIN))
            {
                if (sockbind_[fd].type_ == SOCK_TYPE_UDP)
                {
                    handle_accept_udp(fd);
                }
                else if (sockbind_[fd].type_ == SOCK_TYPE_NOTIFY)  	//通知fd
                {
                    CNotify::notify_recv(fd);
                }
                else if (sockbind_[fd].type_ == SOCK_TYPE_TCP
                        || sockbind_[fd].type_ == SOCK_TYPE_UNIX)
                {
                    handle_accept(fd);
                }
            }

            continue;
        }

        //非正常事件，关闭连接
        if (unlikely(!(it->events & (EPOLLIN | EPOLLOUT))))
        {
            ix_->connset_->dumpinfo(flow, LOG_INFO, "unknow event, closeconn flow");
            ix_->connset_->closeconn(flow);

            continue;
            //INTERNAL_LOG->LOG_P_FILE(LOG_NORMAL, "unknow event, close flow:%d\n", flow);
        }

        // notify fd, flow == 0, just wakeup
        if (!flow)
        {
            INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "notify flow: %d, fd: %d, wakeup\n", flow, fd);
            continue;
        }

        //发数据，可能有tcp\unix类型的数据
        if (it->events & EPOLLOUT)
        {
            ret = ix_->connset_->sendfromcache(flow);

            INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "send flow:%d, fd:%d, return:%d\n", flow, fd, ret);

            bool flag_flush_and_close = ix_->connset_->getcc(flow)->_w.get_fin_bit();
            int flush_data_length = ix_->connset_->getcc(flow)->_w.data_len();

            //接收到FIN后，保证缓存数据发送完毕后关闭
            if (ret == 0 && flag_flush_and_close && flush_data_length == 0)
            {
                ix_->connset_->dumpinfo(flow, LOG_DEBUG, "TCP/UNIX closeconn flow");
                ix_->connset_->closeconn(flow);
                //INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "TCP/UNIX closeconn flow:%d\n", flow);
            }

            //缓存发送完毕
            if (ret == 0)
            {
                //去除EPOLLOUT事件
                ix_->epollflow_->modify(fd, flow, EPOLLIN | EPOLLET);
                INTERNAL_LOG->LOG_P_FILE(LOG_TRACE,
                                         "send complete flow:%d, fd:%d, return:%d\n", flow, fd, ret);
            }
            //发送失败，关闭连接
            else if (unlikely(ret == -E_NEED_CLOSE))
            {
                if (unlikely(func_list_[CB_SENDERROR] != NULL))
                {
                    ConnCache* cc = ix_->connset_->getcc(flow);
                    buff_blob_.len = cc->_w.data_len();
                    buff_blob_.data = cc->_w.data();
                    buff_blob_.extdata = &cc->_info;

                    func_list_[CB_SENDERROR](flow, &buff_blob_, func_args_[CB_SENDERROR]);
                }

                ix_->connset_->dumpinfo(flow, LOG_TRACE, "send fail, closeconn flow");
                ix_->connset_->closeconn(flow);
                //INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "send fail, close flow:%d\n", flow);
            }
            //缓存发送未完毕,EPOLLOUT继续存在,如果E_NOT_FINDFD，epoll已经自动的将该fd删除了,无需处理
            else
            {
                //assert(ret > 0 || ret == -E_NOT_FINDFD);
                INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "send not complete flow:%d, fd:%d, return:%d\n", flow, fd, ret);
            }
        }

        //收数据，这里收数据只有tcp和unix类型的
        //收数据异常, 断开连接，需要回调断连函数
        if (it->events & EPOLLIN)
        {
            int retry_cnt = 50;
            while ( retry_cnt-- )
            {
                try
                {
                    ret = ix_->connset_->recv(flow);
                }
                //内存不足
                catch (...)
                {
                    ret = -E_NEED_CLOSE;
                    INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "out of memory\n");
                }

                INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "recv flow:%d, fd:%d, return:%d\n", flow, fd, ret);

                //收到了数据
                if (likely(ret > 0 || ret == -E_RECV_DONE))
                {
                    INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "recv data flow:%d, fd:%d, return:%d\n", flow, fd, ret);
                    ConnCache* cc = ix_->connset_->getcc(flow);
                    buff_blob_.len = cc->_r.data_len();
                    buff_blob_.data = cc->_r.data();
                    buff_blob_.extdata = &cc->_info;

                    int proto_len = func_list_[CB_RECVDATA](flow, &buff_blob_, func_args_[CB_RECVDATA]);

                    //数据不完整
                    if (proto_len == 0)
                    {
                        continue;
                    }
                    //数据完整, proto_len是数据长度
                    else if (proto_len > 0)
                    {
                        MONITOR(MONITOR_PROXY_PROC_TCP);
                        buff_blob_.len = proto_len;

                        if (func_list_[CB_RECVDONE] != NULL)
                            func_list_[CB_RECVDONE](flow, &buff_blob_, func_args_[CB_RECVDONE]);

                        cc->_r.skip(proto_len);
                    }
                    //被判定为错误的数据
                    else
                    {
                        ix_->connset_->dumpinfo(flow, LOG_TRACE, "invalid data, closeconn flow");
                        ix_->connset_->closeconn(flow);
                        //INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "invalid data, close flow:%d\n", flow);
                        break;
                    }

                    // 在epoll的ET模式下，buf未收满表示收完了
                    if(ret == -E_RECV_DONE)
                    {
                        continue;
                    }
                }
                //客户端主动关闭连接或者其他错误
                else if (ret == -E_NEED_CLOSE)    //recv error
                {
                    MONITOR(MONITOR_CLIENT_CLOSE_TCP);
                    if (unlikely(func_list_[CB_DISCONNECT] != NULL))
                    {
                        func_list_[CB_DISCONNECT](flow, &buff_blob_, func_args_[CB_DISCONNECT]);
                        buff_blob_.len = 0;
                        buff_blob_.data = NULL;
                        buff_blob_.extdata = NULL;
                    }
                    ix_->connset_->dumpinfo(flow, LOG_DEBUG, "client close, closeconn flow");
                    ix_->connset_->closeconn(flow);
                    //INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "client close, close flow:%d\n", flow);
                    break;
                }
                //连接已经不存在或者暂时不可读
                else if (ret == -E_NOT_FINDFD || ret == -EAGAIN)
                {
                    INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "recv not complete flow:%d, fd:%d, return:%d\n", flow, fd, ret);
                    break;
                }
            }

            //在执行完一个连接的EPOLL事件后，立刻检查是否超时
            if(expiretime_ != 0) {
                if(ix_->connset_->check_per_expire(flow, get_time_s() - expiretime_)) {
                    if (unlikely(func_list_[CB_DISCONNECT] != NULL))
                    {
                        func_list_[CB_DISCONNECT](flow, &buff_blob_, func_args_[CB_DISCONNECT]);
                        buff_blob_.len = 0;
                        buff_blob_.data = NULL;
                        buff_blob_.extdata = NULL;
                    }

                    ix_->connset_->dumpinfo(flow, LOG_DEBUG, "connection timeout, closeconn flow");
                    ix_->connset_->closeconn(flow);
                }
            }
        }
    }// process poll events

    check_expire();

    return 0;
}
/* 发送数据 ---> client  */
int CTSockCommu::sendto(unsigned flow, void* arg1, void* arg2)
{
    blob_type* blob = (blob_type*)arg1;

    ConnCache* cc = ix_->connset_->getcc(flow);

    INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "blob->len:%d ,flow:%u\n", blob->len, flow);
    if (unlikely(blob->len == 0 && flow > 0))   //主动关闭连接
    {
        //bug fix, modified by jeremy
        //bug 3356131 fixed by aoxu, 2010-02-24

        int flush_data_length = cc->_w.data_len();

        //只有缓冲未发完，才不关闭连接
        if (flush_data_length > 0)
        {
            cc->_w.set_fin_bit(true);
            INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "set fin bit flow:%d, cache_len:%d\n", flow, flush_data_length);
            return 0;
        }

        ix_->connset_->dumpinfo(flow, LOG_DEBUG, "send zero pkg, closeconn flow");
        ix_->connset_->closeconn(flow);
        //INTERNAL_LOG->LOG_P_FILE(LOG_DEBUG, "closeconn flow:%d\n", flow);
        return 0;
    }

    if (check_over_sendcache_limit((void *)cc))  // 超过sendcache限制，关闭连接
    {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "flow [%u] over sendcache limit [%u/%u], closeconn flow", flow, cc->_w.data_len(), sendcache_limit_);
        ix_->connset_->dumpinfo(flow, LOG_ERROR, "over sendcache limit, closeconn flow");
        ix_->connset_->closeconn(flow);
        return 0;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
	int64_t time_delay = CMisc::time_diff(now, cc->_access);
	if(time_delay <= 1)
	{
		MONITOR(MONITOR_PROXY_RELAY_DELAY_1);
	}
	else if(time_delay <= 10)
	{
		MONITOR(MONITOR_PROXY_RELAY_DELAY_10);
	}
	else if(time_delay <= 50)
	{
		MONITOR(MONITOR_PROXY_RELAY_DELAY_50);
	}
	else if(time_delay <= 100)
	{
		MONITOR(MONITOR_PROXY_RELAY_DELAY_100);
	}
	else
	{
		MONITOR(MONITOR_PROXY_RELAY_DELAY_XXX);
	} 

    int ret = 0;
    try
    {
        ret = ix_->connset_->send(flow, blob->data, blob->len);
    }
    //内存不足
    catch (...)
    {
        ret = -E_NEED_CLOSE;
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "out of memory\n");
    }

    //发送了数据
    if (likely(ret >= 0))
    {
        //未发送完
        if (ret < blob->len)
        {
            //如果不是udp则需要添加EPOLLOUT事件
            if (cc->_type != UDP_SOCKET)
            {
                ix_->epollflow_->modify(cc->_fd, flow, EPOLLET | EPOLLIN | EPOLLOUT);
            }
            else
            {
                ix_->connset_->dumpinfo(flow, LOG_ERROR, "udp send error, closeconn flow");
                INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "detail:[Send %d bytes, need Send length %d]\n", ret, blob->len);
                ix_->connset_->closeconn(flow);
            }
        }
        //发送完毕
        else
        {
            if (unlikely(ret != blob->len))
            {
                INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "Send %d bytes > blob length %d\n", ret, blob->len);
                exit(-1);
            }

            if (unlikely(func_list_[CB_SENDDONE] != NULL))
            {
                buff_blob_.len = blob->len;
                buff_blob_.data = blob->data;
                buff_blob_.extdata = &cc->_info;

                func_list_[CB_SENDDONE](flow, &buff_blob_, func_args_[CB_SENDDONE]);
            }

            //如果开启了UDP回包后自动关闭连接的配置，才自动关闭连接
            if (cc->_type == UDP_SOCKET && udpautoclose_)
            {
                ix_->connset_->dumpinfo(flow, LOG_TRACE, "udp auto closeconn flow"); // performance optimization
                ix_->connset_->closeconn(flow);
            }
        }
    }
    else if (ret == -E_NEED_CLOSE)
    {
        //发送出错，关闭连接
        ix_->connset_->dumpinfo(flow, LOG_ERROR, "send error, closeconn flow");
        ix_->connset_->closeconn(flow);
    }

    return ret;
}

/* 控制接口，暂不使用 */
int CTSockCommu::ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2)
{
    return 0;
}
int CTSockCommu::getconn()
{
    return ix_->connset_->getusedflow();
}

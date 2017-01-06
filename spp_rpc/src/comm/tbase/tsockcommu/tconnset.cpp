
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


#include <errno.h>
#include "tconnset.h"
#include "tsocket.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../misc.h"
#include "../tlog.h"
#include "../../timestamp.h"
#include "../../global.h"
#include "../../singleton.h"
#include "../monitor.h"

using namespace spp::singleton;
using namespace tbase::tlog;
using namespace tbase::tcommu::tsockcommu;
using namespace spp::comm;

static const unsigned C_READ_BUFFER_SIZE = 64 * 1024;
static char RecvBuffer[C_READ_BUFFER_SIZE];

CConnSet::CConnSet(CMemPool& mp, int maxconn): maxconn_(maxconn)
{
    ccs_ = new ConnCache*[maxconn_];

    for (int i = 0; i < maxconn_; ++i) {
        ccs_[i] = new ConnCache(mp);
    }
    usedflow_ = 0;
}

CConnSet::~CConnSet()
{
    ConnCache* cc = NULL;

    for (int i = 0; i < maxconn_; ++i) {
        cc = ccs_[i];

        if (cc->_fd > 0 && cc->_type != UDP_SOCKET)
            close(cc->_fd);

        cc->_r.skip(cc->_r.data_len());
        cc->_w.skip(cc->_w.data_len());

        delete cc;
    }

    delete [] ccs_;
}

//添加连接
int CConnSet::addconn(unsigned& flow, int fd, int type)
{
    ConnCache* cc = ccs_[flow % maxconn_];

    //当前flow不可用，寻找下一个可用的flow
    if (cc->_flow > 0) {
		int i = 0;
        while (++i != maxconn_) {
		    flow = flow + 1;
            flow = flow % MAX_FLOW_NUM;
			if(flow == 0) { //防止flow溢出
				++flow;
			}
			cc = ccs_[flow % maxconn_];

            if (cc->_flow == 0) {
                break;	//找到了可用的flow
            }
        }

        if (i == maxconn_) {
            return -1;	//没有空闲的flow可用
        }
    }

    cc->_flow = flow;
    cc->_fd = fd;
    cc->_info.fd_ = fd;
    cc->_type = type;
    cc->_info.type_ = type;
    gettimeofday(&cc->_access, NULL);
    usedflow_ ++;
    MONITOR(MONITOR_CONNSET_NEW_CONN);
    return 0;
}

//查询fd
int CConnSet::fd(unsigned flow)
{
    ConnCache* cc = ccs_[flow % maxconn_];

    if (flow && cc->_flow == flow) {
        return cc->_fd;
    } else {
        return -1;
    }
}

//接收数据
//返回值：
// -E_NOT_FINDFD
// -E_NEED_CLOSE
// -EAGAIN
// recvd_len > 0, recv data length
int CConnSet::recv(unsigned flow)
{
    //	first, find the fd
    ConnCache* cc = ccs_[flow % maxconn_];

    if (cc->_flow != flow || flow == 0) {
        MONITOR(MONITOR_RECV_FLOWID_ERR); // recv msg flowid invalid
        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "recv but not found, flow:%d,cc->flow:%d\n", flow, cc->_flow);
        return -E_NOT_FINDFD;
    }

    /* 这里保证了接收时间的设置 */
    gettimeofday(&cc->_access, NULL);
    cc->_info.recvtime_ = cc->_access.tv_sec;
    cc->_info.tv_usec   = cc->_access.tv_usec;

    //	second, read from socket
    int fd = cc->_fd;
    unsigned recvd_len = 0;
    int ret = 0;

    if (cc->_type != UDP_SOCKET) {
        ret = CSocket::receive(fd, RecvBuffer, C_READ_BUFFER_SIZE, recvd_len);

        //	third, on data received
        if (ret == 0) {
            if (recvd_len > 0) {
                cc->_r.append(RecvBuffer, recvd_len);
                // 在epoll的ET模式下，recv不满buffer就算收完
                if(recvd_len < C_READ_BUFFER_SIZE)
                {
                    return -E_RECV_DONE;
                }
                return recvd_len;
            } else { //	recvd_len == 0
                return -E_NEED_CLOSE;
            }
        }

        //	fourth, on error occur
        else { //	ret < 0
            if (ret != -EAGAIN) {
                return -E_NEED_CLOSE;
            }

            return -EAGAIN;
        }
    } else {
        ret = CSocket::receive(fd, RecvBuffer, C_READ_BUFFER_SIZE, recvd_len, cc->_addr);

        if (!ret && recvd_len > 0) {
            cc->_r.append(RecvBuffer, recvd_len);
            return recvd_len;
        } else {
            return -E_NEED_CLOSE;
        }
    }
}
//打印flow的信息

void CConnSet::dumpinfo(unsigned flow, int log_type, const char* errmsg)
{
    if (log_type < INTERNAL_LOG->log_level(-1))
    {
        return;
    }

    ConnCache* cc = ccs_[flow%maxconn_];
    if (flow != 0 && cc->_flow == flow)
    {
        char localIpStr[16] = {0};
        strncpy(localIpStr, inet_ntoa(*(struct in_addr*)&cc->_info.localip_), sizeof(localIpStr) - 1);
        char remoteIpStr[16] = {0};
        strncpy(remoteIpStr, inet_ntoa(*(struct in_addr*)&cc->_info.remoteip_), sizeof(remoteIpStr) - 1);
        INTERNAL_LOG->LOG_P_PID(log_type,
        "fd[%d], flow[%u],access[%u s], last recv[%llu ms], now[%llu ms], [%s:%u] to client[%s:%u],errmsg[%s]\n",
                cc->_fd,
                flow,
                cc->_access.tv_sec,
                (int64_t)cc->_info.recvtime_ * 1000 + cc->_info.tv_usec / 1000,
                get_time_ms(),
                localIpStr,
                cc->_info.localport_,
                remoteIpStr,
                cc->_info.remoteport_,
                errmsg);
    }
    else
    {
        INTERNAL_LOG->LOG_P_PID(log_type, "flow[%d] not find, errmsg[%s]\n", flow, errmsg);
    }
    return;
}

//发送数据
//返回值：
// 0 new data not send, only send cache data
// -E_NEED_CLOSE
// send_len >= 0, if send_len > 0 send new data length, send_len = 0 new data not send
// -E_NOT_FINDFD
int CConnSet::send(unsigned flow, const char* data, size_t data_len)
{
    //  first, find the fd
    struct timeval now;
    ConnCache* cc = ccs_[flow % maxconn_];

    if (cc->_flow != flow || flow == 0) {
        MONITOR(MONITOR_SEND_FLOWID_ERR); // rsp msg flowid invalid
        gettimeofday(&now, NULL);
        unsigned flowCostTime =
            (now.tv_sec - cc->_info.recvtime_) * 1000
            + (now.tv_usec - cc->_info.tv_usec) / 1000;

        char localIpStr[16] = {0};
        strncpy(localIpStr, inet_ntoa(*(struct in_addr*)&cc->_info.localip_), sizeof(localIpStr) - 1);
        char remoteIpStr[16] = {0};
        strncpy(remoteIpStr, inet_ntoa(*(struct in_addr*)&cc->_info.remoteip_), sizeof(remoteIpStr) - 1);
        INTERNAL_LOG->LOG_P_PID(LOG_ERROR,
                "flow[%u] maybe cost[%dms], [%s:%u] to client[%s:%u] work group[%d], sendto error[%d], fd not found.\n",
                flow,
                flowCostTime,
                localIpStr,
                cc->_info.localport_,
                remoteIpStr,
                cc->_info.remoteport_,
                spp::global::spp_global::get_cur_group_id(),
                -E_NOT_FINDFD);

        return -E_NOT_FINDFD;
    }

    cc->_access = now;

    //	second, if data in cache, send it first
    int fd = cc->_fd;
    unsigned sent_len = 0;
    int ret = 0;

    if (cc->_w.data_len() != 0) {
        if (cc->_type != UDP_SOCKET) {
            ret = CSocket::send(fd, cc->_w.data(), cc->_w.data_len(), sent_len);
        } else {
            ret = CSocket::send(fd, cc->_w.data(), cc->_w.data_len(), sent_len, cc->_addr);
        }

        //	third, if cache not sent all, append data into w cache, return
        if (ret == -EAGAIN || (ret == 0 && sent_len < cc->_w.data_len())) {
            cc->_w.skip(sent_len);
            cc->_w.append(data, data_len);
            return 0;	//	nothing sent
        } else if (ret < 0) {
            return -E_NEED_CLOSE;
        }
    }

    //	fourth, if cache sent all, send new data
    cc->_w.skip(cc->_w.data_len());
    sent_len = 0;

    if (cc->_type != UDP_SOCKET) {
        ret = CSocket::send(fd, data, data_len, sent_len);
    } else {
        ret = CSocket::send(fd, data, data_len, sent_len, cc->_addr);
    }

    if (ret < 0 && ret != -EAGAIN && ret != -EINPROGRESS) {
        INTERNAL_LOG->LOG_P_PID(LOG_ERROR,
            "flow[%u] send error[ %m ]. E_NEED_CLOSE[%d]\n", flow, -E_NEED_CLOSE);
        return -E_NEED_CLOSE;
    }

    //	fifth, if new data still remain, append into w cache, return
    if (sent_len < data_len) {
        cc->_w.append(data + sent_len, data_len - sent_len);
    }

    return sent_len;
}

//从cache中取数据
//返回值：
// -E_NOT_FINDFD
// 0, send complete
// send_len > 0, send continue
// C_NEED_SEND > 0, send continue
// -E_NEED_CLOSE
int CConnSet::sendfromcache(unsigned flow)
{
    //  first, find the fd
    ConnCache* cc = ccs_[flow % maxconn_];

    if (cc->_flow != flow || flow == 0) {
        return -E_NOT_FINDFD;
    }

    gettimeofday(&cc->_access, NULL);

    //no cache data to send, send completely
    if (cc->_w.data_len() == 0) {
        return 0;
    }

    //	second, if data in cache, send it first
    int fd = cc->_fd;
    unsigned sent_len = 0;
    int ret = 0;

    if (cc->_type != UDP_SOCKET) {
        ret = CSocket::send(fd, cc->_w.data(), cc->_w.data_len(), sent_len);
    } else {
        ret = CSocket::send(fd, cc->_w.data(), cc->_w.data_len(), sent_len, cc->_addr);
    }

    //	third, if cache not sent all, append data into w cache, return
    if (ret == 0) {
        if (sent_len == 0) {
            return C_NEED_SEND;
        } else {
            if (sent_len == cc->_w.data_len()) {	//send completely
                cc->_w.skip(sent_len);
                return 0;
            } else {
                cc->_w.skip(sent_len);
                return sent_len;		//send partly
            }
        }
    } else if (ret == -EAGAIN) {
        return C_NEED_SEND;
    } else {
        return -E_NEED_CLOSE;
    }
}

//关闭连接
int CConnSet::closeconn(unsigned flow)
{
    ConnCache* cc = ccs_[flow % maxconn_];

    if (cc->_flow == flow && flow) {
        cc->_flow = 0;

        if (cc->_type != UDP_SOCKET) {
            close(cc->_fd);
            MONITOR(MONITOR_CLOSE_TCP);
        }
        else
        {
            MONITOR(MONITOR_CLOSE_UDP);
        }

        MONITOR(MONITOR_CONNSET_CLOSE_CONN);

        usedflow_ --;
        cc->_access.tv_sec = 0;		
        cc->_access.tv_usec = 0;
        cc->_type = 0;
        cc->_r.skip(cc->_r.data_len());
        cc->_w.skip(cc->_w.data_len());

        // fixed by ericsha, 2010-11-02, 回应大包时可能引起截断现象
        cc->_w.set_fin_bit(false);
        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "close flow:%d\n", flow);
        return 0;
    } else {
        MONITOR(MONITOR_CLOSE_FLOWID_ERR); // close flowid invalid
        INTERNAL_LOG->LOG_P_FILE(LOG_TRACE, "close but not found, flow:%d\n", flow);
        return -E_NOT_FINDFD;
    }
}

//检查超时
void CConnSet::check_expire(time_t access_deadline, std::list<unsigned>& timeout_flow)
{
    timeout_flow.clear();
    unsigned flow;

    for (int i = 0; i < maxconn_; ++i) {
        if (((flow = ccs_[i]->_flow) > 0) && (ccs_[i]->_access.tv_sec < access_deadline)) {
			
            if (ccs_[i]->_type == UDP_SOCKET) MONITOR(MONITOR_TIMEOUT_UDP);
            if (ccs_[i]->_type == TCP_SOCKET) MONITOR(MONITOR_TIMEOUT_TCP);
		
            timeout_flow.push_back(flow);
        }
    }
}

int CConnSet::check_per_expire(unsigned flow, time_t access_deadline) {
    ConnCache* cc = ccs_[flow % maxconn_];

    if ((flow == cc->_flow) && (cc->_access.tv_sec < access_deadline)) {
        return -1;
    }

    return 0;
}

int CConnSet::getusedflow()
{
    return usedflow_;
}

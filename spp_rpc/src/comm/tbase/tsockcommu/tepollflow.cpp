
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


#include "tepollflow.h"
#include <stdio.h>
#include "../tlog.h"
#include "../../global.h"
#include "../../singleton.h"

using namespace spp::singleton;
using namespace tbase::tlog;
using namespace tbase::tcommu::tsockcommu;

#ifndef UINT_MAX
#define UINT_MAX (~0U)
#endif

/*
鍒涘缓epoll fd
*/
int CEPollFlow::create(size_t iMaxFD)
{
    _maxFD = iMaxFD;
    _fd = epoll_create(1024);

    if (_fd < 0)
        return errno ? -errno : _fd;

    _events = new epoll_event[iMaxFD];
    return 0;
}

/*
绛夊緟epoll浜嬩欢
*/
CEPollFlowResult CEPollFlow::wait(int iTimeout)
{
    errno = 0;
    int nfds = epoll_wait(_fd, _events, _maxFD, iTimeout);

    if (nfds < 0) {
        nfds = 0;

        if (errno != EINTR)
            return CEPollFlowResult(_events, nfds, errno);
    }

    return CEPollFlowResult(_events, nfds);;
}

/*
ctrl epoll unit
*/
int CEPollFlow::ctl(int fd, unsigned flow, int epollAction, int flag)
{
    //assert(_fd != -1);
    if (_fd == -1) {
        INTERNAL_LOG->LOG_P_FILE(LOG_ERROR, "client fd == -1\n");
        exit(-1);
    }

    epoll_event ev;
    ev.data.u64 = (unsigned long long)fd + (((unsigned long long) flow) << 32);
    ev.events = flag;
    errno = 0;
    int ret = epoll_ctl(_fd, epollAction, fd, &ev);

    if (ret < 0)
        return errno ? -errno : ret;

    return 0;
}

/*
get flow id
*/
unsigned CEPollFlowResult::iterator::flow()
{
    if (_res._events == NULL)
        return UINT_MAX;

    unsigned long long u64 = _res._events[_index].data.u64;
    unsigned flow = (u64 >> 32);
    return flow;
}
/*
get event fd
*/
int CEPollFlowResult::iterator::fd()
{
    if (_res._events == NULL)
        return -1;

    unsigned long long u64 = _res._events[_index].data.u64;
    int fd = (u64 & 0x00000000ffffffff);
    return fd;
}

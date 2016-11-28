
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


#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <poller.h>


CPollerObject::CPollerObject(CPollerUnit *o, int fd) :
        netfd(fd),
        ownerUnit(o),
        newEvents(0),
        oldEvents(0),
        epslot(NULL)
{
}

CPollerObject::~CPollerObject()
{
    if (ownerUnit && epslot)
        ownerUnit->FreeEpollSlot(epslot);
    if (netfd > 0) {
        close(netfd);
        //log_debug("netfd[%d] closed", netfd);
        netfd = 0;
    } else {
        //log_debug("netfd[%d], already closed.", netfd);
    }
}

int CPollerObject::AttachPoller(CPollerUnit *unit)
{
    if (unit) {
        if (ownerUnit == NULL)
            ownerUnit = unit;
        else
            return -1;
    }

    if (netfd < 0)
        return -1;

    if (epslot == NULL) {
        if (!(epslot = ownerUnit->AllocEpollSlot())) {
            return -1;
        }

        epslot->poller = this;

        int flag = fcntl(netfd, F_GETFL);
        fcntl(netfd, F_SETFL, O_NONBLOCK | flag);
        struct epoll_event ev;
        ev.events = newEvents;
        ev.data.u64 = (++epslot->seq);
        ev.data.u64 = (ev.data.u64 << 32) + ownerUnit->GetSlotId(epslot);

        if (ownerUnit->Epctl(EPOLL_CTL_ADD, netfd, &ev) == 0) {
            oldEvents = newEvents;
        } else {
            return -1;
        }

        return 0;

    }

    return ApplyEvents();
}

int CPollerObject::DetachPoller()
{
    if (epslot) {
        ownerUnit->FreeEpollSlot(epslot);
        epslot->poller = NULL;
        epslot = NULL;

        struct epoll_event ev;
        if (ownerUnit->Epctl(EPOLL_CTL_DEL, netfd, &ev) == 0)
            oldEvents = newEvents;
        else {
            //log_warning("Epctl: %m");
            return -1;
        }
    }

    return 0;
}

int CPollerObject::ApplyEvents()
{
    if (epslot == NULL || oldEvents == newEvents)
        return 0;

    struct epoll_event ev;

    ev.events = newEvents;
    ev.data.u64 = (++epslot->seq);
    ev.data.u64 = (ev.data.u64 << 32) + ownerUnit->GetSlotId(epslot);

    if (ownerUnit->Epctl(EPOLL_CTL_MOD, netfd, &ev) == 0)
        oldEvents = newEvents;
    else {
        return -1;
    }

    return 0;
}

int CPollerObject::InputNotify(void)
{
    EnableInput(false);

    return POLLER_SUCC;
}

int CPollerObject::OutputNotify(void)
{
    EnableOutput(false);

    return POLLER_SUCC;
}

int CPollerObject::HangupNotify(void)
{
    abort();
    delete this;

    return POLLER_FAIL;
}

CPollerUnit::CPollerUnit(int mp)
{
    maxPollers = mp;
    epfd = -1;
    ep_events = NULL;
    pollerTable = NULL;
    freeSlotList = NULL;
    usedPollers = 0;
}

CPollerUnit::~CPollerUnit()
{
    //for (int i = 0; i < maxPollers; i++) {
    //    if (pollerTable[i].freeList)
    //        continue;

    //    delete pollerTable[i].poller;
    //}

    free(pollerTable);

    if (epfd != -1) {
        close(epfd);
        epfd = -1;
    }

    free(ep_events);
}

int CPollerUnit::SetMaxPollers(int mp)
{
    if (epfd >= 0) {
        return -1;
    }

    maxPollers = mp;

    return 0;
}

int CPollerUnit::InitializePollerUnit(void)
{
    pollerTable = (struct CEpollSlot *)calloc(maxPollers, sizeof(*pollerTable));

    if (!pollerTable) {
        return -1;
    }

    for (int i = 0; i < maxPollers - 1; i++) {
        pollerTable[i].freeList = &pollerTable[i+1];
    }

    pollerTable[maxPollers - 1].freeList = NULL;
    freeSlotList = &pollerTable[0];

    ep_events = (struct epoll_event *)calloc(maxPollers, sizeof(struct epoll_event));

    if (!ep_events) {
        return -1;
    }

    if ((epfd = epoll_create(maxPollers)) == -1) {
        return -1;
    }

    return 0;
}

inline int CPollerUnit::VerifyEvents(struct epoll_event *ev)
{
    int idx = EPOLL_DATA_SLOT(ev);

    if ((idx >= maxPollers) || (EPOLL_DATA_SEQ(ev) != pollerTable[idx].seq)) {
        return -1;
    }

    return 0;
}

void CPollerUnit::FreeEpollSlot(CEpollSlot *p)
{
    p->freeList = freeSlotList;
    freeSlotList = p;
    usedPollers--;
    p->seq++;
}

struct CEpollSlot *CPollerUnit::AllocEpollSlot() {
    CEpollSlot *p = freeSlotList;

    if (0 == p) {
        return NULL;
    }

    usedPollers++;
    freeSlotList = freeSlotList->freeList;
    p->freeList = NULL;

    return p;
}

int CPollerUnit::Epctl(int op, int fd, struct epoll_event *events)
{
    if (epoll_ctl(epfd,  op, fd, events) == -1) {

        return -1;
    }

    return 0;
}

void CPollerUnit::WaitPollerEvents(int timeout)
{
    nrEvents = epoll_wait(epfd, ep_events, maxPollers, timeout);
}

void CPollerUnit::ProcessPollerEvents(void)
{
    int ret_code = -1;

    for (int i = 0; i < nrEvents; i++) {
        if (VerifyEvents(ep_events + i) == -1) {
            continue;
        }

        CEpollSlot *s = &pollerTable[EPOLL_DATA_SLOT(ep_events+i)];
        CPollerObject *p = s->poller;

        p->newEvents = p->oldEvents;

        if (ep_events[i].events & (EPOLLHUP | EPOLLERR)) {
            p->HangupNotify();
            continue;
        }

        if (ep_events[i].events & EPOLLIN) {
            ret_code = p->InputNotify();

            //因为在InputNotify里面已经删除了自己，所以这里必须判断是否已经detach
            if (s->poller != p) {
                continue;
            }

            //再次检测返回值，如果不等于POLLER_SUCC，则表示已经delete掉自己，或者异常
            if (ret_code != POLLER_SUCC) {
                continue;
            }
        }


        if (ep_events[i].events & EPOLLOUT) {
            ret_code = p->OutputNotify();

            //因为在OutputNotify里面已经删除了自己，所以这里必须判断是否已经detach
            if (s->poller != p) {
                continue;
            }

            //再次检测返回值，如果不等于POLLER_SUCC，则表示已经delete掉自己，或者异常
            if (ret_code != POLLER_SUCC) {
                continue;
            }
        }


        //因为在InputNotify, OutputNotify退出后，已经检测指针是否已经detach，因此这里无需判断
        //if(s->poller==p)
        //{
        p->ApplyEvents();
        //}
    }
}



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


#include <stdint.h>
#include <stdlib.h>

#include "timerlist.h"
CTimerObject::~CTimerObject(void)
{
}

void CTimerObject::TimerNotify(void)
{
    abort();
    delete this;
}

void CTimerObject::AttachTimer(class CTimerList *lst)
{
    if (lst->timeout > 0)
        objexp = get_time_ms() + lst->timeout;

    ListMoveTail(lst->tlist);
}

int CTimerList::CheckExpired(int64_t now)
{
    int n = 0;

    if (now == 0) {
        now = get_time_ms();
    }

    while (!tlist.ListEmpty()) {
        CTimerObject *tobj = tlist.NextOwner();

        if (tobj->objexp > now) break;

        tobj->ListDel();
        tobj->TimerNotify();
        n++;
    }

    return n;
}


void CTimerList::TransforTimer(CTimerList* t)
{
    if (t == NULL)
        return;

    while (!tlist.ListEmpty()) {
        CTimerObject *tobj = tlist.NextOwner();
        tobj->ListDel();
        tobj->AttachTimer(t);
    }
}

CTimerList *CTimerUnit::GetTimerList(int to)
{
    CTimerList *tl;

    for (tl = next; tl; tl = tl->next) {
        if (tl->timeout == to)
            return tl;
    }

    tl = new CTimerList(to);
    tl->next = next;

    if (next != NULL) {
        next->prev = &(tl->next);
    }

    next = tl;
    next->prev = &next;
	if(to<timeout_inf||timeout_inf==0)
	{
		timeout_inf=to;

	}
    return tl;
}

CTimerUnit::CTimerUnit(void) : pending(0), next(NULL),timeout_inf(0)
{
}

CTimerUnit::~CTimerUnit(void)
{
    while (next) {
        CTimerList *tl = next;
        next = tl->next;
        delete tl;
    }
};

int CTimerUnit::ExpireMicroSeconds(int msec)
{
    int64_t exp;
    int64_t now;
    CTimerList *tl;

    now = get_time_ms();
    exp = now + msec;

    for (tl = next; tl; tl = tl->next) {
        if (tl->tlist.ListEmpty())
            continue;

        CTimerObject *o = tl->tlist.NextOwner();

        if (o->objexp < exp)
            exp = o->objexp;
    }

    exp -= now;

    if (exp <= 0)
        return 0;

    return exp;
}

int CTimerUnit::CheckPending(void)
{
    int n = 0;

    while (!pending.tlist.ListEmpty()) {
        CTimerObject *tobj = pending.tlist.NextOwner();
        tobj->ListDel();
        tobj->TimerNotify();
        n++;
    }

    return n;
}

int CTimerUnit::CheckExpired(int64_t now)
{
    if (now == 0)
        now = get_time_ms();

    int n = CheckPending();;
    CTimerList *tl;

    for (tl = next; tl; tl = tl->next) {
        n += tl->CheckExpired(now);
    }

    return n;
}

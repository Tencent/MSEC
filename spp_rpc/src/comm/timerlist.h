
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


#ifndef __TIMERLIST_H__
#define __TIMERLIST_H__

#include "list.h"
#include "timestamp.h"

using namespace spp::comm;
class CTimerObject;
class CTimerUnit;

class CTimerList
{
private:
    CListObject<CTimerObject> tlist;
    int timeout;
    CTimerList *next;
    CTimerList **prev;
public:
    friend class CTimerUnit;
    friend class CTimerObject;
    CTimerList(int t) : timeout(t), next(NULL), prev(NULL) { }
    ~CTimerList(void) {

        if (prev != NULL) {
            *prev = next;
        }

        if (next != NULL) {
            if (prev != NULL)
                next->prev = prev;
        }

        tlist.FreeList();
    }
    void TransforTimer(CTimerList* t);
    int CheckExpired(int64_t now = 0);
};

class CTimerUnit
{
private:
    CTimerList pending;
    CTimerList *next;
	int	 timeout_inf;
public:
    friend class CTimerObject;
    CTimerUnit(void);
    ~CTimerUnit(void);
	int GetTimeoutInf()
	{
		return timeout_inf;
	}
    CTimerList *GetTimerList(int);
    int ExpireMicroSeconds(int);
    int CheckExpired(int64_t now = 0);
    int CheckPending(void);
};

class CTimerObject: private CListObject<CTimerObject>
{
private:
    int64_t objexp;

public:
    friend class CTimerList;
    friend class CTimerUnit;
    CTimerObject() { }
    virtual ~CTimerObject(void);
    virtual void TimerNotify(void);
    void DisableTimer(void) {
        ResetList();
    }
    void AttachTimer(class CTimerList *o);
    void AttachZeroTimer(class CTimerUnit *o) {
        ListMoveTail(o->pending.tlist);
    }
};

#endif

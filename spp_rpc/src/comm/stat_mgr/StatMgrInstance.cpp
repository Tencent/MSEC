
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


#include "StatMgrInstance.h"
#if !__GLIBC_PREREQ(2, 3)
#define __builtin_expect(x, expected_value) (x)
#endif
#ifndef likely
#define likely(x)  __builtin_expect(!!(x), 1)
#endif
#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif


USING_SPP_STAT_NS;

CStatMgr* CStatMgrInstance::_mgr = NULL;

CStatMgr* CStatMgrInstance::instance()
{
    if (unlikely(_mgr == NULL))
    {
        _mgr = new CStatMgr();
    }
    return _mgr;
}
void CStatMgrInstance::destroy()
{
    if (_mgr != NULL)
    {
        delete _mgr;
        _mgr = NULL;
    }
}

CStatMgrInstance::CStatMgrInstance()
{
    return ;
}

CStatMgrInstance::~CStatMgrInstance()
{
    return;
}

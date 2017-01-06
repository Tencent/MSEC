
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


#ifndef __SHM_COMMU_H__
#define __SHM_COMMU_H__

#include "tbase/tcommu.h"
#include "memqueue.h"

using namespace tbase::tcommu;
using namespace tbase::tcommu::tshmcommu;

namespace spp
{
namespace comm
{

class CShmCommu : public CTCommu
{
public:
    CShmCommu();
    ~CShmCommu();
    int init(const void* config);
    int clear();
    int poll(bool block = false);
    int sendto(unsigned flow, void* arg1/*数据blob*/, void* arg2/*暂时保留*/);
    int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2);
    int get_msg_count();
protected:
    mem_queue_desc_t producer_;
    mem_queue_desc_t comsumer_;
    int notify_fd_;
    unsigned buff_size_;
    blob_type buff_blob_;
    int locktype_;
    unsigned  msg_timeout;
    void fini();
};



}
}


#endif



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


#ifndef _COMM_INFO_H
#define _COMM_INFO_H

#define PWD_BUF_LEN 256
#define TOKEN_BUF_LEN 300

//proxy statobj
#define RX_BYTES        "rx_bytes"
#define TX_BYTES        "tx_bytes"
#define CONN_OVERLOAD   "conn_overload"
#define SHM_ERROR       "shm_error"
#define CUR_CONN        "cur_connects"

//worker statobj
#define SHM_RX_BYTES    "shm_rx_bytes"
#define SHM_TX_BYTES    "shm_tx_bytes"
#define MSG_TIMEOUT     "msg_timeout"
#define MSG_SHM_TIME    "msg_shm_time"

//mt statobj
#define MT_THREAD_NUM  "mt_thread_num"
#define MT_MSG_NUM     "mt_msg_num"

namespace spp
{

enum CoreCheckPoint
{
    CoreCheckPoint_SppFrame         = 0,
    CoreCheckPoint_HandleInit,          // 1
    CoreCheckPoint_HandleProcess,       // 2
    CoreCheckPoint_HandleFini,          // 3
};

namespace statdef
{
//统计项索引，快速访问StatObj的数组下标
typedef enum DefaultProxyStatObjIndex
{
    PIDX_RX_BYTES = 0,
    PIDX_TX_BYTES,              // 1
    PIDX_CONN_OVERLOAD,         // 2
    PIDX_SHM_ERROR,             // 3
    PIDX_CUR_CONN,              // 4
} proxy_stat_index;

typedef enum DefaultWorkerStatObjIndex
{
    WIDX_SHM_RX_BYTES = 0,
    WIDX_SHM_TX_BYTES,          // 1
    WIDX_MSG_TIMEOUT,           // 2
    WIDX_MSG_SHM_TIME,          // 3
    
    WIDX_MT_THREAD_NUM,	        // 4
    WIDX_MT_MSG_NUM,            // 5
} worker_stat_index;
}
}

#endif


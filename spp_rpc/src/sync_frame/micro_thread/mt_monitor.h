
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


/**
 *  @filename mt_monitor.h
 */
 
#ifndef __MT_MONITOR_H__
#define __MT_MONITOR_H__

#define MONITOR_MT_EPOLL_FD_ERR     "frm.mt fd error"               // 微线程fd非法
#define MONITOR_MT_EPOLL_ERR        "frm.mt epoll error"            // epoll错误
#define MONITOR_MT_POLL_EMPTY       "frm.mt pool empty"             // 微线程池空了
#define MONITOR_MT_OVERLOAD         "frm.mt overload"               // 微线程个数达到上线
#define MONITOR_MT_POLL_INIT_FAIL   "frm.mt pool init failed"       // 微线程池初始化失败
#define MONITOR_MT_HEAP_ERROR       "frm.mt timer heap error"       // 微线程定时器堆错误
#define MONITOR_MT_POOL_SIZE        "frm.mt pool size"              // 微线程池大小
#define MONITOR_MT_SOCKET_ERR       "frm.mt create socket failed"   // 320842
#define MONITOR_MT_SEND_ERR         "frm.mt send failed"            // 发送失败
#define MONITOR_MT_RECV_FAIL        "frm.mt recv failed"            // 接收失败
#define MONITOR_MT_CONNECT_FAIL     "frm.mt connect failed"         // 连接失败
#define MONITOR_MT_SESSION_EXPIRE   "frm.mt session expired"        // udp session超时

#endif

 

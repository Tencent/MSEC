
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


#ifndef _TBASE_TCOMMU_H_
#define _TBASE_TCOMMU_H_
#include <unistd.h>
#include <string.h>
#include <sys/time.h>


#define COMMU_ERR_OVERLOAD_PKG 		-100
#define COMMU_ERR_OVERLOAD_CONN 	-200

/* move from tsockcomm.h */
#define SOCK_TYPE_TCP           0x1
#define SOCK_TYPE_UDP           0x2
#define SOCK_TYPE_UNIX          0x4
#define SOCK_TYPE_NOTIFY        0x8
#define SOCK_MAX_BIND           300

namespace tbase
{
    namespace tcommu
    {

        //回调函数类型
        typedef enum {
            CB_CONNECTED = 0,
            CB_DISCONNECT,
            CB_RECVDATA,
            CB_RECVERROR,
            CB_RECVDONE,
            CB_SENDDATA,
            CB_SENDERROR,
            CB_SENDDONE,
            CB_HANGUP,
            CB_OVERLOAD,
            CB_TIMEOUT,
        } cb_type;

        //控制命令
        typedef enum {
            CT_DISCONNECT = 0,  //断开连接（组件相关）
            CT_CLOSE,			//清理资源（组件相关）
            CT_STAT,			//统计信息（组件相关）
        } ctrl_type;

        //回调函数类型
        //flow: 数据包唯一标示
        //arg1: 通用参数指针1，一般指向数据blob
        //arg2: 通用参数指针2，用户注册回调函数传入的自定义参数指针
        typedef int (*cb_func)(unsigned flow, void* arg1, void* arg2);

        //数据blob
        typedef struct {
            int len;		//数据长度
            char* data;		//数据缓冲区
            void* owner;	//组件指针
            void* extdata;  //扩展数据, 具体组件有具体的含义
        } blob_type;

        //连接扩展信息(作为blob_type中extdata部分)
        typedef struct {
            int fd_;					//fd
            int type_;					//fd type (SOCK_STREAM\SOCK_DGRAM\...)
            unsigned localip_;			//local ip
            unsigned short localport_;	//local port
            unsigned remoteip_;			//remote ip
            unsigned short remoteport_;	//remote port
			time_t  recvtime_;			//recv tv_sec
            suseconds_t tv_usec;        //recv tv_usec
        } TConnExtInfo;

        //通讯类抽象接口
        class CTCommu
        {
        public:
            CTCommu() {
                memset(func_list_, 0, sizeof(cb_func) *(CB_TIMEOUT + 1));
                memset(func_args_, 0, sizeof(void*) *(CB_TIMEOUT + 1));
            }
            virtual ~CTCommu() {}

            //初始化
            //config：配置文件名或者配置参数内存指针
            virtual int init(const void* config) = 0;
            
			//轮询，收发数据
            //block: true表示使用阻塞模式，否则非阻塞模式
            virtual int poll(bool block = false) = 0;

            //发送数据提交
            //flow: 数据包唯一标示
            //arg1: 通用参数指针1， 一般指向数据blob
            //arg2: 通用参数指针2，保留
            virtual int sendto(unsigned flow, void* arg1, void* arg2) = 0;

            //控制接口
            //flow: 数据包唯一标示
            //type: 控制命令
            //arg1: 通用参数指针1，具体组件有具体的含义
            //arg2: 通用参数指针2，具体组件有具体的含义
            virtual int ctrl(unsigned flow, ctrl_type type, void* arg1, void* arg2) = 0;

            //注册回调
            //type: 回调函数类型
            //func: 回调函数
            //args: 用户自定义参数指针, 作为回调函数的第2个通用参数传递
            virtual int reg_cb(cb_type type, cb_func func, void* args = NULL) {
                if (type <= CB_TIMEOUT) {
                    func_list_[type] = func;
                    func_args_[type] = args;
                    return 0;
                } else {
                    return -1;
                }
            }

			//清空所有共享内存队列，仅供proxy启动时使用
			virtual int clear() = 0;

        protected:
            cb_func func_list_[CB_TIMEOUT + 1];
            void* func_args_[CB_TIMEOUT + 1];

            //释放资源
            virtual void fini() = 0;
        };
    }
}
#endif


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
 *  @file SyncFrame.h
 *  @info 同步线程逻辑框架处理
 *  @time 20130515
 **/

#ifndef __NEW_SYNC_FRAME_EX_H__
#define __NEW_SYNC_FRAME_EX_H__
#include <serverbase.h>

namespace SPP_SYNCFRAME {

class CSyncMsg;
using namespace spp::comm;

#define SF_LOG(lvl, fmt, args...)                                           \
do {                                                                        \
    register CServerBase *base = CSyncFrame::Instance()->GetServerBase();   \
    if ((base) && (lvl >= base->log_.log_level(-1))) {                      \
        base->log_.LOG_P_ALL(lvl, fmt"\n", ##args);                         \
    }                                                                       \
} while (0)


/**
 * @brief 微线程同步框架对象
 */
class CSyncFrame
{
    public:
        static bool _init_flag;        ///< 初始化状态标记
        static int  _iNtfySock;        ///< socket类型的通知句柄         
        
		unsigned int _uMsgNo;           ///消息实例数

    public:
        static CSyncFrame* Instance (void);
        static void Destroy (void);

        CSyncFrame();
        ~CSyncFrame();

        /**
         * 初始化异步框架，由插件在worker进程的spp_handle_init里调用
         *
         * @param pServBase CServerBase对象指针
         * @param max_thread_num 最大并发线程数, 可控制极限情况内存占用
         * @param min_thread_num 最小保留线程数, 可控制线程池的空闲线程数目
         *
         * @return 0: 成功； 其他：失败
         *
         */
        int InitFrame(CServerBase *pServBase, int max_thread_num = 50000, int min_thread_num = 200);

        /**
         * 处理请求，由插件在spp_handle_process方法里调用
         *
         * @param pMsg CMsgBase派生类对象指针，存放和请求相关的数据，该对象需要插件以new的方式分配，释放由框架负责
         *
         * @return 0: 成功； 其他：失败
         *
         */ 
        int Process(CSyncMsg *pMsg);

        /**
         * 获取CServerBase对象指针，该对象提供日志、统计功能
         * 宏定义FRAME_LOG是为了简化日志记录的。
         *
         * @return CServerBase对象指针
         **/
        CServerBase* GetServerBase()     {
            return _pServBase;
        }

        
        /**
         *  主线程循环, 每次切换上下文, 让出CPU, 调度框架后端epoll
         */
        void WaitNotifyFd(int utime);

        /**
         *  微线程启动标志, SPP注册内部接口
         */
        bool GetMtFlag();

        /**
         *  主线程循环, 根据前端是否有请求, 决定epoll等待机制
         */
        void HandleSwitch(bool block);

        /**
         *  获取当前处理的请求
         */
        CSyncMsg* GetCurrentMsg();
		
        /**
         *  检查当前栈是否安全
         */
		bool CheckCurrentStack(long lESP);

		/**
         *	获取消息数
         */
         int GetThreadNum();

        /**
         * 设置组ID
         */
        void SetGroupId(int id);

		/**
         *	休眠（切换）
         */
		void sleep(int ms);

    protected:
        static CSyncFrame *_s_instance;     ///< 全局单例句柄
        CServerBase *_pServBase;            ///< SPP服务器句柄    
        int _iGroupId;                      ///< SPP WORKER组id
        int _iNtfyFd;                       ///< 共享内存映射的管道句柄

};


}

#endif

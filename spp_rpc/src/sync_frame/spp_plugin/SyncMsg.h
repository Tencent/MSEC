
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
 *  @file SyncMsg.h
 *  @info 继承自msgbase的头文件定义, 主要扩展了blob信息
 *  @time 20130515
 **/

#ifndef __NEW_SYNC_MSG_EX_H__
#define __NEW_SYNC_MSG_EX_H__
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include "tcommu.h"
#include "serverbase.h"

#include "mt_incl.h"

using std::string;
using namespace NS_MICRO_THREAD;
using namespace tbase;
using namespace tbase::tcommu;

namespace SPP_SYNCFRAME {

/**
 * @brief 微线程同步消息接口
 */
class CSyncMsg : public IMtMsg
{
    public:
        /**
         * 构造析构函数
         */
        CSyncMsg() {
            _srvbase        = NULL;
            _commu          = NULL;
            _flow           = 0;
            _start_time     = mt_time_ms();
            
            memset(&_from_addr, 0, sizeof(_from_addr));
            memset(&_local_addr, 0, sizeof(_local_addr));
            memset(&_time_rcv, 0, sizeof(_time_rcv));
        };
        virtual ~CSyncMsg(){};


        /**
         * 同步消息处理函数
         * @return 0, 成功-用户自己回包到前端,框架不负责回包处理
         *         其它, 失败-框架关闭与proxy连接, 但不负责回业务报文
         */
        virtual int HandleProcess(){ 
            return -1;
        };

        /**
         * 设置CServerBase对象指针，由插件在创建CMsgBase派生类对象之后设置
         */
        void SetServerBase(CServerBase* srvbase)
        {
            _srvbase = srvbase;
        };

        /**
         * 获取CServerBase对象指针
         */
        CServerBase* GetServerBase()
        {
            return _srvbase;
        };

        /**
         * 设置CTCommu对象指针，由插件在创建CMsgBase派生类对象之后设置
         * CTCommu对象在回包时候使用
         */
        void SetTCommu(CTCommu* commu)
        {
            _commu = commu;
        };

        /**
         * 获取CTCommu对象指针
         */
        CTCommu* GetTCommu()
        {
            return _commu;
        };

        /**
         * 设置请求包的flow值，由插件在创建CMsgBase派生类对象之后设置
         */
        void SetFlow(unsigned flow)
        {
            _flow = flow;
        };

        /**
         * 获取请求包的flow值
         */
        unsigned GetFlow()
        {
            return _flow;
        };

        /**
         * 给客户端回包
         *
         * @param blob 回报内容
         */
        int SendToClient(blob_type &blob) {
            if (NULL == _commu) {
                return -999;
            }
            
            int ret = _commu->sendto( _flow, &blob, _srvbase);
            return ret;
        };
       

        /**
         * 设置请求处理总体超时
         *
         * Action处理之前，检查是否超时，如果超时，不会进入Action的实际处理流程，回调IAction::HandleError(EMsgTimeout)
         *
         * @param timeout 请求处理超时配置，单位：ms，默认为0, 0：无需检查请求总体处理超时
         *
         */
        void SetMsgTimeout(int timeout) {
            _msg_timeout = timeout;
        };

        /**
         * 获取请求处理总体超时
         *
         * @return 请求处理超时配置
         *
         */
        int GetMsgTimeout() {
            return _msg_timeout;
        };
        
        /**
         * 获取请求处理时间开销
         *
         * @return 请求处理时间开销，单位: ms
         */
        int GetMsgCost() {
            return (int)(mt_time_ms() - _start_time);
        };

        /**
         * 检查请求处理是否超时？
         *
         * 请求处理的开始时间：CMsgBase对象创建时刻
         *
         * @return true: 超时，false: 还没有超时
         *
         */
        bool CheckMsgTimeout() {
            if (_msg_timeout <= 0) {// 无需检查请求处理超时
                return false;
            }
            
            int cost = GetMsgCost();
            if (cost < _msg_timeout) {// 未超时
                return false;
            }
            
            return true;
        };

        /**
         * 报文目的地址设置与读取接口
         *
         * @param addr 输入地址信息
         */ 
        void SetLocalAddr(const struct sockaddr_in& local_addr) {
            memcpy(&_local_addr, &local_addr, sizeof(_local_addr));
        };
        void GetLocalAddr(struct sockaddr_in& local_addr) {
            memcpy(&local_addr, &_local_addr, sizeof(_local_addr));
        };

        /**
         * 报文来源地址设置与读取接口
         *
         * @param addr 输入地址信息
         */ 
        void SetFromAddr(const struct sockaddr_in& from_addr) {
            memcpy(&_from_addr, &from_addr, sizeof(_from_addr));
        };
        void GetFromAddr(struct sockaddr_in& from_addr) {
            memcpy(&from_addr, &_from_addr, sizeof(_from_addr));
        };

        /**
         * 报文来源时间戳信息
         *
         * @param time_rcv 输入输出时间戳
         */ 
        void SetRcvTimestamp(const struct timeval& time_rcv) {
            memcpy(&_time_rcv, &time_rcv, sizeof(time_rcv));
        };
        void GetRcvTimestamp(struct timeval& time_rcv) {
            memcpy(&time_rcv, &_time_rcv, sizeof(time_rcv));
        };  

        /**
         * 报文内容信息存储, 可选调用
         *
         * @param pkg 报文起始指针
         * @param len 报文长度
         */ 
        void SetReqPkg(const void* pkg, int pkg_len) {
            _msg_buff.assign((char*)pkg, pkg_len);
        };
        
        const string& GetReqPkg() {
            return _msg_buff;
        };

    protected:
        CServerBase* _srvbase;              // 适配框架base
        CTCommu*     _commu;                // 适配通讯器
        unsigned     _flow;                 // 适配流ID
        int          _msg_timeout;          // 请求处理超时配置
        unsigned long long _start_time;     // 消息处理时延记录
    
        struct sockaddr_in _from_addr;      // 报文来源IP
        struct sockaddr_in _local_addr;     // 报文接收的本地IP
        struct timeval _time_rcv;           // 收包的时间戳
        string _msg_buff;                   // 报文拷贝一份, 多线程处理需要

};


}

#endif

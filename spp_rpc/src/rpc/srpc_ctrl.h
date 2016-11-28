
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
 * @filename srpc_ctrl.h
 */

#ifndef __SRPC_CTRL_H__
#define __SRPC_CTRL_H__

#include <stdint.h>
#include <string>
#include <google/protobuf/service.h>
#include "srpc_comm.h"

using namespace google::protobuf;

namespace srpc {

/**
 * @brief RPC控制定义
 */
class CRpcCtrl : public RpcController
{
public:

    // 构造与析构函数
    inline  CRpcCtrl() { m_frame_errno = 0; m_service_ret = 0;}
    virtual ~CRpcCtrl(){}

    // 设置成功
    void SetSuccess() {
        m_frame_errno = 0;
    }

    // 重置控制对象
    void Reset() {
        m_frame_errno = 0;
        m_service_ret = 0;
    }

    // 确认是否失败
    bool Failed() const {
        return (m_frame_errno != 0);
    }

    // 失败提示信息
    string ErrorText() const {
        char err[256];
        snprintf(err, sizeof(err), "error message from back-end: %s, method return: %d", errmsg(m_frame_errno), m_service_ret);
        return err;
    }

    // 设置失败字符串
    void SetFailed(const string& reason) {}

    // 设置框架错误码
    void SetFailed(int32_t err) {
        m_frame_errno = err;
    }

    // 设置业务返回值
    void SetSrvRet(int32_t ret) {
        m_service_ret = ret;
    }

    // 异步退出处理与通知 忽略
    void StartCancel() {}
    bool IsCanceled() const {return false;}
    void NotifyOnCancel(Closure* callback){}

private:

    int32_t  m_frame_errno;
    int32_t  m_service_ret;
};

}

#endif


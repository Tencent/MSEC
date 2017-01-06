
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
 * @brief service impl
 */

#ifndef __$(MODULE_FILENAME_REPLACE)_HEAD_H__
#define __$(MODULE_FILENAME_REPLACE)_HEAD_H__

#include <string>
#include "syncincl.h"
#include "srpcincl.h"
#include "msg_$(MODULE_FILENAME_REPLACE)_impl.h"

$(CODE_BEGIN)

$(SERVICE_DEFINE_BEGIN)
/**
 * @brief 默认生成的服务实现类型
 */
class CRpc$(SERVICE_REPLACE)Impl : public $(MODULE_NAMESPACE_REPLACE)::$(SERVICE_REPLACE)
{
public:

    // 构造函数及析构函数
    CRpc$(SERVICE_REPLACE)Impl() {}
    virtual ~CRpc$(SERVICE_REPLACE)Impl() {}

    $(METHOD_DEFINES)

};

$(METHOD_IMPLEMENTS)

$(SERVICE_DEFINE_END)




$(METHOD_DEFINE_BEGIN)
    // RPC方法函数定义
    virtual void $(METHOD_REPLACE)(::google::protobuf::RpcController* controller,\
                const $(REQUEST_REPLACE)* request, \
                $(RESPONSE_REPLACE)* response, \
                ::google::protobuf::Closure* done);
$(METHOD_DEFINE_END)


$(METHOD_IMPL_BEGIN)
/**
 * @brief RPC方法函数定义
 */
void CRpc$(SERVICE_REPLACE)Impl::$(METHOD_REPLACE)(::google::protobuf::RpcController* controller,
            const $(REQUEST_REPLACE)* request,
            $(RESPONSE_REPLACE)* response,
            ::google::protobuf::Closure* done)
{
    C$(SERVICE_REPLACE)Msg* msg = dynamic_cast<C$(SERVICE_REPLACE)Msg*>(controller);
    int32_t ret = msg->$(METHOD_REPLACE)(request, response);
    msg->SetSrvRet(ret);
}
$(METHOD_IMPL_END)


$(CODE_END)



#endif



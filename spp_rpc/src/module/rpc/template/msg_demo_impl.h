
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
 * @brief 自动生成的业务代码逻辑实现
 */

#ifndef __$(MODULE_FILENAME_REPLACE)_MSG_HEAD_H__
#define __$(MODULE_FILENAME_REPLACE)_MSG_HEAD_H__

#include "syncincl.h"
#include "srpcincl.h"
#include "$(PROTO_FILE).pb.h"

using namespace $(MODULE_NAMESPACE_REPLACE);

$(CODE_BEGIN)

$(MSG_DEFINE_BEGIN)
/**
 * @brief 默认生成的服务实现类型
 * @info  [注意] 不建议业务修改该类的实现，以方便后续SRPC框架更新或者业务新加方法时可直接覆盖该文件
 */
class C$(SERVICE_REPLACE)Msg : public CRpcMsgBase
{
public:

    // 构造函数及析构函数定义
    C$(SERVICE_REPLACE)Msg() {}
    virtual ~C$(SERVICE_REPLACE)Msg() {}

    // 克隆方法定义
    CRpcMsgBase* New() {
        return new C$(SERVICE_REPLACE)Msg;
    }

    $(METHOD_DEFINES)
};
$(MSG_DEFINE_END)

$(METHOD_DEFINE_BEGIN)
    /**
     * @brief  自动生成的业务方法实现接口
     * @param  request  [入参]业务请求报文
     *         response [出参]业务回复报文
     * @return 框架会将返回值作为执行结果传给客户端
     */
    virtual int $(METHOD_REPLACE)(const $(REQUEST_REPLACE)* request, $(RESPONSE_REPLACE)* response);
$(METHOD_DEFINE_END)

$(CODE_END)

#endif



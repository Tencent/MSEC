
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

#include "syncincl.h"
#include "srpcincl.h"
#include "msg_$(MODULE_FILENAME_REPLACE)_impl.h"


$(CODE_BEGIN)

$(METHOD_IMPL_BEGIN)
/**
 * @brief  自动生成的业务方法实现接口
 * @param  request  [入参]业务请求报文
 *         response [出参]业务回复报文
 * @return 框架会将返回值作为执行结果传给客户端
 */
int C$(SERVICE_REPLACE)Msg::$(METHOD_REPLACE)(const $(REQUEST_REPLACE)* request, $(RESPONSE_REPLACE)* response)
{    
    /**
     * TODO 业务逻辑实现，request/response为业务业务定义的protobuf协议格式
     *      业务可使用框架自带的监控系统 ATTR_REPORT("test"), 详见monitor.h
     *      业务可使用框架自带的日志系统 NGLOG_DEBUG("test")，详见srpc_log.h
     */

    return 0;
}
$(METHOD_IMPL_END)

$(CODE_END)




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
 *  @brief 客户端示例
 */
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "srpcincl.h"
#include "$(PROTO_FILE).pb.h"

using namespace $(MODULE_NAMESPACE_REPLACE);
using namespace srpc;
using namespace std;

/**
 * @brief 采用微线程框架实现的简单客户端例子
 */
int main(int argc, char* argv[])
{
    // 初始化微线程框架
    if (!mt_init_frame())
    {
        cout << "Init frame failed, quit" << endl;
        return -1;
    }

    // 定义目标服务与请求消息
    CRpcUdpChannel channel("127.0.0.1:7963");
    $(REQUEST_REPLACE) request;
    // TODO:设置请求报文格式
    
    // 执行RPC调用
    $(SERVICE_REPLACE)::Stub stub(&channel);
    CRpcCtrl ctrl;
    $(RESPONSE_REPLACE) response;
    stub.$(METHOD_REPLACE)(&ctrl, &request, &response, NULL);

    // 检查结果
    if (!ctrl.Failed())
    {
        cout << "Received response OK!" << response.DebugString() << endl;
    }
    else
    {
        cout << ctrl.ErrorText() << endl;
    }

    return 0;

}




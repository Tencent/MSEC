
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


$(DEMO_BEGIN)

<?php

require_once 'pb4php/message/pb_message.php';   // pb4php文件
require_once 'pb_proto_$(PROTO_REPLACE).php';   // 自动生成的pb文件
require_once 'call_service.php';                // callmethod文件

/**
 * @brief 客户端调用示例
 */

$(CODE_BEGIN)

$(METHOD_BEGIN)

/**
 * @brief $(METHOD_REPLACE)测试示例
 */
$req = new $(REQUEST_REPLACE)();

// TODO: 设置请求报文字段

// 序列化报文
$req_str = $req->serializeToString();

// 调用callmethod方法
$rsp_str = callmethod("127.0.0.1:7963@udp", "$(MODULE_REPLACE).$(SERVICE_REPLACE).$(METHOD_REPLACE)", $req_str, 2000);

if ($rsp_str['errmsg'] != 'Success')
{
    exit('call service failed: '.$rsp_str['errmsg']);
}

$rsp = new $(RESPONSE_REPLACE)();

// 反序列化回复报文
$rsp->ParseFromString($rsp_str['rsp']);

var_dump($rsp);



$(METHOD_END)

$(CODE_END)

?>


$(DEMO_END)



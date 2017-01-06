
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

/**
 * @brief 自动生成的业务代码逻辑实现
 * PHP版本实现部分
 */

require_once 'pb4php/message/pb_message.php';   // pb4php文件
require_once 'pb_proto_$(PROTO_REPLACE).php';   // 自动生成的pb文件
require_once 'call_service.php';                // callmethod文件

class $(SERVICE_REPLACE){

    $(CODE_BEGIN)

	$(METHOD_BEGIN)
    /**
     * @brief  自动生成的业务方法实现接口
     * @param  request  [入参]业务请求报文，可能是序列化后的protobuf或者json报文
     *         is_json  [入参]是否json报文
     * @return 业务回复报文，pb序列化后的报体
     */
    public function $(METHOD_REPLACE)($request, $is_json)
    {
        /**
         * 1. json格式报文处理
         */

        if ($is_json)
        {
            // TODO: json格式报文
            // 1. $request为json格式字符串
            // 2. 需要返回json格式字符串，而非json对象
        }


        /**
         * 2. protobuf格式报文处理
         */

        /* 自动生成部分，反序列化请求包体 */
        $req = new $(REQUEST_REPLACE)();
        $req->ParseFromString($request);
        $rsp = new $(RESPONSE_REPLACE)(); 

        // TODO: 业务逻辑实现

        /* 序列化回复包体 */
        $response = $rsp->serializeToString();
        return $response; 
    }
	
    $(METHOD_END)

    $(CODE_END)
}

?>

$(DEMO_END)

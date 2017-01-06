
#
# Tencent is pleased to support the open source community by making MSEC available.
#
# Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
#
# Licensed under the GNU General Public License, Version 2.0 (the "License"); 
# you may not use this file except in compliance with the License. You may 
# obtain a copy of the License at
#
#     https://opensource.org/licenses/GPL-2.0
#
# Unless required by applicable law or agreed to in writing, software distributed under the 
# License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
# either express or implied. See the License for the specific language governing permissions
# and limitations under the License.
#



$(DEMO_BEGIN)
# -*- coding: utf-8 -*-
import $(PROTO_REPLACE)_pb2

#
#  @brief 自动生成的业务代码逻辑实现
#  Python版本实现部分
#

class $(SERVICE_REPLACE):

    $(CODE_BEGIN)
    
    $(METHOD_BEGIN)
    # @brief  自动生成的业务方法实现接口
    # @param  request  [入参]业务请求报文，非pb格式，需要转换成pb
    # @return 业务回复报文，pb序列化后的报体
    def $(METHOD_REPLACE)(self, req_data, is_json):
        # json协议处理
        if is_json:
            # TODO: 业务逻辑实现
            return req_data
            
        # 自动生成部分，反序列化请求包体
        request = $(PROTO_REPLACE)_pb2.$(REQUEST_REPLACE)()
        request.ParseFromString(req_data)
        response = $(PROTO_REPLACE)_pb2.$(RESPONSE_REPLACE)()
        
        # TODO: 业务逻辑实现
        
        # 序列化回复包体
        return response.SerializeToString()

    $(METHOD_END)

    $(CODE_END)

$(DEMO_END)

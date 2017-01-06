#!/usr/bin/env python
#coding:utf-8

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
from msec_impl import *

#
# @brief 客户端调用示例
#

$(CODE_BEGIN)

$(METHOD_BEGIN)

#
# @brief $(METHOD_REPLACE)测试示例
#
request = $(PROTO_REPLACE)_pb2.$(REQUEST_REPLACE)()

# TODO: 设置请求报文字段


# 调用callmethod方法
ret_dict = CallMethod('127.0.0.1:7963@tcp', '$(MODULE_REPLACE).$(SERVICE_REPLACE).$(METHOD_REPLACE)', request, 1000)
if (ret_dict['ret'] == 0):
    response = $(PROTO_REPLACE)_pb2.$(RESPONSE_REPLACE)()
    response.ParseFromString(ret_dict['response'])
    print response
else:
    print 'error: %d %s' % (ret_dict['ret'], ret_dict['errmsg'])


$(METHOD_END)

$(CODE_END)

$(DEMO_END)



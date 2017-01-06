
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
# -*- coding: utf-8 -*-

import traceback

def init(config):
    print "service::init"
    return 0

def fini():
    print "service::fini"
    return 0

def loop():
    return 0

def process(full_method_name, req_data, is_json):
    method_arr = full_method_name.split('.')
    errmsg = ''
    if (len(method_arr) < 3):
        errmsg = "Invalid py method name: " + full_method_name
        print errmsg
        return (-6, errmsg)

    module_name = '_'.join(method_arr[0:-2])
    class_name = method_arr[-2]
    method_name = method_arr[-1]
    
    try:
        module = __import__(module_name)
        my_class = getattr(module, class_name)
    except ImportError, e:
        errmsg = "ImportError " + module_name + ":" + e.message
        print errmsg
        return (-6, errmsg)
    except AttributeError, e:
        errmsg = "AttributeError " + class_name + " in " + module_name + ".py:"  + e.message
        print errmsg
        return (-6, errmsg)
    
    try:
        serv_obj = my_class()
    except Exception, e:
        errmsg = "Create object failed: " + class_name + " in " + module_name + ".py " + e.message
        print errmsg
        return (-6, errmsg)
    
    try:
        rsp_data = getattr(serv_obj, method_name)(req_data, is_json)
        return (0, rsp_data)
    except AttributeError, e:
        errmsg = "AttributeError " + method_name + " for class " + class_name + " in " + module_name + ".py:" + e.message
        print errmsg
        return (-6, errmsg)
    except Exception, e:
        errmsg = "Py exec function (%s.%s) failed: %s" %( class_name, method_name, e.message )
        print errmsg
        print 'invoke method failed.'
        print '%s' % traceback.format_exc()

    return (-21, errmsg)

if __name__ == "__main__":
    import echo_pb2
    request = echo_pb2.EchoRequest()
    request.part1 = 'hello'
    request.part2 = 'from python'
    request.number = 15
    req_data = request.SerializeToString()

    ext_info = [1,]
    (ret, rsp_data) = process('echosample.EchoService.echoCall', req_data, ext_info)

    if ret == 0: 
        response = echo_pb2.EchoResponse()
        response.ParseFromString(rsp_data)
        print response
    else:
        print 'error ret: ' + str(ret)
        print 'error msg: ' + rsp_data


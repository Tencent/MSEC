
# -*- coding: utf-8 -*-
# 

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

import random
import time

rpc_report=None
rpc_set=None
msec_attr_report=None
msec_attr_set=None
nlb_getroute=None
nlb_updateroute=None
srpc_comm_serialize=None
srpc_comm_deserialize=None
srpc_comm_check_pkg=None
srpc_comm_sendrcv=None
msec_log_set_option_str=None
msec_log_error=None
msec_log_info=None
msec_log_debug=None
msec_log_fatal=None
msec_get_config=None

try:
    srpc_comm_module = __import__('srpc_comm_py')
    srpc_comm_serialize = getattr(srpc_comm_module, 'srpc_serialize')
    srpc_comm_deserialize = getattr(srpc_comm_module, 'srpc_deserialize')
    srpc_comm_check_pkg = getattr(srpc_comm_module, 'srpc_check_pkg')
    srpc_comm_sendrcv = getattr(srpc_comm_module, 'srpc_sendrcv')
except ImportError, e:
    pass
except AttributeError, e:
    pass

try:
    monitor_module = __import__('monitor_py')
    rpc_report = getattr(monitor_module, 'rpc_report')
    rpc_set = getattr(monitor_module, 'rpc_set')
    msec_attr_report = getattr(monitor_module, 'attr_report')
    msec_attr_set = getattr(monitor_module, 'attr_set')
except ImportError, e:
    pass
except AttributeError, e:
    pass

try:
    nlb_module = __import__('nlb_py')
    nlb_getroute = getattr(nlb_module, 'getroutebyname')
    nlb_updateroute = getattr(nlb_module, 'updateroute')
except ImportError, e:
    pass
except AttributeError, e:
    pass    

try:
    nglog_module = __import__('log_py')
    msec_log_set_option_str = getattr(nglog_module, 'nglog_set_option_str')
    msec_log_error = getattr(nglog_module, 'nglog_error')
    msec_log_info = getattr(nglog_module, 'nglog_info')
    msec_log_debug = getattr(nglog_module, 'nglog_debug')
    msec_log_fatal = getattr(nglog_module, 'nglog_fatal')
    msec_get_config = getattr(nglog_module, 'get_config')
except ImportError, e:
    pass
except AttributeError, e:
    pass

#
# @brief 设置日志选项接口
# @parm  key   选项key
# @      val   选项value
#
def log_set_option(key, val):
    '''
    set log option
    '''
    global msec_log_set_option_str
    val_str = str(val)
    key_str = str(key)
    if msec_log_set_option_str != None:
        msec_log_set_option_str(key_str, val_str)
    pass

#
# @brief 打印日志接口
# @param log  打印的日志字符串
#
def log_error(log):
    '''
    print error level log
    '''
    global msec_log_error
    if msec_log_error != None:
        msec_log_error(log)
    pass

def log_info(log):
    '''
    print info level log
    '''
    global msec_log_info
    if msec_log_info != None:
        msec_log_info(log)
    pass

def log_debug(log):
    '''
    print debug level log
    '''
    global msec_log_debug
    if msec_log_debug != None:
        msec_log_debug(log)
    pass

def log_fatal(log):
    '''
    print fatal level log
    '''
    global msec_log_fatal
    if msec_log_fatal != None:
        msec_log_fatal(log)
    pass

#
# @brief  读取ini配置接口
# @param  session     session名字
# @       key         key名字
# @       filename    配置文件路径
# @return None		  没有相关配置，或者配置文件不存在
# @		  !=None	  字符串类型数据
#
def get_config(session, key, filename='../etc/config.ini'):
    '''
    get config
    '''
    global msec_get_config
    if msec_get_config != None:
        return msec_get_config(filename, session, key)
    return None

def monitor_add(attr, value=1):
    '''
    add rpc monitor report
    '''
    global msec_rpc_report
    if rpc_report != None:
        rpc_report(attr, value)

def monitor_set(attr, value=1):
    '''
    set rpc monitor report
    '''
    global rpc_set
    if rpc_set != None:
        rpc_set(attr, value)

#
# @brief 监控上报累加值
# @param attr   上报属性值
# @      value  累加值，默认1
#
def attr_report(attr, value=1):
    global msec_attr_report
    if msec_attr_report != None:
        msec_attr_report(attr, value)

#
# @brief 监控上报即时值
# @param attr   上报属性值
# @      value  即时值，默认1
#
def attr_set(attr, value=1):
    global msec_attr_set
    if msec_attr_report != None:
        msec_attr_set(attr, value)

#
# @brief SRPC序列化报文接口
# @param methodname     方法名，"echo.EchoService.EchoTest"
# @      body           包体，pb序列化后的包体
# @      seq            本次收发包的序列号，用于判断是否串包
# @return =None         打包错误
#         !=None        打包完后的二进制数据
#
def srpc_serialize(methodname, body, seq):
    global srpc_comm_serialize
    if srpc_comm_serialize == None:
        return None
    return srpc_comm_serialize(methodname, body, seq)

#
# @brief SRPC反序列化报文接口
# @param pkg         报文包体
# @return 返回值为词典类型，需要判断 ret['errmsg'] == 'success'来判断是否成功
# @       如果成功，返回值为{'errmsg':'success', 'seq':100, 'body': 'hello world'}
#
def srpc_deserialize(pkg):
    global srpc_comm_deserialize
    if srpc_comm_deserialize == None:
        return {'errmsg': 'load srpc_comm_py module failed'}
    return srpc_comm_deserialize(pkg)

#
# @brief SRPC检查报文是否完整接口
# @param pkg    报文包体
# @return <0    非法报文
# @       ==0   报文不完整
# @       >0    报文完整，长度为返回值
#
def srpc_check_pkg(pkg):
    global srpc_comm_check_pkg
    if srpc_comm_check_pkg == None:
        return -2
    return srpc_comm_check_pkg(pkg)

def srpc_sendrcv(host, send_pkg, timeout):
    global srpc_comm_sendrcv
    if srpc_comm_sendrcv == None:
        return None

    return srpc_comm_sendrcv(host, send_pkg, timeout)

#
# @brief 获取路由接口
# @param service_name   业务名，msec中通过web_console设置的两级业务名，如"Login.ptlogin"
# @return =Null         获取路由失败
# @       !=Null        路由信息，返回类型为词典,如:{'ip':'10.0.0.1'; 'port': 7963; 'type': 'tcp'}
#
def getroute(service_name):
    global nlb_getroute
    server_route = {}
    pos1 = service_name.find(':')
    pos2=  service_name.find('@')

    try:
        if (pos1 > 0 and pos2 > 0):
            server_route['ip'] = service_name[0:pos1]
            server_route['port'] = int(service_name[1+pos1:pos2])
            if (service_name[1+pos2:] == 'udp'):
                server_route['type'] = 'udp'
            else:
                server_route['type'] = 'tcp'
    except Exception, e:
        server_route.clear()

    if server_route:
        return server_route

    if nlb_getroute != None:
        server_route = nlb_getroute(service_name)

    return server_route

#
# @brief  更新路由信息
# @param service_name       业务名，msec中通过web_console设置的两级业务名，如"Login.ptlogin"
# @      ip                 IP地址
# @      failed             业务网络调用是否失败
# @      cost               如果成功，需要上报时延，暂时没有对时延上报做路由策略，所以，可以设置为0
#
def updateroute(service_name, ip, failed, cost):
    global nlb_updateroute
    pos = service_name.find(':')
    if (pos > 0):
        return

    if nlb_updateroute != None:
        updateroute(service_name, ip, failed, cost)
    pass

#
# @brief 调用其它业务
# @param service_name  业务名，用作寻址，可传入IP地址或业务名("login.web" or "10.0.0.1:1000@udp")
#        method_name   方法名，pb规定的业务名，带namespace("echo.EchoService.EchoTest")
#        request       pb格式请求包
#        timeout       超时时间，单位毫秒
#  @return 回复包体+错误信息的数组
#  @notice 1. 需要首先判断返回值的 return['ret']是否等于0,0表示成功，其它表示失败，失败可以查看return['errmsg']
#          2. 如果等于0，则可以取出序列化后的报文return['response']
#
def CallMethod(service_name, method_name, request, timeout):
    ret_dict = {}

    monitor_add('call [' + service_name + ']', 1)
    
    seq = random.randint(10000, 2000000000)
    body_data = request.SerializeToString()
    request_data = srpc_serialize(method_name, body_data, seq)
    if request_data == None:
        monitor_add('call [' + service_name + '] failed: serialize failed', 1)
        ret_dict = {'ret':-3, 'errmsg':'serialize failed'}
        return ret_dict

    response_data = srpc_sendrcv(service_name, request_data, timeout)
    if response_data == None:
        monitor_add('call [' + service_name + '] failed: sendrecv failed', 1)
        ret_dict = {'ret':-13, 'errmsg': 'sendrecv failed'}
        return ret_dict

    rsp_data = {}
    rsp_data = srpc_deserialize(response_data)
    if rsp_data['errmsg'] != 'success':
        monitor_add('call [' + service_name + '] failed: deserialize failed', 1)
        ret_dict = {'ret':-3, 'errmsg': 'deserialize failed'}
        return ret_dict

    if rsp_data['seq'] != seq:
        monitor_add('call [' + service_name + '] failed: invalid sequence', 1)
        ret_dict = {'ret': -3, 'errmsg': 'invalid sequence'}
        return ret_dict

    monitor_add('call [' + service_name + '] success', 1)
    ret_dict = {'ret': 0, 'response': rsp_data['body']}
    return ret_dict

if __name__ == "__main__":
    #set request
    import echo_pb2
    request = echo_pb2.EchoRequest()
    request.part1 = 'hello'
    request.part2 = 'from python'
    request.number = 15

    ret_dict = CallMethod('10.104.104.22:7966@tcp', 'echosample.choService.echoCall', request, 300)
    if (ret_dict['ret'] == 0):
        response = echo_pb2.EchoResponse()
        response.ParseFromString(ret_dict['response'])
        print response
    else:
        print 'error: %d %s' % (ret_dict['ret'], ret_dict['errmsg'])

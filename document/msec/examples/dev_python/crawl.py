
# -*- coding: utf-8 -*-
import msec_pb2
import json
from msec_impl import *
import protobuf_json
import socket
import select
import time

#
#  @brief 自动生成的业务代码逻辑实现
#  Python版本实现部分
#

class CrawlService:

    
    # @brief  自动生成的业务方法实现接口
    # @param  request  [入参]业务请求报文，非pb格式，需要转换成pb
    # @return 业务回复报文，pb序列化后的报体
    def GetMP3List(self, req_data, is_json):
        # 自动生成部分，反序列化请求包体
        request = msec_pb2.GetMP3ListRequest()
        response = msec_pb2.GetMP3ListResponse()
        # json协议处理
        if is_json:
            req_json = json.loads(req_data)
            request = protobuf_json.json2pb(request, req_json)
        else:
            request.ParseFromString(req_data)

        # TODO: 业务逻辑实现
        log_info("GetMP3List start")
        monitor_add('GetMP3List entry')
        if request.type != "special" and request.type != "standard":
            response.status = 100
            response.msg = "type field invalid"

        json_req = {"handleClass":"com.bison.GetMP3List",  "requestBody": {"type": request.type} }
        json_ret = self.callmethod_tcp( "Jsoup.jsoup", json_req, self.callback, 10.0)
        if json_ret["ret"] != 0:
            response.status = 100
            response.msg = json_ret["errmsg"]
        else:
            if json_ret["data"]["status"] != 0:
                response.status = 100
                response.msg = "jsoup returns "+str(json_ret["data"]["status"])
            else:
                response.status = 0
                response.msg = "success"
                log_info("GetMP3List successfully")
                monitor_add("GetMP3List succ")
                for mp3 in json_ret["data"]["mp3s"]:
                    one_mp3 = response.mp3s.add()
                    one_mp3.url = mp3["url"]
                    one_mp3.title = mp3["title"]

        # 序列化回复包体
        if is_json:
            return json.dumps(protobuf_json.pb2json(response))
        else:
            return response.SerializeToString()

    def callmethod_tcp(self, servicename, request, callback, timeout):
        ret_dict = {}
        start = time.time()
        route = getroute(servicename)
        if not route:
            ret_dict = {'ret': -1, 'errmsg': 'getroute failed'}
            return ret_dict

        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((route["ip"], route["port"]))
        sock.setblocking(0)

        json_req = json.dumps(request)
        sock.sendall("%-10d%s"%(len(json_req), json_req))

        #check timeout
        current = time.time()
        if current - start >= timeout:
            ret_dict = {'ret': -1, 'errmsg': 'timeout'}
            sock.close()
            return ret_dict

        recvdata = ""
        t = timeout - current + start
        while t>0:
            ready = select.select([sock], [], [], t)
            if ready[0]:
                data = sock.recv(4096)
                if data == "":
                    ret_dict = {'ret': -1, 'errmsg': 'peer closed the connection'}
                    sock.close()
                    return ret_dict
                recvdata += data
            else:
                ret_dict = {'ret': -1, 'errmsg': 'select timeout'}
                sock.close()
                return ret_dict

            ret = callback(recvdata)
            if ret != 0: #ret == 0, package not complete, continue
                if ret > 0: #package complete, length is ret
                    if len(recvdata) < ret:
                        ret_dict = {'ret': -1, 'errmsg': 'callback function error'}
                        sock.close()
                        return ret_dict
                    break
                else: #package format invalid
                    ret_dict = {'ret': -1, 'errmsg': 'response package format error'}
                    sock.close()
                    return ret_dict

            current = time.time()
            t = timeout - current + start

        if t>0:
            ret_dict = {'ret': 0, 'data':json.loads(recvdata[10:ret])}
        else:
            ret_dict = {'ret': -1, 'data': "recv timeout"}
        sock.close()
        return ret_dict

    def callback(self, response):
        resp_len = len(response)
        if resp_len < 10:
            return 0
        pkg_len = int(response[:10])
        if pkg_len + 10 <= resp_len:
            return pkg_len + 10
        return 0
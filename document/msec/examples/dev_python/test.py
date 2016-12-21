import VOA_py_MainLogic_pb2
from msec_impl import *
import time
import socket
import select
import random
import sys

if __name__ == '__main__':
    start = time.time()
    host = ("10.104.104.22",7903)   #replace with actual host
    timeout = 10.0      #timeout 10s
    method_name = "MainLogic.MainLogicService.GetTitles"

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect(host)
    sock.setblocking(0)

    request = VOA_py_MainLogic_pb2.GetTitlesRequest()
    response = VOA_py_MainLogic_pb2.GetTitlesResponse()

    request.type = "special"
    seq = random.randint(10000, 2000000000)
    body_data = request.SerializeToString()

    request_data = srpc_serialize(method_name, body_data, seq)
    if request_data == None:
        ret_dict = {'ret':-1, 'errmsg':'serialize failed'}
        sock.close()
        print ret_dict
        sys.exit(1)
    sock.sendall(request_data)
    # check timeout
    current = time.time()
    if current - start >= timeout:
        ret_dict = {'ret': -1, 'errmsg': 'timeout'}
        sock.close()
        print ret_dict
        sys.exit(1)

    recvdata = ""
    t = timeout - current + start
    while t > 0:
        ready = select.select([sock], [], [], t)
        if ready[0]:
            data = sock.recv(4096)
            if data == "":
                ret_dict = {'ret': -1, 'errmsg': 'peer closed the connection'}
                sock.close()
                print ret_dict
                sys.exit(1)
            recvdata += data
        else:
            ret_dict = {'ret': -1, 'errmsg': 'select timeout'}
            sock.close()
            print ret_dict
            sys.exit(1)

        ret = srpc_check_pkg(recvdata)
        if ret != 0:  # ret == 0, package not complete, continue
            if ret > 0:  # package complete, length is ret
                if len(recvdata) < ret:
                    ret_dict = {'ret': -1, 'errmsg': 'check package error'}
                    sock.close()
                    print ret_dict
                    sys.exit(1)
                break
            else:  # package format invalid
                ret_dict = {'ret': -1, 'errmsg': 'response package format error'}
                sock.close()
                print ret_dict
                sys.exit(1)

        current = time.time()
        t = timeout - current + start

    if t>0:
        response_ret = srpc_deserialize(recvdata)
        if response_ret['errmsg'] != 'success':
            ret_dict = {'ret': -1, 'errmsg': 'deserialize failed'}
            sock.close()
            print ret_dict
            sys.exit(1)
        if response_ret['seq'] != seq:
            ret_dict = {'ret': -1, 'errmsg': 'invalid sequence'}
            sock.close()
            print ret_dict
            sys.exit(1)
        response.ParseFromString(response_ret['body'])
        print response
        sock.close()
    else:
        ret_dict = {'ret': -1, 'errmsg': 'recv timeout'}
        sock.close()
        print ret_dict
        sys.exit(1)

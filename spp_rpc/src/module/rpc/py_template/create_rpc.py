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

import sys
import os
import re


## parse .pb.h file, get module/service/method/ info
def ParseProtoFile(filename, template_path):
    """ Just parse sercice proto file 
    """
    dict_pb = {}
    
    ## get file name
    proto_begin = filename.rfind('/') + 1
    dict_pb["proto"] = filename[proto_begin:filename.rfind('.')]

    ## 1. delete //.. /*.. */
    file_content = open(filename, 'rb').read()
    pattern = re.compile(r'//.*?$', re.S | re.M)
    file_content = pattern.sub("", file_content)
    pattern = re.compile(r'/\*.*?\*/', re.S | re.M)
    file_content = pattern.sub("", file_content)
    #open('tmp.data', 'w').write(file_content)    

    ## 2. get module name: package XXX;
    pattern = re.compile(r'package\s+(\S+);', re.M)
    module_list = pattern.findall(file_content)
    if not module_list:
        print filename + ' no module config'
        sys.exit(-1)
    dict_pb['module'] = module_list[0]
    dict_pb['module_filename'] = module_list[0].replace('.', '_');

    ## 3. get service module list
    pattern = re.compile(r'(service\s+(\w+).*?{.*?})', re.S | re.M)
    service_list =  pattern.findall(file_content)
    if not service_list:
        print filename + ' no service config'
        sys.exit(-1)
    #print service_list
    
    ## 4. scan all service methods
    services = {}
    for item in service_list:
        method_list = []
        #print item[1]
        p = re.compile(r'rpc\s+(\w+)\s*\((\w+)\)\s+returns\s+\((\w+)\)', re.S | re.M)
        mlist = p.findall(item[0])
        if not mlist:
            print filename + ' no method config'
            sys.exit(-1)
        for m in mlist:
            method_list.append((m[0], m[1], m[2]))
        services[item[1]] = method_list
    dict_pb['services'] = services   
    #print dict_pb

    ## 5.create proto file for php
    os.system("chmod a+x %s/../../lib/protoc" % (template_path));
    
    return dict_pb  

## create server codes
def GenerateRpcServer(pb, output_path):
    """ """
    if not pb['module']:
        return

    template_path = pb["frame_path"] + "/rpc/py_template"
    demo_path = template_path + '/demo.py'

    for (key,value) in pb['services'].items():
        filename = output_path + "%s.py" % pb['module_filename']
        if not os.path.isfile(filename):
            ProcessDemoPy(pb['module'], pb['proto'], key, value, demo_path, filename)

    if os.system("%s/lib/protoc -I%s --python_out=%s %s/%s.proto" % (pb["frame_path"], output_path, output_path, output_path, pb["proto"])) != 0:
        print 'invalid protoc file'
        sys.exit(-1)
    os.system("cp %s/entry.py %s" % (template_path, output_path))
    os.system("cp -rf %s/build.sh %s" % (template_path, output_path))
    os.system("cp -rf %s/msec_impl.py %s" %(template_path, output_path))
    os.system("cp -rf %s/srpc.proto %s" % (template_path, output_path))
    pass

def GenerateRpcClient(pb, output_path):
    """ """
    if not pb['module']:
        return

    template_path = pb["frame_path"] + "/rpc/py_template"
    demo_path = template_path + '/demo_client.py'

    for (key,value) in pb['services'].items():
        filename = output_path + "%s_client.py" % pb['module_filename']
        if not os.path.isfile(filename):
            ProcessDemoPy(pb['module'], pb['proto'], key, value, demo_path, filename)

    if os.system("%s/lib/protoc -I%s --python_out=%s %s/%s.proto" % (pb["frame_path"], output_path, output_path, output_path, pb["proto"])) != 0:
        print 'invalid protoc file'
        sys.exit(-1)
    os.system("cp -rf %s/lib %s -rf" % (template_path, output_path))
    os.system("cp -rf %s/msec_impl.py %s" %(template_path, output_path))
    os.system("cp -rf %s/nlb_py.so %s" %(template_path, output_path))
    os.system("cp -rf %s/srpc_comm_py.so %s" %(template_path, output_path))
    os.system("cp -rf %s/libsrpc_proto_py_c.so %s" %(template_path, output_path))
    os.system("cp -rf %s/srpc.proto %s" % (template_path, output_path))

    os.system("cp -rf %s/python %s" %(template_path, output_path))
    os.system("cp -rf %s/python2.7 %s" %(template_path, output_path))
    os.system("chmod a+x %s/python2.7" % (output_path))
    os.system("cp -rf %s/README.client %s" %(template_path, output_path))

    pass

def ProcessDemoPy(module, proto, service, methods, inputfile, outputfile):
    """ Just create demo py file"""
    file_content = open(inputfile, 'rb').read()
    service_code = ""
    file_content = re.findall(r'\$\(DEMO_BEGIN\)(.*)\$\(DEMO_END\)', file_content, re.S | re.M)[0]
    file_content = re.sub(r'\$\(PROTO_REPLACE\)', proto, file_content, re.S | re.M)
    file_content = re.sub(r'\$\(SERVICE_REPLACE\)', service, file_content, re.S | re.M)

    method_code = ""
    method_def = re.findall(r'\$\(METHOD_BEGIN\)(.*)\$\(METHOD_END\)', file_content, re.S | re.M)[0]
    for method in methods:
        one_method_def = ""
        one_method_def = re.sub(r'\$\(METHOD_REPLACE\)', method[0], method_def, re.S | re.M)
        one_method_def = re.sub(r'\$\(REQUEST_REPLACE\)', method[1], one_method_def, re.S | re.M)
        one_method_def = re.sub(r'\$\(RESPONSE_REPLACE\)', method[2], one_method_def, re.S | re.M)
        one_method_def = re.sub(r'\$\(SERVICE_REPLACE\)', service, one_method_def, re.S | re.M)
        one_method_def = re.sub(r'\$\(MODULE_REPLACE\)', module, one_method_def, re.S | re.M)
        one_method_def = re.sub(r'\$\(PROTO_REPLACE\)', proto, one_method_def, re.S | re.M)
        method_code += one_method_def
    #print method_code
    code_block = re.findall(r'\$\(CODE_BEGIN\).*?\$\(CODE_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(code_block, method_code)
    #print file_content
    #return

    open(outputfile, 'w').write(file_content)
    return

## main process

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print "Usage: create_rpc.py proto.filename frame_path output_path "
        sys.exit(-1)
    
    ## parse proto file
    frame_path = os.path.abspath(sys.argv[2])
    template_path = frame_path + "/rpc/py_template"
    pb = ParseProtoFile(sys.argv[1], template_path)
    pb["frame_path"] = frame_path

    ## check output path
    output_path = os.path.abspath(sys.argv[3]) + '/' + pb["module_filename"]
    if os.path.exists(output_path):
        os.system("rm %s -rf" % (output_path))
    os.makedirs(output_path)

    ## check output svr path
    server_path = output_path + '/' + pb["module_filename"] + "_server/"
    if not os.path.exists(server_path):
        os.makedirs(server_path)
    os.system("cp %s %s" % (sys.argv[1], server_path))

    ## client path
    client_path = output_path + '/' + pb["module_filename"] + "_client/"
    if not os.path.exists(client_path):
        os.makedirs(client_path)
    os.system("cp %s %s" % (sys.argv[1], client_path))

    ## create server code
    GenerateRpcServer(pb, server_path)
    
    ## create client code
    GenerateRpcClient(pb, client_path) 

    print("%s generate succ" % server_path)

    pass
    
    


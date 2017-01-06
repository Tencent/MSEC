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
def ParseProtoFile(filename):
    """ Just parse sercice proto file 
    """
    dict_pb = {}
    
    ## get file name
    proto_begin = filename.rfind('/') + 1
    dict_pb["proto"] = filename[proto_begin:filename.rfind('.')]
    #print dict_pb["proto"]

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
        print filename + ' no package config'
        sys.exit(-1)
    dict_pb['module'] = module_list[0]
    dict_pb['module_namespace'] = module_list[0].replace('.', '::');
    dict_pb['module_filename'] = module_list[0].replace('.', '_');
    print dict_pb

    ## 3. get service module list
    pattern = re.compile(r'(service\s+(\w+).*?{.*?})', re.S | re.M)
    service_list =  pattern.findall(file_content)
    if not service_list:
        print filename + ' no service config'
        sys.exit(-1)
    
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
            method_list.append((item[1], m[0], m[1], m[2]))
        services[item[1]] = method_list
    dict_pb['services'] = services
    
    return dict_pb  


## process code replace
def ReplaceCode(org_str, filename, namespace, service, method, request, response):
    """ """
    org_str = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', filename, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)',namespace, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(SERVICE_REPLACE\)', service, org_str, re.S | re.M) 
    org_str = re.sub(r'\$\(METHOD_REPLACE\)', method, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(REQUEST_REPLACE\)', request, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(RESPONSE_REPLACE\)', response, org_str, re.S | re.M)
    return org_str


## process service impl file
def ProcessService(dict_pb, inputfile, outputfile):
    """ Just create service file"""
    if not dict_pb['module']:
        return
    filename  = dict_pb['module_filename']
    namespace = dict_pb['module_namespace']
    file_content  = open(inputfile, 'rb').read()
    #print file_content
    mth_def = re.findall(r'\$\(METHOD_DEFINE_BEGIN\)(.*)\$\(METHOD_DEFINE_END\)', file_content, re.S | re.M)[0]
    mth_imp = re.findall(r'\$\(METHOD_IMPL_BEGIN\)(.*)\$\(METHOD_IMPL_END\)', file_content, re.S | re.M)[0]
    service_def = re.findall(r'\$\(SERVICE_DEFINE_BEGIN\)(.*)\$\(SERVICE_DEFINE_END\)', file_content, re.S | re.M)[0]
    #print service_def
    
    service_code = ""
    for (key,value) in dict_pb['services'].items():
        method_def = ""
        method_imp = ""
        for item in value:
            method_def = method_def + ReplaceCode(mth_def, filename, namespace, item[0], item[1], item[2], item[3])
            method_imp = method_imp + ReplaceCode(mth_imp, filename, namespace, item[0], item[1], item[2], item[3])
        service_one = re.sub(r'\$\(METHOD_DEFINES\)', method_def, service_def, re.S | re.M)
        service_one = re.sub(r'\$\(SERVICE_REPLACE\)', key, service_one, re.S | re.M)
        service_one = re.sub(r'\$\(METHOD_IMPLEMENTS\)', method_imp, service_one, re.S | re.M)
        service_code += service_one
        #print service_code
    code_block = re.findall(r'\$\(CODE_BEGIN\).*?\$\(CODE_END\)', file_content, re.S | re.M)[0]  
    file_content = file_content.replace(code_block, service_code)
    file_content = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)', namespace, file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', filename, file_content, re.S | re.M)  
    open(outputfile, 'w').write(file_content) 

## process msg head file
def ProcessMsgHead(dict_pb, inputfile, outputfile):
    """ Just create msg file"""
    if not dict_pb['module']:
        return
    filename  = dict_pb['module_filename']
    namespace = dict_pb['module_namespace']
    file_content = open(inputfile, 'rb').read()
    #print file_content
    mth_def = re.findall(r'\$\(METHOD_DEFINE_BEGIN\)(.*)\$\(METHOD_DEFINE_END\)', file_content, re.S | re.M)[0]
    service_def = re.findall(r'\$\(MSG_DEFINE_BEGIN\)(.*)\$\(MSG_DEFINE_END\)', file_content, re.S | re.M)[0]

    #print service_def
    service_code = ""
    for (key,value) in dict_pb['services'].items():
        method_def = ""
        for item in value:
            method_def = method_def + ReplaceCode(mth_def, filename, namespace, item[0], item[1], item[2], item[3])
        service_one = re.sub(r'\$\(METHOD_DEFINES\)', method_def, service_def, re.S | re.M)
        service_one = re.sub(r'\$\(SERVICE_REPLACE\)', key, service_one, re.S | re.M)
        service_code += service_one
        #print service_code
    code_block = re.findall(r'\$\(CODE_BEGIN\).*?\$\(CODE_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(code_block, service_code)
    file_content = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)', namespace, file_content, re.S | re.M)
    file_content = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', filename, file_content, re.S | re.M)
    file_content = re.sub(r'\$\(PROTO_FILE\)', dict_pb['proto'], file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)


## process msg cpp file
def ProcessMsgImpl(dict_pb, inputfile, outputfile):
    """ Just create msg file"""
    if not dict_pb['module']:
        return
    filename = dict_pb['module_filename'];
    namespace = dict_pb['module_namespace'];
    file_content = open(inputfile, 'rb').read()
    #print file_content
    mth_def = re.findall(r'\$\(METHOD_IMPL_BEGIN\)(.*)\$\(METHOD_IMPL_END\)', file_content, re.S | re.M)[0]

    #print service_def
    service_code = ""
    for (key,value) in dict_pb['services'].items():
        method_def = ""
        for item in value:
            method_def = method_def + ReplaceCode(mth_def, filename, namespace, item[0], item[1], item[2], item[3])
        service_code += method_def
        #print service_code
    code_block = re.findall(r'\$\(CODE_BEGIN\).*?\$\(CODE_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(code_block, service_code)
    file_content = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)', namespace, file_content, re.S | re.M)
    file_content = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', filename, file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)

## process frame cpp file
def ProcessFrame(dict_pb, inputfile, outputfile):
    """ Just create msg file"""
    if not dict_pb['module']:
        return
    filename = dict_pb['module_filename'];
    namespace = dict_pb['module_namespace'];
    file_content = open(inputfile, 'rb').read()
    #print file_content
    mth_def = re.findall(r'\$\(REGIST_BEGIN\)(.*)\$\(REGIST_END\)', file_content, re.S | re.M)[0]
    #print mth_def

    #print service_def
    service_code = ""
    for (key,value) in dict_pb['services'].items():
        service_code += re.sub(r'\$\(SERVICE_REPLACE\)', key, mth_def, re.S | re.M)
        #print service_code
    code_block = re.findall(r'\$\(CODE_BEGIN\).*?\$\(CODE_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(code_block, service_code)
    file_content = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', filename, file_content, re.S | re.M)
    file_content = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)', namespace, file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)
    #print file_content

## process makefile
def ProcessMakefile(dict_pb, inputfile, outputfile):
    """ """
    if not dict_pb['module']:
        return
    file_content = open(inputfile, 'rb').read()
    file_content = re.sub(r'\$\(MODULE_FILENAME_REPLACE\)', dict_pb['module_filename'], file_content, re.S | re.M)
    file_content = re.sub(r'\$\(PROTO_FILE\)', dict_pb['proto'], file_content, re.S | re.M)
    file_content = re.sub(r'\$\(FRAME_PATH\)', dict_pb['frame_path'], file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)

## create server codes
def GenerateRpcServer(pb, output_path):
    """ """
    if not pb['module']:
        return
    template_path = pb["frame_path"] + "/rpc/template/"

    ProcessService(pb, template_path + 'service_demo_impl.hpp', output_path + "service_%s_impl.hpp" % pb['module_filename'])
    ProcessMsgHead(pb, template_path + 'msg_demo_impl.h', output_path + "msg_%s_impl.h" % pb['module_filename'])
    ProcessMsgImpl(pb, template_path + 'msg_demo_impl.cpp', output_path + "msg_%s_impl.cpp" % pb['module_filename'])
    ProcessFrame(pb, template_path + 'service_demo_frm.cpp', output_path + "service_%s_frm.cpp" % pb['module_filename'])
    ProcessMakefile(pb, template_path + 'Makefile', output_path + 'Makefile')
    if os.system("make -C %s" % output_path) == -1:
        print 'make rpc server failed'
        sys.exit(-1)
    pass

## create client frame 
def ProcessClientFrame(pb, inputfile, outputfile):
    """ """
    if not pb['module']:
        return
    
    filename = pb['module_filename'];
    namespace = pb['module_namespace'];
    file_content = open(inputfile, 'rb').read()
   
    # Replace proto file
    file_content = re.sub(r'\$\(PROTO_FILE\)', pb['proto'], file_content, re.S | re.M)
    file_content = re.sub(r'\$\(MODULE_NAMESPACE_REPLACE\)', namespace, file_content, re.S | re.M)

    # Just example one first method
    for (key,value) in pb['services'].items():
        for item in value:
            file_content = ReplaceCode(file_content, filename, namespace, item[0], item[1], item[2], item[3])
            open(outputfile, 'w').write(file_content)
            return



## create client rpc code
def GenerateRpcClient(pb, output_path):
    """ """
    if not pb['module']:
        return
    template_path = pb["frame_path"] + "/rpc/template/"
    
    ProcessClientFrame(pb, template_path + 'client_demo_sync.cpp', output_path + "client_%s_sync.cpp" % pb['module_filename'])
    ProcessMakefile(pb, template_path + 'Makefile.client', output_path + 'Makefile')
    if os.system("make -C %s" % output_path) == -1:
        print 'make rpc client failed'
        sys.exit(-1)
    pass


## main process

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print "Usage: create_rpc.py proto.filename frame_path output_path"
        sys.exit(-1)
    
    ## parse proto file
    pb = ParseProtoFile(sys.argv[1])
    pb["frame_path"] = os.path.abspath(sys.argv[2])

    ## check output path
    output_path = os.path.abspath(sys.argv[3]) + '/' + pb["module_filename"]
    if os.path.exists(output_path):
        system("rm %s -rf" % (output_path))
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

    ## cp frame include path
    os.system("cp %s/include %s -rf" % (pb["frame_path"], output_path));

    ## cp rpc lib
    lib_path = output_path + "/lib"
    if not os.path.exists(lib_path):
        os.makedirs(lib_path)
    os.system("cp %s/lib/* %s -rf" % (pb["frame_path"], lib_path))
        
    ## create server code
    GenerateRpcServer(pb, server_path)

    ## create client code
    GenerateRpcClient(pb, client_path)

    pass
    
    


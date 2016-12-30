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

def UnderscoresToCamelCase(str, cap_next_letter):
    result = ""
    for i in range(0, len(str)):
        if ('a' <= str[i] and str[i] <= 'z'):
            if (cap_next_letter):
                result += str[i].upper()
            else:
                result += str[i]
            cap_next_letter = False
        elif ('A' <= str[i] and str[i] <= 'Z'):
            if (i == 0 and not cap_next_letter):
                result += str[i].lower()
            else:
                result += str[i]
            cap_next_letter = False
        elif ('0' <= str[i] and str[i] <= '9'):
            result += str[i]
            cap_next_letter = True
        else:
            cap_next_letter = True

    return result;

## parse .proto file, get module/service/method/ info
def ParseProtoFile(filename):
    """ Just parse sercice proto file 
    """
    dict_pb = {}
    
    ## get file name
    proto_begin = filename.rfind('/') + 1
    dict_pb["proto"] = UnderscoresToCamelCase(filename[proto_begin:filename.rfind('.')], True)
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
        return dict_pb
    dict_pb['pkg'] = module_list[0]
    #print dict_pb

    ## 3. get service module list
    pattern = re.compile(r'(service\s+(\w+).*?{.*?})', re.S | re.M)
    service_list =  pattern.findall(file_content)
    
    ## 4. scan all service methods
    services = {}
    for item in service_list:
        method_list = []
        #print item[1]
        p = re.compile(r'rpc\s+(\w+)\s*\((\w+)\)\s+returns\s+\((\w+)\)', re.S | re.M)
        mlist = p.findall(item[0])
        for m in mlist:
            method_list.append((item[1], TransMethodName(m[0]), m[1], m[2]))
        services[item[1]] = method_list
    dict_pb['services'] = services
   
    #print "dict_pb", dict_pb 
    return dict_pb  

def TransMethodName(method):
    methodname = method
    if (method[0].isupper()):
        methodname = method[0].lower() + method[1:]
    return methodname

## process code replace
def ReplaceCode(org_str, proto, module, service, method, request, response):
    """ """
    org_str = re.sub(r'\$\(PROTO_REPLACE\)', proto, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(METHOD_REPLACE\)', method, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(REQUEST_REPLACE\)', request, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(RESPONSE_REPLACE\)', response, org_str, re.S | re.M)
    return org_str

## process code replace
def ReplaceCode2(org_str, proto, pkg, service):
    """ """
    org_str = re.sub(r'\$\(PROTO_REPLACE\)', proto, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(PKG_REPLACE\)', pkg, org_str, re.S | re.M)
    org_str = re.sub(r'\$\(SERVICE_REPLACE\)', service, org_str, re.S | re.M) 
    return org_str


## process service impl file
def ProcessService(dict_pb, inputfile, outputfile):
    """ Just create service file"""
    if not dict_pb['pkg']:
        return
    file_content = open(inputfile, 'rb').read()
    #print file_content
    mth_imp = re.findall(r'\$\(METHOD_IMPL_BEGIN\)(.*)\$\(METHOD_IMPL_END\)', file_content, re.S | re.M)[0]
    reg_imp = re.findall(r'\$\(SERVICE_REGISTER_BEGIN\)(.*)\$\(SERVICE_REGISTER_END\)', file_content, re.S | re.M)[0]
    #print service_def
    
    service_code = ""
    register_imp = ""
    interface_list = ""
    for (key,value) in dict_pb['services'].items():
        method_imp = ""
        register_imp = register_imp + ReplaceCode2(reg_imp, dict_pb['proto'], dict_pb['pkg'], key)
        if (len(interface_list) != 0):
            interface_list = interface_list + ","
        interface_list = interface_list + dict_pb['proto'] + "." + key + ".BlockingInterface"
        for item in value:
            method_imp = method_imp + ReplaceCode(mth_imp, dict_pb['proto'], dict_pb['module'], item[0], item[1], item[2], item[3])
           
    file_content = re.sub(r'\$\(MODULE_REPLACE\)', dict_pb['module'], file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(PKG_REPLACE\)', dict_pb['pkg'], file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(PROTO_REPLACE\)', dict_pb['proto'], file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(INTERFACE_LIST_REPLACE\)', interface_list, file_content, re.S | re.M)  

    mth_block = re.findall(r'\$\(METHOD_IMPL_BEGIN\).*\$\(METHOD_IMPL_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(mth_block, method_imp)

    reg_block = re.findall(r'\$\(SERVICE_REGISTER_BEGIN\).*\$\(SERVICE_REGISTER_END\)', file_content, re.S | re.M)[0]
    file_content = file_content.replace(reg_block, register_imp)
    open(outputfile, 'w').write(file_content) 

## process makefile
def ProcessPOM(dict_pb, inputfile, outputfile):
    """ """
    if not dict_pb['pkg']:
        return
    file_content = open(inputfile, 'rb').read()
    file_content = re.sub(r'\$\(PROTO_FILE\)', dict_pb['proto_file'], file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)

## process makefile
def ProcessScript(dict_pb, inputfile, outputfile):
    """ """
    if not dict_pb['pkg']:
        return
    file_content = open(inputfile, 'rb').read()
    file_content = re.sub(r'SERVER_ENTRY=.*', "SERVER_ENTRY=" + dict_pb['pkg'] + ".ServiceImpl", file_content, re.S | re.M)
    file_content = re.sub(r'CLIENT_ENTRY=.*', "CLIENT_ENTRY=" + dict_pb['pkg'] + ".Client", file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)


## create server codes
def GenerateRpcServer(pb, output_path):
    """ """
    if not pb['pkg']:
        return

    ProcessService(pb, output_path + "/ServiceImpl.java", output_path + "/ServiceImpl.java")
    #os.system("make -C %s" % output_path )
    pass


## create client rpc code
def GenerateRpcClient(dict_pb, output_path):
    """ """
    if not dict_pb['pkg']:
        return
    inputfile = output_path + "/Client.java"
    outputfile = output_path + "/Client.java"

    file_content = open(inputfile, 'rb').read()
    file_content = re.sub(r'\$\(PKG_REPLACE\)', dict_pb['pkg'], file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(MODULE_REPLACE\)', dict_pb['module'], file_content, re.S | re.M)  
    file_content = re.sub(r'\$\(PROTO_REPLACE\)', dict_pb['proto'], file_content, re.S | re.M)  
    
    for (key,value) in dict_pb['services'].items():
        for item in value:
            file_content = re.sub(r'\$\(REQUEST_REPLACE\)', item[2], file_content, re.S | re.M)  
            file_content = re.sub(r'\$\(RESPONSE_REPLACE\)', item[3], file_content, re.S | re.M)  
            file_content = re.sub(r'\$\(RESPONSE_REPLACE\)', item[3], file_content, re.S | re.M)  
            file_content = re.sub(r'\$\(SERVICE_REPLACE\)', item[0], file_content, re.S | re.M)  
            file_content = re.sub(r'\$\(METHOD_REPLACE\)', item[1], file_content, re.S | re.M)  
            break
        break
    open(outputfile, 'w').write(file_content) 
    #os.system("make -C %s" % output_path )
    pass


## main process

if __name__ == '__main__':
    if len(sys.argv) < 4:
        print "Usage: create_rpc.py proto.filename frame_path module_name"
	sys.exit(0)

    ##switch to current dir
    curdir = os.path.split(os.path.realpath(__file__))[0]
    os.chdir(curdir)
    
    ## parse proto file
    pb = ParseProtoFile(sys.argv[1])
    pb["frame_path"] = os.path.abspath(sys.argv[2])
    pb["module"] = sys.argv[3]
    pb["proto_file"] = sys.argv[1]


    ## check output path
    output_path = os.path.abspath(sys.argv[2]) + '/src/main/java/' + pb["pkg"].replace(".", "/")
    demo_path = os.path.abspath(sys.argv[2]) + '/src/main/java/sample' 
    os.system("mkdir -p %s 2>/dev/null" % (output_path))
    os.system("cp -r %s/* %s/" % (demo_path, output_path))
    os.system("rm -r %s" % (demo_path))

    ## compile .proto file
    proto_out_path = os.path.abspath(sys.argv[2]) + '/src/main/java/'
    os.system("protoc -I. --java_out=%s %s" % (proto_out_path, sys.argv[1])) == 0 or sys.exit(-1);

    ## create server code
    GenerateRpcServer(pb, output_path)

    ## create client code
    GenerateRpcClient(pb, output_path)

    ProcessPOM(pb, "pom.xml", "pom.xml")
    ProcessScript(pb, "bin/entry.sh", "bin/entry.sh")
    pass
    
    


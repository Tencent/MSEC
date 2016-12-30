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
import commands

## process makefile
def ProcessScript(pkg, inputfile, outputfile):
    """ """
    if not pkg:
        return
    file_content = open(inputfile, 'rb').read()
    file_content = re.sub(r'SERVER_ENTRY=.*', "SERVER_ENTRY=" + pkg + "ServiceImpl", file_content, re.S | re.M)
    file_content = re.sub(r'CLIENT_ENTRY=.*', "CLIENT_ENTRY=" + pkg + "Client", file_content, re.S | re.M)
    open(outputfile, 'w').write(file_content)


## main process

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print "Usage: create_rpc_release.py jar_package"
        sys.exit(0)

    ##switch to current dir
    curdir = os.path.split(os.path.realpath(__file__))[0]
    os.chdir(curdir)
    
    cmd = "jar tvf " + sys.argv[1] + "  |grep ServiceImpl.class | awk '{print $NF}' |grep -o '.*/' | head -n1"
    status, output = commands.getstatusoutput(cmd)  
    if (status != 0):
        print "ServiceImpl not found."
        sys.exit(0);
    
    ProcessScript(output.replace("/", "."), "bin/entry.sh", "bin/entry.sh")
    pass
    
    


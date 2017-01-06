#!/bin/sh

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


libs=`ldd ../../third_party/python/lib/python2.7/lib-dynload/*.so | grep -v ":" | awk '{print $3}' | sort | uniq | grep 'so'`
rmdir ./libs
mkdir ./libs
echo $libs
for lib in $libs
do
    copy_flag=`echo $lib | egrep "ssl|crypt|krb5" | grep -v "grep" | wc -l`
    if [ $copy_flag -eq 1 ]
    then
        real_lib=$lib
        lib_dir=`dirname $real_lib`
        while [ -L $real_lib ]
        do
            real_lib=`readlink $real_lib`
            if [ ${real_lib:0:1} != '/' ]
            then
                real_lib=${lib_dir}"/"${real_lib}
                lib_dir=`dirname $real_lib`
            else
                lib_dir=`dirname $real_lib`
            fi
        done

        file_name=`echo $real_lib | awk -F"/" '{print $NF}'`
        link_name=`echo $lib | awk -F"/" '{print $NF}'`
        echo $file_name $link_name $real_lib
        cp $real_lib ./libs
        if [ $file_name != $link_name ]
        then
            echo $file_name $link_name
            cd ./libs
            ln -s $file_name $link_name
            cd -
        fi
    fi
done

cp ./libs/* ../../../bin/lib -rf
mkdir -p ../../module/rpc/py_template/lib
cp ./libs/* ../../module/rpc/py_template/lib -rf

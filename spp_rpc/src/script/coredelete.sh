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

#入参检查
if [ $# -ne 2 ];then
    echo "USAGE: $0 [core|gstack] remainfilenum"
    echo "e.g. : $0 core 10"
    echo "e.g. : $0 pkg  10"
    exit 1
fi

#删除的文件个数=总的文件个数-保留的文件个数
if [ $1 = "gstack" ];then
    total=`ls /data/spp_exception_log/spp_gstack* 2>/dev/null | wc -l`
fi

if [ $1 = "pkg" ]; then
    total=`ls /data/spp_exception_log/spp_pack* 2>/dev/null | wc -l`
fi

if [ $total -gt $2 ]
then
    rmnum=$(($total-$2))
else
    exit 0
fi

#删除多余的文件
if [ $1 = "gstack" ];then
    for file in `ls -t /data/spp_exception_log/spp_gstack* 2>/dev/null | tail -n $rmnum`
    do
        rm $file
    done
else
    for file in `ls -t /data/spp_exception_log/spp_pack* 2>/dev/null | tail -n $rmnum`
    do
        rm $file
    done
fi


exit 0

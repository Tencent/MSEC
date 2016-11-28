
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



#!/bin/bash

BIN_PATH=$(dirname $(readlink -f  $0))
cd $BIN_PATH/../
mkdir log 2>/dev/null
nohup java -cp './*:lib/*' org.msec.LogsysProxy -c conf/config.properties  >log/nohup.log 2>&1 &

cnt=`ps axu|grep java |grep 'org.msec.LogsysProxy' | grep -v grep |wc -l`
if (( cnt > 0 )); then
	echo "LogsysProxy start ok."
else
	echo "LogsysProxy start failed."
fi

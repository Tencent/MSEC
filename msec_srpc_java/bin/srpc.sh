
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



#!/bin/sh

BIN_PATH=$(dirname $(readlink -f  $0))
cd $BIN_PATH/../
mkdir log 2>/dev/null

source bin/entry.sh
export LD_LIBRARY_PATH=bin/lib

function start() {
    nohup java -Ddummy=msec.srpc -cp './*:bin/*:bin/lib/*:lib/*' $SERVER_ENTRY  >log/nohup.log 2>&1 &

	sleep 2
	cnt=`ps axu|grep java |grep "$SERVER_ENTRY" | grep -v grep |wc -l`
	if (( cnt > 0 )); then
			echo "Server($SERVER_ENTRY) start ok."
	else
			echo "Server($SERVER_ENTRY) start failed."
	fi
}


function stop() {
	cnt=`ps axu|grep java |grep "$SERVER_ENTRY" | grep -v grep |wc -l`
	if (( cnt > 0 )); then
			ps axu|grep java |grep "$SERVER_ENTRY" | grep -v grep  | awk '{print $2}' | xargs -I{} kill -9 {}

            sleep 2
			cnt=`ps axu|grep java |grep "$SERVER_ENTRY" | grep -v grep |wc -l`
			if (( cnt > 0 )); then
					echo "Server($SERVER_ENTRY) stop failed."
			else
					echo "Server($SERVER_ENTRY) stop ok."
			fi
	else
			echo "Server($SERVER_ENTRY) stop ok."
	fi
}

function restart() {
	stop
	sleep 3
	start
}

if [ $# -ne 1 ]; then
    echo "Usage: $0 start/stop/restart"
	exit -1
fi

if [ "$1" = "start" ]; then
    start
elif [ "$1" = "stop" ]; then
	stop
elif [ "$1" = "restart" ]; then
	restart
fi

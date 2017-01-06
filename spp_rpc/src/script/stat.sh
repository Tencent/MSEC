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

DEFAULT_PROXY_STAT=../stat/stat_srpc_proxy.dat
DEFAULT_WORKER_STAT=../stat/stat_srpc_worker1.dat

RED=\\e[1m\\e[31m
DARKRED=\\e[31m 
GREEN=\\e[1m\\e[32m
DARKGREEN=\\e[32m 
BLUE=\\e[1m\\e[34m
DARKBLUE=\\e[34m 
YELLOW=\\e[1m\\e[33m
DARKYELLOW=\\e[33m 
MAGENTA=\\e[1m\\e[35m
DARKMAGENTA=\\e[35m 
CYAN=\\e[1m\\e[36m
DARKCYAN=\\e[36m 
RESET=\\e[m

if [ $# -lt 1 ] 
then
	echo -e "$RED USAGE: $0 $YELLOW -proxy|-worker [stat_file] $RESET"
	exit 0;
fi

if [ $# = 2 ]; then
	stat_target_file=$2
elif [ $# = 1 ]; then
	if [ $1 = "-proxy" ]; then
		stat_target_file=$DEFAULT_PROXY_STAT
	elif [ $1 = "-worker" ]; then
		stat_target_file=$DEFAULT_WORKER_STAT
	else
		echo "only support \"-proxy\" \"-worker\" type"
		exit 0;
	fi
fi

if [ -s $stat_target_file ]; then
	echo -e "";
else
	echo -e "stat_file[$stat_target_file] is not exist"
fi


iter=0

if [ $1 = "-proxy" ]
then
	while [ 1 ]
	do
		iter=$(($iter+1));
		#取前后2秒的统计数据
		rx_bytes=`./stat_tool $stat_target_file | grep rx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		rx_packets=`./stat_tool $stat_target_file | grep rx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		tx_bytes=`./stat_tool $stat_target_file | grep tx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		tx_packets=`./stat_tool $stat_target_file | grep tx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		conn_overload=`./stat_tool $stat_target_file | grep conn_overload | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		shm_error=`./stat_tool $stat_target_file | grep shm_error | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		
		sleep 1
		next_rx_bytes=`./stat_tool $stat_target_file | grep rx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		next_rx_packets=`./stat_tool $stat_target_file | grep rx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		next_tx_bytes=`./stat_tool $stat_target_file | grep tx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		next_tx_packets=`./stat_tool $stat_target_file | grep tx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		next_conn_overload=`./stat_tool $stat_target_file | grep conn_overload | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		next_shm_error=`./stat_tool $stat_target_file | grep shm_error | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
        cur_connects=`./stat_tool $stat_target_file | grep cur_connects | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
	
		#计算差值
		rx_bytes_per_sec=$(($next_rx_bytes-$rx_bytes))
		rx_packets_per_sec=$(($next_rx_packets-$rx_packets))
		tx_bytes_per_sec=$(($next_tx_bytes-$tx_bytes))
		tx_packets_per_sec=$(($next_tx_packets-$tx_packets))
		conn_overload_per_sec=$(($next_conn_overload-$conn_overload))
		shm_error_per_sec=$(($next_shm_error-$shm_error))
	
		if [ $(($iter%20)) -eq 1 ]
        then
            echo -e "$DARKGREEN RX bytes / packets	TX bytes / packets	Conn overload	Shm errors  Cur connects$RESET"
            echo -e "-------------------------------------------------------------------------------------------"
        fi

		printf "%8d / %7d\t%8d / %7d\t%13d\t%9d\t%8d\n" $rx_bytes_per_sec $rx_packets_per_sec $tx_bytes_per_sec $tx_packets_per_sec $conn_overload_per_sec $shm_error_per_sec $cur_connects

	done
fi

if [ $1 = "-worker" ]
then
	while [ 1 ]
	do
		iter=$(($iter+1));

		shm_rx_bytes=`./stat_tool $stat_target_file | grep shm_rx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		shm_rx_packets=`./stat_tool $stat_target_file | grep shm_rx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		shm_tx_bytes=`./stat_tool $stat_target_file | grep shm_tx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		shm_tx_packets=`./stat_tool $stat_target_file | grep shm_tx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
        shm_queue_packets=`./stat_tool $stat_target_file | grep msg_shm_time| awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
        shm_queue_packets_time=`./stat_tool $stat_target_file | grep msg_shm_time| awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`

		sleep 1
		
		next_shm_rx_bytes=`./stat_tool $stat_target_file | grep shm_rx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		next_shm_rx_packets=`./stat_tool $stat_target_file | grep shm_rx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
		next_shm_tx_bytes=`./stat_tool $stat_target_file | grep shm_tx_bytes | awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`
		next_shm_tx_packets=`./stat_tool $stat_target_file | grep shm_tx_bytes | awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
        next_shm_queue_packets=`./stat_tool $stat_target_file | grep msg_shm_time| awk -F"|" '{printf $3}' | awk -F" " '{printf $1}'`
        next_shm_queue_packets_time=`./stat_tool $stat_target_file | grep msg_shm_time| awk -F"|" '{printf $4}' | awk -F" " '{printf $1}'`

		
		shm_rx_bytes_per_sec=$(($next_shm_rx_bytes-$shm_rx_bytes))
		shm_rx_packets_per_sec=$(($next_shm_rx_packets-$shm_rx_packets))
		shm_tx_bytes_per_sec=$(($next_shm_tx_bytes-$shm_tx_bytes))
		shm_tx_packets_per_sec=$(($next_shm_tx_packets-$shm_tx_packets))

        shm_queue_packets_per_sec=$(($next_shm_queue_packets-$shm_queue_packets))
        shm_queue_packets_time_per_sec=$(($next_shm_queue_packets_time-$shm_queue_packets_time))
        if [ $shm_queue_packets_per_sec -ne 0 ]
        then
            shm_queue_packets_time_per_sec=$(echo "scale=2;${shm_queue_packets_time_per_sec}/${shm_queue_packets_per_sec}"|bc)
        else
            shm_queue_packets_time_per_sec=0
        fi

        if [ $(($iter%20)) -eq 1 ]
        then
            echo -e "$DARKGREEN worker RX bytes / packets	TX bytes / packets  msg_shm_time ms$RESET"
            echo -e "------------------------------------------------------------------------------"
        fi

		printf "%16d / %7d\t%8d / %7d\t%11.2lf\n" $shm_rx_bytes_per_sec $shm_rx_packets_per_sec $shm_tx_bytes_per_sec $shm_tx_packets_per_sec $shm_queue_packets_time_per_sec

	done
fi

if [ $1 = "-content" ]
then
	while [ 1 ]
	do
		iter=$(($iter+1));

		######ouput all content of stat file######
		L=`./stat_tool $stat_target_file`
		echo -e "$L"

		sleep 1
	done
fi


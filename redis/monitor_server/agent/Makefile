
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



EXE=monitor_agent mon_set sysmon_set test_mmap test_client test_get test_set test_thread_pool
INCLUDE = -I../lib/wbl -I../lib/nlb -Imonitor_api
LIB = -L../lib/wbl -lwbl -L../lib/nlb -lnlbapi -Lmonitor_api -lmonitor -Wl,-dn -lprotobuf -Wl,-dy -lpthread
all: $(EXE)

monitor_agent: monitor_agent.cpp
#	cp ../server/src/monitor.proto .
	protoc --cpp_out=./ *.proto
	g++ -O2 -o monitor_agent monitor_agent.cpp monitor.pb.cc -g -Wall $(INCLUDE) $(LIB)

mon_set: mon_set.cpp
	g++ -O2 -o mon_set mon_set.cpp -g -Wall -Imonitor_api -Lmonitor_api -lmonitor

sysmon_set: sysmon_set.cpp
	g++ -O2 -o sysmon_set sysmon_set.cpp -g -Wall -Imonitor_api -Lmonitor_api -lmonitor

test_mmap: test_mmap.c
	gcc -o test_mmap test_mmap.c -g -Wall -Imonitor_api -Lmonitor_api -lmonitor

test_client: test_client.cpp
	g++ -o test_client test_client.cpp -g -Wall -Imonitor_api -Lmonitor_api -lmonitor -I../lib/wbl -L../lib/wbl -lwbl

test_get: test_get.cpp
	g++ -o test_get test_get.cpp monitor.pb.cc -g -Wall -I../lib/wbl -L../lib/wbl -lwbl -Wl,-dn -lprotobuf -Wl,-dy -lpthread

test_set: test_set.cpp
	g++ -o test_set test_set.cpp monitor.pb.cc -g -Wall -O2 -I../lib/wbl -L../lib/wbl -lwbl -Wl,-dn -lprotobuf -Wl,-dy -lpthread

test_thread_pool: test_thread_pool.cpp
	g++ -o test_thread_pool test_thread_pool.cpp -g -Wall -I../lib/wbl -L../lib/wbl -lwbl -lpthread

install:
	cp -rf monitor_agent mon_set sysmon* /msec/agent/monitor/
clean:
	rm $(EXE)

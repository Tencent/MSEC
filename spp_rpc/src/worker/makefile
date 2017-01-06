
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



COMM = ../comm
TBASE = ../comm/tbase
PROXY= ../proxy
WORKER=../worker
RPC=../rpc
ASYNC=../async
ASYNCFRAME=../async_frame
STATMGR=../comm/stat_mgr
MT=../sync_frame/micro_thread
SYNCFRAME=../sync_frame/spp_plugin
LIB_PATH=../bin/lib/
LIB_INSTALL_PATH=../../bin/lib/
SYNC=../sync
INTERNAL=../internal
PHPLIB=../third_party/php/lib

CFLAGS += -g -Wall -D_GNU_SOURCE -D_SPP_WORKER -D_MP_MODE -Wno-write-strings -DTIXML_USE_STL -Werror #-DOPEN_PRINT #定义_MP_MODE使得多进程写日志安全
INC += -I$(COMM) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(ASYNC) -I$(SYNCFRAME) -I$(STATMGR) -I$(RPC) -I$(INTERNAL)/se
LIB += -lpthread -ldl -rdynamic -L$(RPC) -lrpc_svr_comm  -L$(COMM) -L$(ASYNC) -L$(ASYNCFRAME) -L$(SYNC)\
	-L$(SYNCFRAME) -L$(MT) -lmtspp -lmt -lcommon -L$(INTERNAL)/monitor/lib -lmonitor -L$(PHPLIB) -lphp5\
	-L$(INTERNAL)/se -lse -L$(INTERNAL)/se/libunwind -lunwind-x86_64 -lunwind -L$(INTERNAL)/se/libbacktrace -lbacktrace \
        -Wl,-rpath,$(LIB_PATH) -L$(LIB_INSTALL_PATH) -ljansson -lhttpsupport -llzma

TARGET=srpc_worker 
all : $(TARGET)
OBJ = main.o			\
	defaultworker.o	\
	$(COMM)/timerlist.o \
	$(COMM)/poller.o \
	$(STATMGR)/ICostStat.o
include ../incl_comm.mk

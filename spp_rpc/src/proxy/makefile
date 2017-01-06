
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
MT=../sync_frame/micro_thread
SYNCFRAME=../sync_frame/spp_plugin
STATMGR=../comm/stat_mgr
LIB_PATH=../bin/lib/
LIB_INSTALL_PATH=../../bin/lib
INTERNAL=../internal
PHPLIB=../third_party/php/lib

CFLAGS += -g -Wall -D_GNU_SOURCE -D_SPP_PROXY -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL#-DOPEN_PRINT
INC += -I$(COMM) -I$(TBASE) -I$(TBASE)/tsockcommu -I$(STATMGR) -I$(RPC)
LIB += -lpthread -ldl -rdynamic -L$(RPC) -lrpc_svr_comm -L$(COMM) -L$(SYNCFRAME) -L$(MT) -lcommon \
       -L$(PHPLIB) -lphp5 -lmtspp -lmt -L$(INTERNAL)/monitor/lib -lmonitor -Wl,-rpath,$(LIB_PATH) \
       -L$(LIB_INSTALL_PATH) -lhttpsupport -ljansson
  
TARGET=srpc_proxy
all : $(TARGET)
OBJ = main.o			\
	defaultproxy.o \
	$(STATMGR)/ICostStat.o \
	$(STATMGR)/IFrameStat.o
include ../incl_comm.mk


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
STAT_MGR=../comm/stat_mgr
LIB_PATH = ./lib
LIB_INSTALL_PATH = ../../bin/lib
INTERNAL=../internal
SYNCFRAME=../sync_frame/spp_plugin
MT=../sync_frame/micro_thread

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif   

CFLAGS += -g  -Wall -rdynamic -D_GNU_SOURCE -D_SPP_CTRL  -D_MP_MODE -Wno-write-strings -Werror -DTIXML_USE_STL#-DOPEN_PRINT
INC += -I$(COMM) -I$(TBASE) -I$(STAT_MGR) -I$(RPC)
LIB += -lpthread -ldl -rdynamic -L$(RPC) -lrpc_svr_comm -L$(SYNCFRAME) -L$(MT) -lmtspp -lmt \
       -L$(COMM) -lcommon -L$(INTERNAL)/monitor/lib -lmonitor -Wl,-rpath,$(LIB_PATH) \
       -L$(LIB_INSTALL_PATH) -ljansson -lhttpsupport
TARGET=srpc_ctrl 
all : $(TARGET)
OBJ = main.o			\
	defaultctrl.o
include ../incl_comm.mk	

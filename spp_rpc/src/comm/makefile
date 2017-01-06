
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




#****************************************************************************

COMM = ../comm
TBASE = ../comm/tbase
TSOCKCOMMU = ../comm/tbase/tsockcommu
LIB_PATH = ../../bin/lib/
STATMGR = ./stat_mgr
BUILD_DIR=../comm
CONFIG=./config
INTERNAL=../internal

ifeq ($(ARCH),32)
	CFLAGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	CFLAGS +=  -m64 -DSUS_LINUX -pthread
endif

CFLAGS += -O2 -fno-inline -g -ggdb -Wall -D_GNU_SOURCE -D_MP_MODE -Wno-write-strings -Werror
CFLAGS += -pipe -fno-ident -fPIC -shared -z defs

INC += -I./ -I$(COMM) -I$(TBASE) -I$(TSOCKCOMMU) -I$(CONFIG) -I$(INTERNAL)/monitor/include
LIB += -lpthread -ldl
TARGET=libcommon.a
all : $(TARGET)
OBJ = $(COMM)/benchadapter.o \
	  $(COMM)/timerlist.o \
	  $(COMM)/frame.o \
	  $(COMM)/global.o \
	  $(COMM)/IPC.o \
	  $(COMM)/poller.o \
	  $(COMM)/serverbase.o \
	  $(COMM)/crc32.o \
	  $(COMM)/keygen.o \
	  $(COMM)/monitor.o \
	  $(COMM)/memqueue.o \
	  $(COMM)/shmcommu.o \
	  $(TBASE)/tlog.o \
	  $(TBASE)/misc.o \
	  $(TBASE)/tprocmon.o \
	  $(TBASE)/tshmcommu.o \
	  $(TBASE)/tstat.o \
	  $(TBASE)/notify.o \
	  $(TSOCKCOMMU)/tcache.o \
	  $(TSOCKCOMMU)/tconnset.o \
	  $(TSOCKCOMMU)/tepollflow.o \
	  $(TSOCKCOMMU)/tmempool.o \
	  $(TSOCKCOMMU)/tsockcommu.o \
	  $(TSOCKCOMMU)/tsocket.o \
	  $(STATMGR)/ICostStat.o \
	  $(STATMGR)/IFrameStat.o \
	  $(STATMGR)/StatMgr.o \
	  $(STATMGR)/StatMgrInstance.o \
	  $(CONFIG)/config.o \
	  $(CONFIG)/inifile.o \


RED = \\e[1m\\e[31m
DARKRED = \\e[31m
GREEN = \\e[1m\\e[32m
DARKGREEN = \\e[32m
BLUE = \\e[1m\\e[34m
DARKBLUE = \\e[34m
YELLOW = \\e[1m\\e[33m
DARKYELLOW = \\e[33m
MAGENTA = \\e[1m\\e[35m
DARKMAGENTA = \\e[35m
CYAN = \\e[1m\\e[36m
DARKCYAN = \\e[36m
RESET = \\e[m
CRESET =  ;echo -ne \\e[m; test -s $@

$(TARGET):$(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@ar crs $@ $^ $(CRESET)
	@chmod +x $@

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)   
%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET)    
clean:
	@rm -f $(OBJ) $(TARGET)



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



########MAKEFILE##########
PB_INC=../include/pb
PROTOC=../lib/protoc
FRAME_INC_PATH=../include
DEBUG= -g

INCCOMM=-I$(FRAME_INC_PATH) -I$(PB_INC)  -I$(FRAME_INC_PATH)/spp_incl/ -I$(FRAME_INC_PATH)/sync_incl -I$(FRAME_INC_PATH)/internal -I$(FRAME_INC_PATH)/r3c -I$(FRAME_INC_PATH)/hiredis -I$(FRAME_INC_PATH)/mysqlclient
#LIBCOMM= -L../lib $(shell ls ../lib | egrep "^lib" | egrep -v "libmt.a|libnlbapi.a|libprotobuf.a|librpc_clt_comm.a" | cut -b 4- | cut -d . -f 1 | awk '{print "-l"$$0}')
LIBCOMM= -L../lib $(shell ls ../lib | egrep "^lib" | egrep -v "libmt.a|libnlbapi.a|libprotobuf.a|librpc_clt_comm.a|libhiredis.a|libmysqlclient.a|libr3c.a" | cut -b 4- | cut -d . -f 1 | awk '{print "-l"$$0}') -lmysqlclient -lr3c -lhiredis
BINARY = service_$(MODULE_FILENAME_REPLACE).so

### rules
ALLPROTOBUF=$(wildcard ./*.proto)
ALLPROTOBUF_H=$(patsubst %.proto,%.pb.h,$(ALLPROTOBUF))
ALLPROTOBUF_OBJ=$(patsubst %.proto,%.pb.o,$(ALLPROTOBUF))

# Comment the following line if you are not using the gnu c compiler
C_ARGS = -Wall -g -fPIC -rdynamic

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif

# You might have to change this if your c compiler is not cc
CC = g++

# You shouldn't need to make any more changes below this line.
all: proto $(BINARY) 

### shell color config
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

%.o: %.cpp
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)

%.o: %.cc
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)

%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)  

%.o:%.S
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET) 

%.pb.h:%.proto
	@echo -e Generating $(BLUE)$@$(RESET)...
	@chmod a+x $(PROTOC)
	@$(PROTOC)  $<  --cpp_out=$(shell dirname $<)  --proto_path=$(shell dirname $<) \
	--proto_path=$(PB_INC) --proto_path=$(FRAME_INC_PATH)

%.pb.o:%.proto
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ ` echo $< | sed 's/.proto$$/.pb.cc/g' ` $(INCCOMM) $(CRESET)

clean:
	@rm -f $(BINARY)  *.o 

proto: $(ALLPROTOBUF_H) 


LIB_OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c)) \
          $(patsubst %.cc,%.o,$(wildcard *.cc)) $(patsubst %.cxx,%.o,$(wildcard *.cxx)) $(ALLPROTOBUF_OBJ)

service_$(MODULE_FILENAME_REPLACE).so: $(LIB_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@$(CC) -o $@ $^ -shared -ldl $(C_ARGS) $(LIBCOMM) -Wl,-z,interpose -Wl,-rpath,../bin/lib $(CRESET)
	@chmod +x $@ 

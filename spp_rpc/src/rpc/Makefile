
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
PB_INC=../third_party/pb
PROTOC=../third_party/pb/protoc
MICRO_THREAD=../sync_frame/micro_thread
SYNC_FRAME=../sync_frame/spp_plugin
COMM_DIR=../comm
INTERNAL=../internal
DEBUG= -g

INCCOMM= -I$(MICRO_THREAD) -I$(PB_INC) -I$(MICRO_THREAD) -I$(SYNC_FRAME) -I$(INTERNAL)/log/include \
         -I$(INTERNAL)/monitor/include -I$(INTERNAL)/nlb/include -I$(COMM_DIR) -I$(COMM_DIR)/tbase \
         -I$(COMM_DIR)/config -I./http -I../third_party/http-parser
LIB=-L$(INTERNAL)/nlb/lib -lnlbapi -L$(INTERNAL)/monitor/lib -lmonitor -L$(INTERNAL)/log/lib -llogsys_api -L$(PB_INC) -lprotobuf -lm
BINARY = librpc_svr_comm.so  librpc_clt_comm.so librpc_clt_comm.a  

### rules
ALLPROTOBUF=$(wildcard ./*.proto)
ALLPROTOBUF_H=$(patsubst %.proto,%.pb.h,$(ALLPROTOBUF))
ALLPROTOBUF_OBJ=$(patsubst %.proto,%.pb.o,$(ALLPROTOBUF))

# Comment the following line if you are not using the gnu c compiler
#C_ARGS = -Wall -g -fPIC -D_DEBUG
C_ARGS = -Wall -g -fPIC -O1
#.SUFFIXES: .o .cpp

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -pthread
else
	C_ARGS +=  -m64 -pthread
endif
# You might have to change this if your c compiler is not cc
CC = g++

# You shouldn't need to make any more changes below this line.

#.c.o:
#	$(CC) -c $*.c

all: proto $(BINARY) other_support install

other_support:
	make -C ./php-ext
	make -C ./http
	make -C ./python-ext

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

%.o: %.c
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET)  

%.o:%.S
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ $< $(INCCOMM) $(CRESET) 

%.pb.h:%.proto
	@echo -e Generating $(BLUE)$@$(RESET)...
	@chmod a+x $(PROTOC)
	@$(PROTOC)  $<  --cpp_out=$(shell dirname $<)  --proto_path=$(shell dirname $<) --proto_path=$(PB_PATH)

%.pb.o:%.proto
	@echo -e Compiling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(C_ARGS) -c -o $@ ` echo $< | sed 's/.proto$$/.pb.cc/g' ` $(INCCOMM) $(CRESET)

clean:
	@rm -f $(BINARY)  *.o 
	@make clean -C ./http

proto: $(ALLPROTOBUF_H) 


CLT_OBJ = srpc.pb.o  srpc_proto.o  srpc_channel.o srpc_comm.o srpc_intf.o
SVR_OBJ = srpc.pb.o  srpc_proto.o  srpc_network.o srpc_service.o srpc_log.o srpc_comm.o srpc_intf.o srpc_cintf.o

librpc_clt_comm.so: $(CLT_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@$(CC) -o $@ $^ -lm -shared -Wl,-z,interpose $(CRESET)
	@chmod +x $@ 

librpc_svr_comm.so: $(SVR_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@$(CC) -o $@ $^ $(LIB) -shared -Wl,-z,interpose $(CRESET)
	@chmod +x $@ 
librpc_clt_comm.a: $(CLT_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@ar crs  $@  $^  $(CRESET)
	@chmod +x $@ 

install:
	@-cp ./librpc_clt_comm.so ../../bin/lib/
	@-cp ./librpc_svr_comm.so ../../bin/lib/
	@-mkdir -p ../module/include/srpc_incl
	@-cp *.h ../module/include/srpc_incl
	@-cp ./http/libhttpsupport.so ../../bin/lib/
	@-cp ../third_party/jansson/lib/libjansson.so* ../../bin/lib/
	@-mkdir -p ../module/include/http_support
	@-cp ../third_party/http-parser/http_parser.h ../module/include/http_support -rf
	@-cp ./http/http_support.h ../module/include/http_support -rf

3nd_api_pkg:
	@echo -e  Generate cpp 3nd_api_pkg ...
	@-mkdir -p ./3nd_API_cpp
	@-rm ./3nd_API_cpp/* -rf
	@cp ../sync_frame/micro_thread/libmt.a ./3nd_API_cpp
	@cp ./librpc_clt_comm.a ./3nd_API_cpp
	@cp ../third_party/pb/libprotobuf.a ./3nd_API_cpp
	@cp ../internal/nlb/lib/libnlbapi.a ./3nd_API_cpp
	@cd ./3nd_API_cpp;  ar -x libmt.a; ar -x librpc_clt_comm.a; ar -x libnlbapi.a; ar -x libprotobuf.a; ar crs libsrpcclient.a *.o; cd - 
	@rm ./3nd_API_cpp/{libmt.a,libprotobuf.a,librpc_clt_comm.a,libnlbapi.a,*.o} -rf
	@mkdir -p 3nd_API_cpp/include
	@cp srpc.pb.h srpc_comm.h srpc_intf.h ../internal/nlb/include/nlbapi.h 3nd_API_cpp/include -rf
	@-rm 3nd_API_cpp.tgz -f
	@tar zcf 3nd_API_cpp.tgz 3nd_API_cpp
	@mv 3nd_API_cpp.tgz ../../dist -f



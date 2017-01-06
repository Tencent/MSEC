
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
DEBUG= -g

PHPINC=../../third_party/php/include/php
PHPLIB=../../third_party/php/lib
PHPPROTOLIB=../php-ext
INCCOMM=-I../../comm -I../../comm/tbase -I../../sync_frame/micro_thread -I../../sync_frame/spp_plugin -I. -I.. -I../rpc -I$(PHPINC) -I$(PHPINC)/Zend -I$(PHPINC)/TSRM -I$(PHPINC)/main -I../../third_party/pb -I../php-ext -I../http/ -I../../third_party/http-parser
LIBCOMM=-L$(PHPLIB) -lphp5 -L$(PHPPROTOLIB) -lsrpc_proto_php
BINARY= msec_php.so

# Comment the following line if you are not using the gnu c compiler
C_ARGS = -Wall -g -fPIC -rdynamic

C_ARGS += -DSUS_LINUX -pthread

# You might have to change this if your c compiler is not cc
CC = g++

# You shouldn't need to make any more changes below this line.
all: $(BINARY) 

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

clean:
	@rm -f $(BINARY)  *.o 

proto: $(ALLPROTOBUF_H) 


LIB_OBJ = $(patsubst %.cpp,%.o,$(wildcard *.cpp)) $(patsubst %.c,%.o,$(wildcard *.c)) \
          $(patsubst %.cc,%.o,$(wildcard *.cc)) $(patsubst %.cxx,%.o,$(wildcard *.cxx)) $(ALLPROTOBUF_OBJ)

$(BINARY): $(LIB_OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED)
	@-rm -f $@ 
	@$(CC) -o $@ $^ -shared -ldl $(C_ARGS) $(LIBCOMM) -Wl,-z,interpose -Wl,-rpath,../bin/lib $(CRESET)
	@chmod +x $@ 
	install $(BINARY) ../../../bin
	install ../../third_party/php/lib/libphp5.so ../../../bin/lib

3nd_api_pkg:
	@echo -e  Generate php $@ ...
	@-mkdir -p 3nd_API_php
	@-rm 3nd_API_php/* -rf
	@cp ../php-ext/srpc_comm_php/.libs/srpc_comm_php.so 3nd_API_php/
	@cp ../php-ext/nlb_php/.libs/nlb_php.so 3nd_API_php/
	@cp ../php-ext/libsrpc_proto_php_c.so 3nd_API_php/
	@tar -zxvf ../../module/rpc/php_template/pb4php.tgz -C 3nd_API_php/
	@tar zcvf 3nd_API_php.tgz ./3nd_API_php
	@-mv 3nd_API_php.tgz ../../../dist -f

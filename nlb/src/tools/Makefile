
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



CC=gcc
CFLAGS= -pipe -fno-ident -z defs -g -ggdb -Wall -D_GNU_SOURCE -Wno-write-strings -Werror
ifeq ($(ARCH),32)
	CFLAGS +=  -march=pentium4 -m32 -pthread
else
	CFLAGS +=  -m64 -pthread
endif

INC= -I./ -I../api -I../comm  -I../third_party/zookeeper/include/include -I../third_party/zookeeper/include/generated
CLIENT_TEST= client_test
SHOW_TOOL= show_servers
GET_ROUTE=get_route
UPDATE_ROUTE=update_route
OBJ_UPDATE_ROUTE=update_route.o
OBJ_GET_ROUTE= get_route.o
OBJ_CLIENT= client_test.o
OBJ_SHOW=show_servers.o
LIB=  -L../comm -lcomm -L../api -lnlbapi ../third_party/zookeeper/lib/libzookeeper_mt.a -lm

all: $(CLIENT_TEST) $(SHOW_TOOL) $(GET_ROUTE)
	@echo -e Compiling tools...

$(GET_ROUTE): $(OBJ_GET_ROUTE)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	@chmod +x $@

$(CLIENT_TEST): $(OBJ_CLIENT)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	@chmod +x $@
$(SHOW_TOOL): $(OBJ_SHOW)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	@chmod +x $@

$(UPDATE_ROUTE): $(OBJ_UPDATE_ROUTE)
	@echo -e Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	@chmod +x $@

distclean:
	@echo -e Clean ...
	@-rm -rf $(OBJ_CLIENT) $(OBJ_SHOW) $(CLIENT_TEST) $(SHOW_TOOL) $(GET_ROUTE) $(OBJ_GET_ROUTE) $(OBJ_ADD_SERVICE)

include ../incl_comm.mk

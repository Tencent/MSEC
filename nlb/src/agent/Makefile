
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

#INC= -I./ -I../comm -I../api -I../third_party/zookeeper/include/include -I../third_party/zookeeper/include/generated -I../third_party/cJSON-master
INC= -I./ -I../comm -I../api -I../third_party/zookeeper/include/zookeeper -I../third_party/jansson/include
TARGET= nlbagent 
OBJ= sysinfo.o zkheartbeat.o zkloadreport.o zkplugin.o zkservice.o config.o routeprocess.o networking.o jsonparser.o event.o agent.o policy.o log.o main.o
LIB= -L../comm -lcomm ../third_party/zookeeper/lib/libzookeeper_st.a ../third_party/jansson/lib/libjansson.a -lm

$(TARGET): $(OBJ)
	@echo -e  Linking $(CYAN)$@$(RESET) ...$(RED) 
	@$(CC) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
	@chmod +x $@

include ../incl_comm.mk

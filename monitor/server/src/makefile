
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



#deplist:yum install protobuf-static mariadb-devel
EXE=./monitor_server

INCLUDE=../../lib/tce/include ../../lib/mht/ ../../lib/wbl /usr/include/mysql
LIBPATH=../../lib/tce/lib ../../lib/mht/ ../../lib/wbl /usr/lib64/mysql

STATIC_LIB=tce mht wbl protobuf
DYNAMIC_LIB=mysqlclient pthread z

SOURCE=$(wildcard *.cpp) 
OBJ=$(patsubst %.cpp, %.o, $(SOURCE))

#CFLAGS= -g -Wall
CFLAGS= -Wall -fno-strict-aliasing -g -O2
CC=g++

all:pb $(EXE)
$(EXE):$(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(addprefix -L, $(LIBPATH)) -Wl,-dn $(addprefix -l, $(STATIC_LIB)) -Wl,-dy $(addprefix -l, $(DYNAMIC_LIB))

%.o:%.cpp
	$(CC) $(CFLAGS) -o $@ -c $< $(addprefix -I, $(INCLUDE))

pb:
	protoc --cpp_out=./ *.proto
	rename .cc .cpp *.cc

clean:
	rm -rf *.d *.o $(EXE) $(ARLIB)

install:
	cp -rf $(EXE) ../bin/


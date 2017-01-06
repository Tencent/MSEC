
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



PROTO_PHP_OBJ=srpc_proto_php.o ../srpc_comm.o ../srpc.pb.o
PROTO_PHP_BIN=libsrpc_proto_php.so
PROTO_PHP_BIN_WITH_PB=libsrpc_proto_php_c.so
PB_INC=../../third_party/pb
SRPC_INC=../
INC=-I$(PB_INC) -I$(SRPC_INC)
LIB=-lm

all: proto_php
	cd ./log_php; sh ./log_install.sh
	cd ./nlb_php; sh ./nlb_install.sh
	cd ./monitor_php; sh ./monitor_install.sh
	cd ./srpc_comm_php; sh ./srpc_comm.sh
	install $(PROTO_PHP_BIN) ../../../bin/lib

proto_php: $(PROTO_PHP_BIN) $(PROTO_PHP_BIN_WITH_PB)

$(PROTO_PHP_BIN): $(PROTO_PHP_OBJ)
	-rm -f $@ 
	g++ -o $@ $^ $(LIB) -shared -Wl,-z,interpose
	chmod +x $@ 

$(PROTO_PHP_BIN_WITH_PB): $(PROTO_PHP_OBJ)
	-rm -f $@ 
	g++ -o $@ $^ $(LIB) -shared -Wl,-z,interpose -L../../third_party/pb -lprotobuf
	chmod +x $@


%.o: %.cpp
	g++ -fPIC -Wall -g -c -o $@ $< $(INC)

%.o: %.cc
	g++ -fPIC -Wall -g -c -o $@ $< $(INC)

clean:
	-rm *.o $(PROTO_PHP_BIN) $(PROTO_PHP_BIN_WITH_PB) -rf


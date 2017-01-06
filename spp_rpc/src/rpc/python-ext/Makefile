
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



PROTO_PY_OBJ=srpc_proto_py.o ../srpc_comm.o ../srpc.pb.o
PROTO_PY_BIN=libsrpc_proto_py.so
PROTO_PY_BIN_WITH_PB=libsrpc_proto_py_c.so
PB_INC=../../third_party/pb
SRPC_INC=../
INC=-I$(PB_INC) -I$(SRPC_INC)
LIB=-lm

all: proto_py 
	make -C log_py_new
	make -C nlb_py
	make -C monitor_py
	make -C srpc_comm_py 
	-mkdir libs 2>/dev/null
	install $(PROTO_PY_BIN) ../../../bin/lib
	find log_py_new nlb_py monitor_py srpc_comm_py -name '*.so' | xargs -I{} cp -f {} libs

proto_py: $(PROTO_PY_BIN) $(PROTO_PY_BIN_WITH_PB)

$(PROTO_PY_BIN): $(PROTO_PY_OBJ)
	-rm -f $@ 
	g++ -o $@ $^ $(LIB) -shared -Wl,-z,interpose
	chmod +x $@ 

$(PROTO_PY_BIN_WITH_PB): $(PROTO_PY_OBJ)
	-rm -f $@ 
	g++ -o $@ $^ $(LIB) -shared -Wl,-z,interpose -L../../third_party/pb -lprotobuf
	chmod +x $@

%.o: %.cpp
	g++ -fPIC -Wall -g -c -o $@ $< $(INC)

%.o: %.cc
	g++ -fPIC -Wall -g -c -o $@ $< $(INC)

clean:
	-rm *.o $(PROTO_PY_BIN) $(PROTO_PY_BIN_WITH_PB) -rf
	-rm libs */build -rf


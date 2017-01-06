
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



LIB=srpc_comm_py.so
LIB_C=srpc_comm_py_c.so

PYINC=../../../third_party/python/include/python2.7
PYLIB=-L../../../third_party/python/lib  -lpython2.7
NLBINC=../../../internal/nlb/include
NLBLIB=-L../../../internal/nlb/lib -lnlbapi
COMM_INC=..
COMM_LIB=-L.. -lsrpc_proto_py
COMM_C_LIB=-L.. -lsrpc_proto_py_c
CC=g++
SRC=$(wildcard *.cpp)
OBJ=$(SRC:.cpp=.o)
CFLAGS=-g -Wall -O2 -fPIC

all:$(LIB) $(LIB_C)

$(LIB):$(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(LIB) -shared  -I$(NLBINC) -I$(COMM_INC) $(COMM_LIB) $(NLBLIB) -I$(PYINC) $(PYLIB)
	install $(LIB) ../../../../bin/lib/

$(LIB_C):$(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(LIB_C) -shared  -I$(NLBINC) -I$(COMM_INC) $(COMM_C_LIB) $(NLBLIB) -I$(PYINC) $(PYLIB)

clean:
	-rm $(LIB) $(LIB_C)

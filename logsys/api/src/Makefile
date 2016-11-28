
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



BINS := liblogsys_api.a logsys_api_test 
CURPWD=$(shell pwd)

#Compiler Flags
USER_MARCOS := 
PROTOBUF_PATH=../lib/pb

WARNING_FLAGS = -Wall \
	-Wsign-promo \
	-Woverloaded-virtual \
	-Wstrict-null-sentinel \
	-Wctor-dtor-privacy \
	-Wextra \
	-Wformat=2 \
	-Wno-unused-parameter \
	-Wmissing-include-dirs \
	-Wfloat-equal \
	-Wpointer-arith \
	-Wno-missing-field-initializers  \
	-Wformat-nonliteral

INCLUDE_PATH := . $(PROTOBUF_PATH)

CFLAGS = $(addprefix -I,$(INCLUDE_PATH)) $(WARNING_FLAGS) $(addprefix -D,$(USER_MARCOS)) -g -O2 -fPIC 
CXXFLAGS = $(CFLAGS) #-std=c++11

#ld parameters
LIBS := pthread  protobuf
LIBPATHS := $(PROTOBUF_PATH)
LDFLAGS = $(addprefix -L,$(LIBPATHS)) $(addprefix -l,$(LIBS))

CC = /usr/bin/gcc
CXX = /usr/bin/g++

API_SOURCE := logsys_api.cpp inifile.cpp
API_OBJS   := $(patsubst %.cpp,%.o,$(API_SOURCE))

PROTO := $(patsubst %.proto, %.pb.cc, $(wildcard *.proto))
PROTO_OBJS := $(patsubst %.cc, %.o, $(PROTO))

##
.PHONY : all clean

all: protobuflib proto $(BINS)

protobuflib:
	@if [ ! -f "../lib/pb/libprotobuf.a" ]; then \
		tar zxvf ../../../third_party//protobuf/protobuf-*tar.gz -C ../lib/; \
		cd ../lib/protobuf*; \
		./configure CFLAGS=-fPIC CPPFLAGS=-fPIC CXXFLAGS=-fPIC --prefix=$(CURPWD)/../lib/tmp;\
		make; make install; \
		/bin/sh ./libtool --tag=CXX   --mode=link g++ -pthread -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare -O2 -g -DNDEBUG -pthread  -o protoc src/main.o -lpthread ./src/.libs/libprotoc.a ./src/.libs/libprotobuf.a -lz; \
		cd -; \
		mkdir -p ../lib/pb; \
		cp $(CURPWD)/../lib/protobuf-*/protoc ../lib/pb; \
		cp $(CURPWD)/../lib/tmp/include/* ../lib/pb -rf; \
		cp $(CURPWD)/../lib/tmp/lib/libprotobuf.a ../lib/pb; \
		rm $(CURPWD)/../lib/tmp -rf; \
	fi

%.pb.cc: %.proto
	@chmod +x $(PROTOBUF_PATH)/protoc
	$(PROTOBUF_PATH)/protoc -I=./ --cpp_out=./ $<

%.cc: %.o
	$(CXX) $(CXXFLAGS) -c $<

proto: $(PROTO)

liblogsys_api.a: $(PROTO_OBJS) $(API_OBJS) 
	ar q $@ $^
	
logsys_api_test: logsys_api_test.o liblogsys_api.a 
	$(CXX) -o $@ $^ $(LDFLAGS)

clean:  
	rm -rf *~?? core* *.d *.o *.lo *.bak *.pb.h *.pb.cc $(BINS) $(OBJS) 



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



PROTBUF_PATH=../../third_party/protobuf/protobuf-bin
R3C_PATH=../../third_party/srpc/r3c
HTTP_PARSER_PATH=../../third_party/srpc/http-parser
LIBUNWIND_PATH=../../third_party/srpc/libunwind
LIBBACKTRACE_PATH=../../third_party/srpc/libbacktrace
MYSQLCLIENT_PATH=../../third_party/srpc/mysqlclient
HIREDIS_PATH=../../third_party/srpc/hiredis
CURPWD=$(shell pwd)

all: protobuflib r3c http_parser libunwind libbacktrace 
	-mkdir ../bin/
	-mkdir ../bin/lib/
	-mkdir ../moni/
	-mkdir ../stat/
	-mkdir ../log/
	find . -name "*.sh" | xargs chmod a+x 
	cp ./script/*.sh ../bin/
	make all -C ./comm/
	make all -C ./internal/se
	make all -C ./sync_frame/
	make all -C ./rpc
	make all -C ./proxy/
	make all -C ./controller/
	make all -C ./worker/
	make all -C ./tool/shm_key_tool/
	make all -C ./tool/shm_read_tool/
	make all -C ./tool/stat_tool/
	make all -C ./tool/cost_stat_tool/
	make -C ./third_party/hiredis
	sed -i 's/-I$$(HIREDIS)\/include/-I../g' third_party/r3c/Makefile 
	make libr3c.a -C ./third_party/r3c
	make copy -C ./module/
	
clean:
	make clean -C ./comm/
	make clean -C ./internal/se
	make clean -C ./sync_frame
	make clean -C ./rpc
	make clean -C ./proxy/
	make clean -C ./controller/
	make clean -C ./worker/
	make clean -C ./module/
	make clean -C ./tool/shm_key_tool/
	make clean -C ./tool/shm_read_tool/
	make clean -C ./tool/stat_tool/
	make clean -C ./tool/cost_stat_tool/
	make clean -C ./third_party/hiredis
	make clean -C ./third_party/r3c
	-rm -rf ../bin/
	-rm -rf ../moni/
	-rm -rf ../stat/
	-rm -rf ../log/
	-rm -rf ./publish/spp*

libunwind:
	@if [ ! -d "internal/se/libunwind" ]; then \
		tar zxvf ../../third_party/srpc/libunwind/libunwind*.tar.gz -C ./third_party/; \
		cd ./third_party/libunwind*; \
		./configure --prefix=$(CURPWD)/third_party/tmp; \
		make; make install; \
		cd -; \
		mkdir -p internal/se/libunwind; \
		cp $(CURPWD)/third_party/tmp/lib/libunwind.a internal/se/libunwind; \
		cp $(CURPWD)/third_party/tmp/lib/libunwind-x86_64.a internal/se/libunwind; \
		cp $(CURPWD)/third_party/tmp/include/* internal/se/libunwind; \
		rm $(CURPWD)/third_party/tmp -rf; \
	fi

libbacktrace:
	@if [ ! -d "internal/se/libbacktrace" ]; then \
		-rm ./third_party/libbacktrace* -rf; \
		cp ../../third_party/srpc/libbacktrace/libbacktrace*.zip ./third_party; \
		cd ./third_party; unzip libbacktrace*.zip; cd libbacktrace*; \
		./configure --prefix=$(CURPWD)/third_party/tmp; \
		make; make install; \
		cd ../../; \
		mkdir -p internal/se/libbacktrace; \
		cp $(CURPWD)/third_party/tmp/lib/libbacktrace.a internal/se/libbacktrace; \
		cp $(CURPWD)/third_party/tmp/include/*.h internal/se/libbacktrace; \
		rm $(CURPWD)/third_party/tmp -rf; \
	fi	


protobuflib:
	@if [ ! -f "./third_party/pb/libprotobuf.a" ]; then \
		tar zxvf ../../third_party/protobuf/protobuf-*tar.gz -C ./third_party; \
		cd ./third_party/protobuf*; \
		./configure CFLAGS=-fPIC CPPFLAGS=-fPIC CXXFLAGS=-fPIC --prefix=$(CURPWD)/third_party/tmp;\
		make; make install; \
		/bin/sh ./libtool --tag=CXX   --mode=link g++ -pthread -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare -O2 -g -DNDEBUG -pthread  -o protoc src/main.o -lpthread ./src/.libs/libprotoc.a ./src/.libs/libprotobuf.a -lz; \
		cd -; \
		mkdir -p ./third_party/pb; \
		cp $(CURPWD)/third_party/protobuf-*/protoc ./third_party/pb; \
		cp $(CURPWD)/third_party/tmp/include/* ./third_party/pb -rf; \
		cp $(CURPWD)/third_party/tmp/lib/libprotobuf.a ./third_party/pb; \
		rm $(CURPWD)/third_party/tmp -rf; \
	fi

r3c:
	@if [ ! -d "./third_party/r3c" ]; then \
		-rm ./third_party/r3c* -rf; \
		cp ../../third_party/srpc/r3c/r3c*.zip ./third_party; \
		cd ./third_party; unzip r3c*.zip; rm r3c*.zip; mv r3c* r3c;\
		cd -; \
	fi

http_parser:
	@if [ ! -d "./third_party/http-parser" ]; then \
		-rm ./third_party/http-parser -rf; \
		tar zxvf ../../third_party/srpc/http-parser/http-parser*.tar.gz -C ./third_party; \
		mv ./third_party/http-parser* ./third_party/http-parser; \
	fi


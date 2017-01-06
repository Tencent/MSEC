
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

all: nlb monitor log protobuflib r3c http_parser libunwind libbacktrace  php jansson json2pb python setuptools
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
	make -C ./rpc/php
	make -C ./rpc/python
	make copy -C ./module/
	make copy -C ./rpc/python
	
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
	make clean -C ./rpc/php
	make clean -C ./rpc/python
	-rm -rf ../bin/
	-rm -rf ../moni/
	-rm -rf ../stat/
	-rm -rf ../log/
	-rm -rf ./publish/spp*
	-rm -rf ./internal/nlb
	-rm -rf ./internal/log
	-rm -rf ./internal/monitor

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


protobuflib: python setuptools
	@if [ ! -f "./third_party/pb/libprotobuf.a" ]; then \
		tar zxvf ../../third_party/protobuf/protobuf-*tar.gz -C ./third_party; \
		cd ./third_party/protobuf*; \
		./configure CFLAGS=-fPIC CPPFLAGS=-fPIC CXXFLAGS=-fPIC --prefix=$(CURPWD)/third_party/tmp;\
		make; make install; \
		/bin/sh ./libtool --tag=CXX   --mode=link g++ -pthread -Wall -Wwrite-strings -Woverloaded-virtual -Wno-sign-compare -O2 -g -DNDEBUG -pthread  -o protoc src/main.o -lpthread ./src/.libs/libprotoc.a ./src/.libs/libprotobuf.a -lz; \
		cd ./python; \
		export LD_LIBRARY_PATH=$(CURPWD)/third_party/python/lib; \
		export PYTHONHOME=$(CURPWD)/third_party/python; \
		$(CURPWD)/third_party/python/bin/python setup.py build; \
		$(CURPWD)/third_party/python/bin/python setup.py install; \
		cd ../../../; \
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

php:
	@if [ ! -d "./third_party/php" ]; then \
		rm ./third_party/php* -rf; \
		tar -zxvf ../../third_party/srpc/php/php* -C ./third_party; \
		cd ./third_party/php* ;\
		./configure --enable-debug --without-pear --disable-xmlreader --disable-xmlwriter --disable-xml --disable-libxml --disable-dom --disable-simplexml --enable-sockets --enable-maintainer-zts --disable-cgi --enable-json --enable-cli --enable-embed --with-mysql=mysqlnd --with-mysqli=mysqlnd --with-pdo-mysql=mysqlnd --enable-pdo --prefix=$(CURPWD)/third_party/php; \
		make; make install; \
	fi
    
python:
	@if [ ! -d "./third_party/python" ]; then \
		rm ./third_party/python* -rf; \
		tar -xvf ../../third_party/srpc/python/* -C ./third_party; \
		cd ./third_party/Python* ;\
		./configure --enable-shared --prefix=$(CURPWD)/third_party/python; \
		make; make install; \
		cd -;\
	fi; 

setuptools: python
	@if [ ! -f "./third_party/python/bin/easy_install" ]; then \
		rm ./third_party/setuptools* -rf; \
		tar zxvf ../../third_party/srpc/setuptools/setuptools*tar.gz -C ./third_party; \
		cd ./third_party/setuptools*; \
		export LD_LIBRARY_PATH=$(CURPWD)/third_party/python/lib; \
		export PYTHONHOME=$(CURPWD)/third_party/python; \
		$(CURPWD)/third_party/python/bin/python bootstrap.py; \
		$(CURPWD)/third_party/python/bin/python setup.py install; \
	fi



jansson:
	@if [ ! -d "./third_party/jansson" ]; then \
		rm ./third_party/jansson* -rf; \
		tar -zxvf ../../third_party/srpc/jansson/jansson* -C ./third_party; \
		cd ./third_party/jansson*; \
		./configure --prefix=$(CURPWD)/third_party/jansson; \
		export CFLAGS=-fPIC; export CPPFLAGS=-fPIC; \
		make; make install; \
	fi

json2pb:
	@if [ ! -d "./third_party/json2pb" ]; then \
		unzip -o ../../third_party/srpc/json2pb/json2pb*.zip -d ./third_party; \
		mv ./third_party/json2pb* ./third_party/json2pb; \
	fi

nlb:
	@if [ ! -d "./internal/nlb/" ]; then \
		make -C ../../nlb/src; \
		mkdir -p ./internal/nlb/{lib,include}; \
		cp ../../nlb/src/api/libnlbapi.a ./internal/nlb/lib; \
		cp ../../nlb/src/api/nlbapi.h ./internal/nlb/include; \
	fi

monitor:
	@if [ ! -d "./internal/monitor/" ]; then \
		make -C ../../monitor/agent/monitor_api; \
		mkdir -p ./internal/monitor/{lib,include}; \
		cp ../../monitor/agent/monitor_api/libmonitor.a ./internal/monitor/lib; \
		cp ../../monitor/agent/monitor_api/monitor_client.h ./internal/monitor/include; \
	fi

log:
	@if [ ! -d "./internal/log/" ]; then \
		make -C ../../logsys/api/src; \
		mkdir -p ./internal/log/{lib,include}; \
		cp ../../logsys/api/src/liblogsys_api.a ./internal/log/lib; \
		cp ../../logsys/api/src/logsys_api.h ./internal/log/include; \
	fi


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



VERSION=`cat ./comm/version.h | grep NLB_VERSION | grep -v grep | awk -F "\"" '{print $$2}'`

ifndef ZOOKEEPER_TAR_PATH
#	export TMP_ONLY_FOR_EXECUTING:=$(error You must type "make ZOOKEEPER_TAR_PATH=/path")
ZOOKEEPER_TAR_PATH=../../third_party/nlb/zookeeper/zookeeper-3.4.8.tar.gz
endif

ifndef JANSSON_TAR_PATH
#	export TMP_ONLY_FOR_EXECUTING:=$(error You must type "make JANSSON_TAR_PATH=/path")
JANSSON_TAR_PATH=../../third_party/srpc/jansson/jansson-2.9.tar.gz
endif

CURPWD=$(shell pwd)

all: zookeeper jansson
	@make -C ./comm
	@make -C ./agent
	@make -C ./api
	@make -C ./tools

zookeeper:
	@if [ ! -d "./third_party/zookeeper" ]; then \
		-rm ./third_party/zookeeper -rf; \
		mkdir -p ./third_party/zookeeper; \
		tar -zxvf $(ZOOKEEPER_TAR_PATH) -C ./third_party; \
		cd ./third_party/zookeeper*/src/c; ./configure --prefix=$(CURPWD)/third_party/zookeeper; make; make install; \
	fi

jansson:
	@if [ ! -d "./third_party/jansson" ]; then \
		tar zxvf $(JANSSON_TAR_PATH) -C ./third_party; \
		cd ./third_party/jansson*/; ./configure --prefix=$(CURPWD)/third_party/jansson; make; make install;\
	fi 

clean:
	@make clean -C ./comm
	@make clean -C ./agent
	@make clean -C ./api
	@make distclean -C ./tools

release: clean all
	@echo -e "Generate nlb release package ..."
	@-rm ../release -rf
	@-mkdir -p ../release/nlb_agent_$(VERSION)/{bin,tools,log}
	@-mkdir -p ../release/nlb_api_$(VERSION)/{lib,include}
	@-cp agent/nlbagent ../release/nlb_agent_$(VERSION)/bin -rf
	@-cp tools/{get_route,show_servers,start.sh,stop.sh} ../release/nlb_agent_$(VERSION)/tools -rf
	@-chmod a+x tools/*
	@-cp api/libnlbapi.a ../release/nlb_api_$(VERSION)/lib -rf
	@-cp api/nlbapi.h ../release/nlb_api_$(VERSION)/include -rf
	@-tar -C ../release -zcf nlb_agent_$(VERSION).tar.gz nlb_agent_$(VERSION)
	@-tar -C ../release -zcf nlb_api_$(VERSION).tar.gz nlb_api_$(VERSION)
	@-md5sum nlb_agent_$(VERSION).tar.gz > md5sum
	@-md5sum nlb_api_$(VERSION).tar.gz >> md5sum
	@-mv nlb_agent_$(VERSION).tar.gz ../release
	@-mv nlb_api_$(VERSION).tar.gz ../release
	@-mv md5sum ../release

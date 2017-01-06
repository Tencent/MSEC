
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



clean:
	@echo
	@echo "Cleaning..." 

copy: cppcopy phpcopy pycopy

phpcopy:
	@cp ../third_party/php/bin/php ./rpc/php_template -rf
	@cp ../rpc/php-ext/libsrpc_proto_php_c.so ./rpc/php_template -rf
	@cp ../rpc/php-ext/nlb_php/.libs/nlb_php.so ./rpc/php_template -rf
	@cp ../rpc/php-ext/srpc_comm_php/.libs/srpc_comm_php.so ./rpc/php_template -rf
	@cp ../rpc/php/call_service.php ./rpc/php_template -rf

pycopy:
	@cp ../rpc/python-ext/libs/nlb_py.so ./rpc/py_template -rf
	@cp ../rpc/python-ext/libs/monitor_py.so ./rpc/py_template -rf
	@cp ../rpc/python-ext/libs/srpc_comm_py_c.so ./rpc/py_template/srpc_comm_py.so -rf
	@cp ../rpc/python-ext/libsrpc_proto_py_c.so ./rpc/py_template -rf

cppcopy: 
	@rm ./include/spp_incl -rf
	@-mkdir ./include/spp_incl
	@cp ../comm/spp_version.h ./include/spp_incl/.
	@cp ../comm/serverbase.h ./include/spp_incl/.
	@cp ../comm/timerlist.h ./include/spp_incl/.
	@cp ../comm/timestamp.h ./include/spp_incl/.
	@cp ../comm/poller.h ./include/spp_incl/.
	@cp ../comm/tbase/atomic*.h ./include/spp_incl/.
	@cp ../comm/tbase/myatomic*.h ./include/spp_incl/.
	@cp ../comm/tbase/list.h ./include/spp_incl/.
	@cp ../comm/tbase/tcommu.h ./include/spp_incl/.
	@cp ../comm/tbase/tlog.h  ./include/spp_incl/.
	@cp ../comm/tbase/tprocmon.h  ./include/spp_incl/.
	@cp ../comm/tbase/tstat.h  ./include/spp_incl/.
	@cp ../comm/tbase/tstat_policy.h  ./include/spp_incl/.
	@cp ../comm/stat_mgr/ICostStat.h ./include/spp_incl/.
	@cp ../comm/monitor.h ./include/spp_incl/.
	@cp ../comm/config/configini.h ./include/spp_incl/.
	@rm ./include/sync_incl -rf
	@-mkdir ./include/sync_incl
	@cp ../sync_frame/include/* ./include/sync_incl/.
	@rm ./include/nlb -rf
	@-mkdir ./include/nlb 
	@cp ../internal/nlb/include/nlbapi.h ./include/nlb
	@-rm ./include/hiredis -rf
	@-mkdir ./include/hiredis
	@cp ../third_party/hiredis/hiredis.h ./include/hiredis
	@cp ../third_party/hiredis/net.h ./include/hiredis
	@cp ../third_party/hiredis/read.h ./include/hiredis
	@cp ../third_party/hiredis/sds.h ./include/hiredis
	@-rm ./include/mysqlclient -rf
	@-mkdir ./include/mysqlclient
	@cp ../third_party/mysqlclient/include/* ./include/mysqlclient -rf
	@-rm ./include/r3c -rf
	@-mkdir ./include/r3c
	@cp ../third_party/r3c/r3c.h ./include/r3c
	@-rm ./lib -rf
	@-mkdir ./lib
	@-cp ../sync_frame/micro_thread/libmt.a ./lib -rf
	@-mkdir -p ./include/pb
	@-cp ../third_party/pb/google ./include/pb -rf
	@-cp ../third_party/pb/protoc ./lib -rf
	@-cp ../third_party/pb/libprotobuf.a ./lib -rf
	@-cp ../third_party/hiredis/libhiredis.a ./lib -rf
	@-cp ../third_party/r3c/libr3c.a ./lib -rf
	@-cp ../third_party/mysqlclient/lib/libmysqlclient.a ./lib -rf
	@-cp ../rpc/librpc_clt_comm.a ./lib -rf
	@-cp ../internal/nlb/lib/libnlbapi.a ./lib -rf
	@chmod a+x ./lib/*

dev_pkg: 
	@echo -e "generate srpc dev package..."
	@-mkdir -p ./srpc_dev/rpc
	@-cp ./example ./srpc_dev/ -rf
	@-cp ./include ./srpc_dev/ -rf
	@-cp ./lib ./srpc_dev/ -rf
	@-cp ./client ./srpc_dev/ -rf
	@-cp ./rpc/template ./srpc_dev/rpc/ -rf
	@-cp ./rpc/php_template ./srpc_dev/rpc/ -rf
	@-cp ./rpc/py_template ./srpc_dev/rpc/ -rf
	@-cp ../third_party/python ./srpc_dev/rpc/py_template -rf
	@tar --exclude=.svn -zcvf srpc_dev_pkg.tar.gz ./srpc_dev
	@-rm ./srpc_dev -rf


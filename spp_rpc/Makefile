
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



# Makefile for CIS
all:
	make all -C ./src/
	
clean:
	make clean -C ./src/

release: all
	@-mkdir ./srpc_bin
	@-cp ./bin ./srpc_bin/ -rf
	@-cp ./moni ./srpc_bin/ -rf
	@-cp ./stat ./srpc_bin/ -rf
	@-cp ./log ./srpc_bin/ -rf
	@-mkdir -p ./srpc_bin/etc
	@tar zcvf srpc_bin_pkg.tar.gz ./srpc_bin
	@rm ./srpc_bin -rf
	@mv srpc_bin_pkg.tar.gz ./dist
	@make dev_pkg -C ./src/module
	@mv ./src/module/srpc_dev_pkg.tar.gz ./dist
	@make 3nd_api_pkg -C ./src/rpc/php
	@make 3nd_api_pkg -C ./src/rpc
	@make 3nd_api_pkg -C ./src/rpc/python

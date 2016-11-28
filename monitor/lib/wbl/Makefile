
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



INCLUDE += 
CPPFLAGS = $(INCLUDE) -Wall -g -O2

lib_wbl_obj := wbl_timer.o wbl_comm.o wbl_config_file.o concurrent_node_factory.o concurrent_queue.o thread_pool.o tc_ex.o tc_epoller.o tc_socket.o tc_clientsocket.o

all: libwbl.a

clean:
	rm -f $(lib_wbl_obj) libwbl.a

libwbl.a: $(lib_wbl_obj)
	ar rcs libwbl.a $(lib_wbl_obj)


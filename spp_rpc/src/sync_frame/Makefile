
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




# You might have to change this if your c compiler is not cc
CC = g++
C_ARGS=-Wall -g -fPIC

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You shouldn't need to make any more changes below this line.
.cpp.o:
	    $(CC) $(C_ARGS) $(INCCOMM) -c $^ 
.c.o:
	    gcc $(C_ARGS)  $(INCCOMM) -c $^
					  
# Targets, maybe no use spp plugin, don't make it
all: mtlib spp_plug install

# Defines
MT_LIB=./micro_thread
SYNC_LIB=./spp_plugin

mtlib:
	make -C $(MT_LIB)

spp_plug:
	make -C $(SYNC_LIB)

clean:
	make -C $(MT_LIB) clean
	make -C $(SYNC_LIB) clean
	-rm -rf ./include
	-rm -rf ./lib

install:
	mkdir -p ./include
	cp  $(MT_LIB)/mt_api.h $(MT_LIB)/mt_capi.h $(MT_LIB)/mt_version.h $(MT_LIB)/mt_msg.h $(MT_LIB)/mt_incl.h $(SYNC_LIB)/*.h ./include
	install $(MT_LIB)/*.so ../../bin/lib
	install $(SYNC_LIB)/*.so ../../bin/lib


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



COMM = ../../comm
CONFIG = ../../comm/config
TBASE = ../../comm/tbase

ifeq ($(ARCH),32)
        CFLAGS += -m32 -march=pentium4
endif

INC = -I$(COMM) -I$(TBASE) -I$(CONFIG)
LIB = -L$(COMM) -lcommon 

all:shm_key_tool
shm_key_tool:main.o
	    g++ $(CFLAGS) $(INC)  main.o -g -o shm_key_tool -Wno-write-strings $(LIB)
		install shm_key_tool ../../../bin/
main.o:main.cpp
	    g++ $(CFLAGS) $(INC)  main.cpp -c -g -Wno-write-strings
clean:
	    @-rm *.o shm_key_tool -rf

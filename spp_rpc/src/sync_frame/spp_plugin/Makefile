
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



SPP_COMM=../../comm
SPP_COMM_TBASE=../../comm/tbase
INTERNAL=../../internal
MT_LIB=../micro_thread

INCCOMM=-I$(MT_LIB) -I$(SPP_COMM) -I$(SPP_COMM_TBASE) -I$(INTERNAL)/monitor/include -I../../worker -I../../comm/stat_mgr
LIBCOMM=-L$(MT_LIB) -lmt

DEBUG= -g

BINARY= libmtspp.so


# Comment the following line if you are not using the gnu c compiler
#C_ARGS = -Wall -g -D_DEBUG
C_ARGS = -Wall -ggdb -DMD_HAVE_EPOLL
#.SUFFIXES: .o .cpp

# You might have to change this if your c compiler is not cc
CC = g++
C_ARGS= -g -fPIC 

ifeq ($(ARCH),32)
	C_ARGS +=  -march=pentium4 -m32 -DSUS_LINUX -pthread
else
	C_ARGS +=  -m64 -DSUS_LINUX -pthread
endif
# You shouldn't need to make any more changes below this line.

#.c.o:
#	$(CC) -c $*.c

all:  $(BINARY)  

.cpp.o:
	    $(CC) $(C_ARGS) $(INCCOMM) -c $^ 
.c.o:
	    gcc $(C_ARGS)  $(INCCOMM) -c $^
					  
clean:
	rm -f $(BINARY)  *.o *.gch
	#-rm ar.tmp tmp-libsyncframe.a

libmtspp.a : SyncFrame.o
	rm -f $@
	ar q  $@  $^ 
	echo OPEN $@ > ar.tmp
	echo ADDLIB $(MT_LIB)/libmt.a >> ar.tmp
	echo SAVE >> ar.tmp
	echo END >> ar.tmp
	ar -M < ar.tmp
	rm ar.tmp

libmtspp.so : SyncFrame.o
	@echo -e  Linking $@ ...
	@rm -f $@
	@$(CC) -o $@ $^ -shared $(C_ARGS) -Wl,-z,interpose

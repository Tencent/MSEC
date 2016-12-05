
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



#include /home/happylong/tlib/IncTLib.Mk 

#BUILD = BUILD_DEBUG
BUILD =BUILD_RELEASE
EXES = test_timer  test_bitmap test_shm_array test_any_value test_svr test_client test_config 
MY_LIB = -L../lib/ -ltce -L/usr/local/mysql/lib/ -lmysqlclient -L/lib/ -lz -lpthread

MY_INC= -I../include/ -I/usr/local/mysql/include/
C_FLAGS = -Wall

all: $(EXES) 

.SUFFIXES: .c

.c:
	$(CC) -DNDEBUG -D$(BUILD) $(C_FLAGS)  -o $* $*.c $(LIB) $(MY_INC) $(MY_LIB)
.cpp:
	g++  -DNDEBUG -D$(BUILD) $(C_FLAGS)  $(MY_INC) $(INC_CGICC)   -o $* $*.cpp  $(LIB) $(MY_LIB) $(LIB_CGICC) -O3
#canvas.cpp
install:
	cp $(EXES) /home/happylong/apache/cgi-bin/

clean:
	rm -f $(EXES)
	rm -f *.o
	rm -f *.log


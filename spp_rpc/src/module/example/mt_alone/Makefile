
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



INCL_PATH=../../include
LIB_PATH=../../lib

CXX = g++
CFLAGS += -g -Wall -Wno-write-strings -Werror
INC = -I$(INCL_PATH) -I$(INCL_PATH)/spp_incl -I$(INCL_PATH)/sync_incl
LIB = -L$(LIB_PATH) -lmt -ldl

ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

TARGET = mt_alone
OBJ = mt_alone.o

$(TARGET): $(OBJ)
	$(CXX) $(CFLAGS) $^ $(LIB) -o $@

clean:
	@echo -e Clean ...
	@-rm *.o
	@-rm $(TARGET)

%.o: %.cpp
	@echo -e Compiling $< ...
	$(CXX) $(CFLAGS) -c -o $@ $< $(INC)



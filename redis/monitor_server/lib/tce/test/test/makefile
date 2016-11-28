
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



LIBS := pthread tce tconv
INCS := ./  ../../include/ ../../../tcgi/include

LIBPATHS := ../../lib/ ../../../tcgi/lib/

#USER_MARCOS := _LINUX_OS_ _DAEMON_ #-DNDEBUG
USER_MARCOS := 
CFLAGS = -DNDEBUG -g -O -Wall 
CC = g++ 


SOURCE := $(wildcard *.cpp)  
OBJS := $(patsubst %.cpp,%.o,$(SOURCE))
  
%.o:%.cpp
	$(CC) $(CFLAGS) $(addprefix -D,$(USER_MARCOS)) $(addprefix -I,$(INCS)) -c $< -o $@

all: $(OBJS)
	$(CC) $(CFLAGS) $(addprefix -L,$(LIBPATHS))  $(addprefix -I,$(INCS)) -o test_any_value test_any_value.o $(addprefix -l,$(LIBS))

clean :  
	rm -rf *.d *.o *.lo  test_any_value
	 

DEPEND= makedepend $(addprefix -D,$(USER_MARCOS)) $(addprefix -I,$(INCS))
depend:$(SOURCE)
	$(DEPEND) $(SOURCE)


	
# DO NOT DELETE


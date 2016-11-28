
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



.SUFFIXES: .o .cpp

CXX        = g++
#CFLAGS     = -g -O -Wall #-DDEBUG]
#CFLAGS = -Wall  -O2 -DDEBUG -fPIC
CFLAGS     = -g -Wall -O2

INCLUDEDIR = -I./

OBJECT = link_table.o shm_adapter.o multi_hash_table.o  

OUTPUT  := libmht.a 

all: $(OUTPUT)
 
libmht.a:$(OBJECT)
	ar -rs $@ $^


.cpp.o:
	$(CXX) $(CFLAGS) $(INCLUDEDIR) -c $^ 
	
.c.o:
	gcc $(CFLAGS) $(INCLUDEDIR) -c $^ 

test:
	g++ -g -Wall test_mht.cpp -o test -L./ -lmht


install:

uninstall:

clean:
	rm -f *.o $(OUTPUT)


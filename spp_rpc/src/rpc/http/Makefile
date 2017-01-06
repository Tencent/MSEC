
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



TARGET=libhttpsupport.so
OBJ=http_support.o ../../third_party/json2pb/json2pb.o ../../third_party/http-parser/http_parser.o
INC=-I. -I.. -I../../third_party/jansson/include -I../../third_party/json2pb -I../../third_party/pb -I../../third_party/http-parser
LIB=-L../../third_party/jansson/lib -ljansson
CXXFLAGS=-fPIC
LDFLAGS=
CXX=g++

$(TARGET): $(OBJ)
	$(CXX) -o $@ -shared  $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) 

%.o: %.cc
	$(CXX) $(CXXFLAGS) -c -o $@ $< $(INC) 

clean:
	-rm $(OBJ) $(TARGET)


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



CFLAGS += -g -Wall -D_GNU_SOURCE 
INC += 
LIB += 

SRCS=$(wildcard *.c) $(wildcard *.cc) $(wildcard *.cxx) $(wildcard *.cpp)
OBJS=$(addsuffix .o, $(basename $(SRCS)))

RED = \\e[1m\\e[31m
DARKRED = \\e[31m 
GREEN = \\e[1m\\e[32m
DARKGREEN = \\e[32m 
BLUE = \\e[1m\\e[34m
DARKBLUE = \\e[34m 
YELLOW = \\e[1m\\e[33m
DARKYELLOW = \\e[33m 
MAGENTA = \\e[1m\\e[35m
DARKMAGENTA = \\e[35m 
CYAN = \\e[1m\\e[36m
DARKCYAN = \\e[36m 
RESET = \\e[m
CRESET =  ;echo -ne \\e[m; test -s $@

all:$(OBJS)
#	@echo -e Linking $(CYAN)$@$(RESET) ...$(RED)
#	@$(CXX) -o $@ $^ $(CFLAGS) $(LIB) $(CRESET)
%.o: %.cpp
	@echo -e Compling $(GREEN)$<$(RESET) ...$(RED)
	@$(CXX) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
%.o: %.c
	@echo -e Compling $(GREEN)$<$(RESET) ...$(RED)
	@$(CC) $(CFLAGS) -c -o $@ $< $(INC) $(CRESET) 	 
clean:
	@rm -f $(OBJS) 

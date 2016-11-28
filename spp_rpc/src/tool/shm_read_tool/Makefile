
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



ifeq ($(ARCH),32)
	CFLAGS += -m32 -march=pentium4
endif

shmread.o:shmread.c
	gcc -g -c shmread.c -o shmread.o $(CFLAGS)
shmread:shmread.o
	gcc shmread.o -o shmread $(CFLAGS)
	install shmread  ../../../bin/
clean:
	rm -rf *.o shmread
all:shmread


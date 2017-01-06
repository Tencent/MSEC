#!/bin/sh

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

../../../third_party/php/bin/phpize 
./configure --enable-nlb_php --with-php-config=../../../third_party/php/bin/php-config
sed -i '1i EXTRA_INCLUDES = -I../../../internal/nlb/include' Makefile
sed -i 's/EXTRA_LDFLAGS =/EXTRA_LDFLAGS = -L..\/..\/..\/internal\/nlb\/lib -lnlbapi/g' Makefile
make
install .libs/nlb_php.so ../../../../bin/lib/


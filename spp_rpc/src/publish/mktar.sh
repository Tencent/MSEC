
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



#!/bin/sh

if [ $# != 1 ]; then
	echo "usage: mktar.sh version"
	exit -1
fi

vn=`grep $1 ../../src/comm/spp_version.h | wc -l`
if [ $vn -ne 1 ]
then
	echo -e "version $1 ??? WHAT ABOUT spp_version.h\n"
	exit -2
fi

cd ../../src/
make clean all
cd -
cd ../../src/module/
make clean;make clean
cd -
rm -rf spp 
mkdir -p ./spp/bin
cp ../../bin/* ./spp/bin/ -r
cp ../module ./spp/ -r
cp ../../etc ./spp/ -r

find ./spp/ -name ".svn" |xargs rm -rf

tar czf ../../dist/spp_$1_64bit.tar.gz spp

rm -rf spp

# 32BIT ##################################
cd ../../src/
make clean all32
cd -
cd ../../src/module/
make clean;make clean
cd -
rm -rf spp
mkdir -p ./spp/bin
cp ../../bin/* ./spp/bin/ -r
cp ../module ./spp/ -r
cp ../../etc ./spp/ -r

find ./spp/ -name ".svn" |xargs rm -rf

tar czf ../../dist/spp_$1_32bit.tar.gz spp
rm -rf spp

md5sum ../../dist/*$1*.gz > ../../dist/md5sum


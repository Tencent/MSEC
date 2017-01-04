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


# This script build 3 tars:
# 1. java_dev.tar   for rpc server development
# 2. java.tar       for rpc server distribution  
# 3. java_api.tar   api for rpc server

CURDIR=$(dirname $(readlink -f  $0))
cd $CURDIR
rm java.tar  java_dev.tar java_api.tar 2>/dev/null
rm -rf java_api  2>/dev/null
mkdir java_api

cd srpc
mvn clean; mvn package; 

#build srpc-proxy.jar
cd target/classes;
jar cf srpc-proxy.jar  srpc  \
       api/lb/msec/org  \
       sofiles/libjni_lb.so   \
       org/msec/net/NettyCodecUtils.class  \
       org/msec/rpc/SrpcProxy.class  \
       org/msec/rpc/RpcRequest.class && mv srpc-proxy.jar ../../../java_api
cd -;

mvn clean;
cd $CURDIR

cp lib/protobuf-java-2.5.0.jar  java_api
( cd java_api && tar cvf java_api.tar  * && mv java_api.tar .. )

tar cvf java_dev.tar bin  create_rpc.py  create_rpc_release.py  etc lib  pom.xml  pom_offline.xml  src  srpc  srpc.iml  srpc.proto

tar cvf java.tar  bin  create_rpc_release.py  etc  lib


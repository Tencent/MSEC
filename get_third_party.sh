#!/bin/sh
set -e

echo ' download jar files used by msec_console:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/msec_console;cd third_party/msec_console
wget http://central.maven.org/maven2/org/apache/commons/commons-compress/1.8/commons-compress-1.8.jar
wget http://central.maven.org/maven2/commons-fileupload/commons-fileupload/1.2.2/commons-fileupload-1.2.2.jar
wget http://central.maven.org/maven2/commons-io/commons-io/2.4/commons-io-2.4.jar
wget http://www.java2s.com/Code/JarDownload/jackson-all/jackson-all-1.7.4.jar.zip
wget http://central.maven.org/maven2/javax/servlet/javax.servlet-api/3.1.0/javax.servlet-api-3.1.0.jar
wget http://central.maven.org/maven2/org/jfree/jcommon/1.0.21/jcommon-1.0.21.jar
wget http://central.maven.org/maven2/org/jfree/jfreechart/1.0.18/jfreechart-1.0.18.jar
wget http://central.maven.org/maven2/junit/junit/4.12/junit-4.12.jar
wget http://central.maven.org/maven2/log4j/log4j/1.2.17/log4j-1.2.17.jar
wget http://central.maven.org/maven2/mysql/mysql-connector-java/5.1.38/mysql-connector-java-5.1.38.jar
wget http://maven.restlet.org/org/json/org.json/2.0/org.json-2.0.jar
wget http://central.maven.org/maven2/com/google/protobuf/protobuf-java/2.6.0/protobuf-java-2.6.0.jar
wget http://central.maven.org/maven2/org/scf4j/scf4j-props/1.0.1/scf4j-props-1.0.1.jar
wget http://central.maven.org/maven2/org/slf4j/slf4j-api/1.7.18/slf4j-api-1.7.18.jar
wget http://www.java2s.com/Code/JarDownload/zookeeper/zookeeper-3.4.3.jar.zip
wget http://nchc.dl.sourceforge.net/project/jgraphviz/jgraphviz/0.1/com.rapitasystems.jgraphviz_2.20.3_linux_x86.zip
cd -

echo ' download jar files used by monitor console:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/monitor/monitor_console;cd third_party/monitor/monitor_console
cp ../../msec_console/*  ./
cd -

echo ' download jar files used by redis console:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/redis/redis_console; cd   third_party/redis/redis_console
cp ../../msec_console/*  ./
wget http://search.maven.org/remotecontent?filepath=redis/clients/jedis/2.8.1/jedis-2.8.1.jar
cd -

echo 'download jar files used by remote shell server:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/remote_shell/server;cd third_party/remote_shell/server
wget http://central.maven.org/maven2/org/bouncycastle/bcprov-jdk16/1.46/bcprov-jdk16-1.46.jar
wget http://www.java2s.com/Code/JarDownload/jackson-all/jackson-all-1.7.4.jar.zip
wget http://central.maven.org/maven2/junit/junit/4.12/junit-4.12.jar
wget http://maven.restlet.org/org/json/org.json/2.0/org.json-2.0.jar
cd -

echo 'download libs used by remote shell agent:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/remote_shell/agent;cd third_party/remote_shell/agent
wget https://github.com/cesanta/frozen/blob/master/frozen.c
wget https://github.com/cesanta/frozen/blob/master/frozen.h
cd -

mkdir -p third_party/monitor/monitor_server; cd  third_party/monitor/monitor_server
echo 'download protobuf for monitor_server'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
wget https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz
echo 'download zlib for monitor_server:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
wget http://zlib.net/fossils/zlib-1.2.8.tar.gz
echo 'download mysql client library for monitor_server:'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
wget http://cdn.mysql.com/archives/mysql-5.5/MySQL-devel-5.5.8-1.linux2.6.x86_64.rpm
cd -


echo 'download protobuf, zlib and mysql client library for redis'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/redis/monitor_server; cd  third_party/redis/monitor_server
cp  ../../monitor/monitor_server/protobuf* .
cp  ../../monitor/monitor_server/zlib* .
cp  ../../monitor/monitor_server/MySQL* .
cd -

echo 'download zookeeper for nlb'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/nlb/zookeeper; cd third_party/nlb/zookeeper;
wget http://mirror.bit.edu.cn/apache/zookeeper/zookeeper-3.4.8/zookeeper-3.4.8.tar.gz
cd -

echo 'download protobuf....'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'
mkdir -p third_party/protobuf; cd third_party/protobuf
wget -c -O protobuf-2.5.0.tar.gz https://github.com/google/protobuf/releases/download/v2.5.0/protobuf-2.5.0.tar.gz
cd - 

mkdir -p third_party/srpc; cd  third_party/srpc
echo 'download library for srpc....'
echo '>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>'

mkdir -p r3c; cd r3c
wget --content-disposition --timeout=5 --tries=5 'https://codeload.github.com/eyjian/r3c/zip/master'
cd -

mkdir -p libbacktrace; cd libbacktrace
wget --content-disposition --timeout=5 --tries=5 'https://codeload.github.com/ianlancetaylor/libbacktrace/zip/master'
cd -

mkdir -p http-parser; cd http-parser
wget --content-disposition --timeout=5 --tries=5 'https://codeload.github.com/nodejs/http-parser/tar.gz/v2.7.1'
cd -

mkdir -p libunwind; cd libunwind
wget --content-disposition --timeout=5 --tries=5 'http://download.savannah.gnu.org/releases/libunwind/libunwind-1.1.tar.gz'
cd -

mkdir -p jansson; cd jansson
wget --content-disposition --timeout=5 --tries=5 'http://www.digip.org/jansson/releases/jansson-2.9.tar.gz'
cd -

mkdir -p php; cd php
wget --content-disposition --timeout=5 --tries=5 'http://cn2.php.net/distributions/php-5.6.25.tar.gz'
cd -

mkdir -p python; cd python
wget --content-disposition --timeout=5 --tries=5 'https://www.python.org/ftp/python/2.7.12/Python-2.7.12.tgz'
cd -

mkdir -p json2pb; cd json2pb
wget --content-disposition --timeout=5 --tries=5 'https://codeload.github.com/downup2u/json2pb-master/zip/master' 
cd -

mkdir -p setuptools; cd setuptools
wget --content-disposition --timeout=5 --tries=5 'https://codeload.github.com/pypa/setuptools/tar.gz/v32.0.0'
cd -

cd -

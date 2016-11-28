#!/bin/sh

echo 'build logsys api...'
cd api/src;
make clean && make
if [ $? -ne 0 ]; then
    exit
fi
cd - &> /dev/null

echo 'build logsys flume-mysql-sink...'
cd flume-ng-mysql-sink
mvn clean && mvn package
if [ $? -ne 0 ]; then
    exit
fi
cd - &> /dev/null

echo 'build logsys flume-protobuf-source...'
cd flume-protobuf-source
mvn clean && mvn package
if [ $? -ne 0 ]; then
    exit
fi
cd - &> /dev/null

echo 'build logsys proxy...'
cd proxy
mvn clean && mvn package
if [ $? -ne 0 ]; then
    exit
fi
cd - &> /dev/null

编译步骤：
1. 下载zookeeper源码包
    wget http://apache.fayea.com/zookeeper/zookeeper-3.4.8/zookeeper-3.4.8.tar.gz
2. 下载jansson源码,并解压
    wget http://www.digip.org/jansson/releases/jansson-2.9.tar.gz
3. 编译
    make ZOOKEEPER_TAR_PATH=./zookeeper-3.4.8.tar.gz JANSSON_TAR_PATH=./jansson-2.9.tar.gz

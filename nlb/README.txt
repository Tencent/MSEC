编译步骤：
1. 下载zookeeper源码包
    wget http://apache.fayea.com/zookeeper/zookeeper-3.4.8/zookeeper-3.4.8.tar.gz
2. 下载cJSON源码,并解压
	github地址：https://github.com/DaveGamble/cJSON
3. 编译
	make ZOOKEEPER_TAR_PATH=/.../zookeeper-3.4.8.tar.gz CJSON_TAR_PATH=/.../cJSON-version
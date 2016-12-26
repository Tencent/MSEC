# 1、概述 #
这里是毫秒服务引擎的代码，分好几个模块，开发语言主要是java和c/c++。每个模块需要单独编译。
官网 <http://haomiao.qq.com> 上有编译好的docker镜像和文档可以快速部署，强烈推荐。

## 编译环境要求 ##
* gcc version >= 4.1.2
* autoconf  version >= 2.59
* JDK  version > = 1.6
* Maven version > = 3.2.5
* Linux内核版本>=2.6.18
* CPU架构：x86\_64

## 关于用到的外部库 ##
下载到源代码后，在能够连接外网的linux服务器上手工执行get\_third\_party.sh脚本，会下载各模块用到的外部库，保存在third\_party目录下； srpc\_java和logsys是maven组织的java项目，mvn通过pom.xml配置自动下载用到的外部库。

# 2、web console #
## 2.1 简介 ##

Web console的代码在msec\_console子目录，是一个典型的Java Web Application。目录msec\_console下的src子目录是java代码，web子目录是静态页面、css、图片等web资源文件。

## 2.2 用到的外部库 ##
* commons-compress-1.8.jar
* commons-fileupload-1.2.2.jar
* commons-io-2.4.jar
* ini4j-0.5.4.jar
* jackson-all-1.6.0.jar
* javax.servlet-api-3.1.0.jar
* jcommon-1.0.21.jar
* jfreechart-1.0.18.jar
* junit-4.12.jar
* log4j-1.2.17.jar
* mysql-connector-java-5.1.38-bin.jar
* org.json.jar
* protobuf-java-2.6.1.jar
* scf4j-props-1.0.1.jar
* slf4j-api-1.7.18.jar
* zookeeper-3.4.8.jar

几乎所有上面的jar都能在 <http://mvnrepository.com/> 下载到。

#3、remote shell#
## 3.1 简介 ##
子目录remote\_shell是console服务器用来对业务运营机进行文件传输、远程命令执行的系统的代码。典型的应用场景是：发布的时候利用remote\_shell系统传输发布文件、执行发布命令。

子目录remote\_shell下又有三个子目录：

* server是部署在console服务器上发出命令的程序，用c语言开发；
* agent是部署在业务运营机上的程序，它接收server的命令，用java开发的；
* InteractiveTool是和server部署在一起的一个命令行工具，通过它可以手工发起命令，用java开发的。

## 3.2 用到的外部库 ##
server用到：

* bcprov-jdk16-1.46.jar
* jackson-all-1.6.0.jar
* junit-4.12.jar
* org.json.jar

agent用到：

* frozen, 轻量的json解析C语言库

# 4、monitor #
## 4.1 简介 ##
子目录monitor是一套集中式的监控服务，该监控服务既可作为msec的集中式监控服务供msec业务上报，也可单独部署并使用独立的standalone\_console进行管理。典型的应用场景是：业务上报监控到监控agent，监控agent汇总定时上报给监控服务，开发运营人员通过msec的web_console页面或独立部署的standalone\_console页面查看业务监控汇总视图。

子目录monitor下又有三个子目录：

* lib是供agent/server用的公共库，用c++语言开发；
* server是监控服务，用c++语言开发；
* agent是部署在业务运营机上的程序，它汇总业务模块的监控数据，定时上报给监控服务，用c++开发的；

standalone\_console是一个典型的Java Web Application。子目录standalone\_console下的src子目录是java代码，web子目录是静态页面、css、图片等web资源文件。

## 4.2 用到的外部库 ##
server用到：

* protobuf库（版本2.5.0）
* mysqlclient库（版本>=5.5.0）
* zlib库（版本>=1.2.0）

standalone\_console用到：

* commons-compress-1.8.jar
* commons-fileupload-1.2.2.jar
* commons-io-2.4.jar
* jackson-all-1.6.0.jar
* javax.servlet-api-3.1.0.jar
* jcommon-1.0.21.jar
* jfreechart-1.0.18.jar
* junit-4.12.jar
* log4j-1.2.17.jar
* mysql-connector-java-5.1.38-bin.jar
* org.json.jar
* protobuf-java-2.6.1.jar
* scf4j-props-1.0.1.jar
* slf4j-api-1.7.18.jar
* zookeeper-3.4.8.jar

# 5、redis #
## 5.1 简介 ##
子目录redis\_console是一套基于redis cluster的KV运维管理平台。Web侧的代码在redis\_console子目录，是一个典型的Java Web Application。子目录redis\_console下的src子目录是java代码，web子目录是静态页面、css、图片等web资源文件。子目录monitor\_server提供了一套专用于redis服务的监控系统，和msec的monitor服务的代码实现有些微的差别，但代码路径格式是一致的。

## 5.2 用到的外部库 ##
monitor\_server/server用到：

* protobuf库（版本2.5.0）
* mysqlclient库（版本>=5.5.0）
* zlib库（版本>=1.2.0）

redis\_console用到：

* commons-compress-1.8.jar
* commons-fileupload-1.2.2.jar
* commons-io-2.4.jar
* jackson-all-1.6.0.jar
* javax.servlet-api-3.1.0.jar
* jcommon-1.0.21.jar 
* jedis-2.8.1.jar
* jfreechart-1.0.18.jar
* junit-4.12.jar
* log4j-1.2.17.jar
* mysql-connector-java-5.1.38-bin.jar
* org.json.jar
* protobuf-java-2.6.1.jar
* scf4j-props-1.0.1.jar
* slf4j-api-1.7.18.jar
* zookeeper-3.4.8.jar

特别注明：
**如果用于部署，redis\_console子目录中的web\resources\redis.tgz里需要加上redis的二进制服务，包含redis-server和redis-cli。**

# 6、nlb #
## 6.1 简介 ##
nlb是一套网络负载均衡寻址系统。源码在子目录nlb下，使用纯C语言开发。src目录为所有代码，代码主要包含agent和api两部分，配置通过zookeeper集群下发，由web console导入配置到zookeeper集群。

源码目录介绍：

* agent	agent源码
* api		api源码
* comm	通用代码
* tools		工具源码：包含get\_route和show\_servers工具

## 6.2 用到的外部库 ##
* zookeeper-3.4.8
* jansson-2.9

# 7、srpc #
## 7.1 简介 ##
srpc是一个逻辑层rpc框架。子目录spp\_rpc是srpc的源码，使用C++作为主开发语言，支持C++/Python/PHP语言，C++支持微线程。

源码主要目录介绍：

* sync\_frame	微线程源码
* controller		controller进程源码
* proxy		proxy进程源码
* worker		worker进程源码
* rpc			rpc代码
* module/rpc/template		C++自动生成代码源码
* module/rpc/php\_template	PHP自动生成代码源码
* module/rpc/py\_template		Python自动生成代码源码

## 7.2 用到的外部库 ##
* protobuf-2.5.0
* http-parser
* jansson
* json2pb
* libbacktrace
* libunwind
* php
* r3c


# 8、srpc\_java #
## 8.1 简介 ##
srpc\_java是一个逻辑层的java框架, 对应的子目录是msec\_srpc\_java。

源码主要目录介绍：

* srpc     	框架代码
* src          业务示例代码
* bin          框架执行脚本
* lib          构建之后，框架依赖的库
* pom.xml     框架通过maven构建

## 8.2 用到的外部库 ##
* commons-beanutils-1.7.0.jar
* commons-cli-1.2.jar
* commons-collections-3.1.jar
* commons-lang-2.5.jar
* commons-logging-1.1.1.jar
* commons-logging.jar
* ini4j-0.5.1.jar
* junit-3.8.1.jar
* log4j-1.2.17.jar
* netty-3.2.10.Final.jar
* protobuf-java-2.5.0.jar
* protobuf-java-format-1.2.jar

# 9、logsys #

## 9.1 简介 ##

Logsys是msec中日志系统。

源码主要目录介绍：

* api  日志系统提供的API，供srpc框架使用
* flume-ng-mysql-sink    apache flume的插件，使日志数据写入mysql, 通过maven命令行构建
* flume-protobuf-source   apache flume的插件，读取protobuf协议格式的日志数据, 通过maven命令行构建
* proxy  日志系统的查询接口, 通过maven命令行构建

## 9.2 用到的外部库 ##
* protobuf-2.5.0
* commons-beanutils-1.7.0.jar
* commons-cli-1.2.jar
* commons-collections-3.1.jar
* commons-lang-2.5.jar
* commons-logging-1.1.1.jar
* ezmorph-1.0.6.jar
* jackson-core-asl-1.9.4.jar
* jackson-mapper-asl-1.9.4.jar
* json-lib-2.4-jdk15.jar
* junit-3.8.1.jar
* log4j-1.2.17.jar
* mysql-connector-java-5.1.25.jar

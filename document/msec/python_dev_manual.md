# Python开发者手册

**MSEC** 是一个开发+运营解决方案，不只是一个开发框架。支持Java、C/C++、PHP和Python开发语言，其中后三种语言底层框架都是基于C++开发的逻辑层框架。本文主要介绍框架提供给Python开发者使用的接口介绍。

## 框架简介
C++底层框架的实现请参见[cpp_dev_manual.md](https://github.com/Tencent/MSEC/blob/master/document/msec/cpp_dev_manual.md)的[SRPC介绍](https://github.com/Tencent/MSEC/blob/master/document/msec/cpp_dev_manual.md#srpc简介)一节。

## 支持Python原理
SRPC对Python的支持比较简单，使用底层Python API对Python环境做初始化，然后调用Python用户态函数。大致伪码如下：

```c++
Py_Initialize(); 					// 初始化Python环境

while (true)
{
	recv_pkg(request);		       	// 接收来自proxy的请求包
	response = PyEval_CallObject();	// 调用开发者实现的Python用户态函数
	send_pkg(response);				// 回复回复包给proxy
}

Py_Finalize(); 						// 去初始化Python环境
```

## Python环境

### 编译环境
SRPC内部会自带Python的环境，基于Python-2.7.12，配置参数如下：

> ./configure --enable-shared

如果业务开发者需要重新编译Python，请采用对应的兼容版本。

### Python插件
MSEC的监控、日志、寻址等特性都是通过Python扩展的方式给予支持。

## Python编码

### 协议定义
SRPC的协议完全使用google protobuf， Python同样支持progobuf，协议的定义实际上就是一个proto文件。在MSEC中，开发同学只需要在页面上定义好协议即可。
**注意**： 后续章节介绍，示例都采用该协议，文件名service.proto

```protobuf
// 包名, 建议小写 linux 命令风格: echo
package echo;

// 请求消息定义
message EchoRequest
{
    optional bytes message  = 1;
}

// 应答消息定义
message EchoResponse
{
    optional bytes message  = 1;
}

// 定义服务, 建议首字符大写，只允许定义一个
service EchoService
{
    // 定义方法, 可以多份
    rpc EchoTest(EchoRequest) returns (EchoResponse);
}

```

### 生成代码
开发包的rpc/py_template/目录下包含自动生成代码的Python脚本，在MSEC中，可以直接在页面操作自动生成代码包，脚本使用如下：

```
// 用法介绍
Usage: create_rpc.py proto.filename frame_path output_path

// 使用示例
python create_rpc.py service.proto ../../ ./
```

自动生成的代码目录如下：


```
echo_client						// 客户端代码
|-- README.client
|-- echo_client.py				// 自动生成的客户端代码
|-- lib							// Python编译环境依赖的某些库
|-- libsrpc_proto_py_c.so		// SRPC打解包库
|-- msec_impl.py					// msec接口
|-- nlb_py.so					// Python nlb扩展
|-- python						// Python安装目录
|-- python2.7					// Python执行脚本
|-- service.proto				
|-- service_pb2.py				// 业务协议pb文件
|-- srpc.proto					
`-- srpc_comm_py.so				// SRPC通用扩展
echo_server
|-- build.sh						// 创建python发布包的脚本
|-- echo.py						// 自动生成的服务器端代码
|-- entry.py
|-- msec_impl.py					// msec接口
|-- service.proto
|-- service_pb2.py
`-- srpc.proto

```

### 服务器端编码

#### 业务主逻辑实现
在$(package)_server/$(package).py文件下（即echo_server/echo.py），业务可以修改业务代码实现。关注TODO部分:

```python

class EchoService:
    # @brief  自动生成的业务方法实现接口
    # @param  req_data  [入参]业务请求报文，非pb格式，需要转换成pb，也有可能是json字符串
    # @       is_json   [入参]是否为json
    # @return 业务回复报文，pb序列化后的包体或者json字符串
    def EchoTest(self, req_data, is_json):
        json协议处理
        if is_json:
            # TODO: 业务逻辑实现
            return req_data

        自动生成部分，反序列化请求包体
        request = service_pb2.EchoRequest()
        request.ParseFromString(req_data)
        response = service_pb2.EchoResponse()

        # TODO: 业务逻辑实现
        response.message = request.message

        # 序列化回复包体
        return response.SerializeToString()
```

#### http+json支持

上一节的第一个TODO，如果业务请求为http+json格式，需要在这里处理json格式的请求。后续版本会做到json和protobuf处理不做区分，请期待。
http+json的请求报文格式和回复报文格式，参见[cpp_dev_manual.md](cpp_dev_manual.md)的[http+json支持](cpp_dev_manual.md#httpjson支持)一节。

#### 初始化逻辑实现

如果业务需要在进程启动的时候做一些初始化，在进程退出的时候做一些反初始化的逻辑，SRPC提供了如下接口让业务可以实现相关逻辑。相关文件在服务器目录下的[entry.py](https://github.com/Tencent/MSEC/tree/master/spp_rpc/src/module/rpc/py_template/entry.py)。

```python
#
# @brief 业务初始化逻辑，业务可自己实现
#
def init(config):
    print "service::init"
    return 0

#
# @brief 业务反初始化逻辑，业务可自己实现
#
def fini():
    print "service::fini"
    return 0
```

#### 定时任务实现
如果业务需要定时做一些业务逻辑，可以在loop里面实现自己的定时逻辑，自己判断超时。相关文件在服务器目录下的[entry.py](https://github.com/Tencent/MSEC/tree/master/spp_rpc/src/module/rpc/py_template/entry.py)。

```python
def loop():
    return 0
```

#### 扩展引入
SRPC支持Python的egg格式扩展，直接将egg文件放入bin/lib目录即可。在MSEC中，可以通过页面上传的第三方库的方式引入扩展，操作简单。

## Python插件介绍
SRPC提供了四个插件srpc_comm_py,nlb_py,log_py,monitor_py。可以统一调用[msec_impl.py](https://github.com/Tencent/MSEC/tree/master/spp_rpc/src/module/rpc/py_template/msec_impl.py)中的接口实现插件的调用，首先需要import msec_impl,然后才能调用下面的各个接口

### 调用SRPC服务接口

如果业务需要调用其它的后端业务，该业务为MSEC标准服务，则直接可以调用CallMethod接口实现。

```python
#
# @brief 调用其它业务
# @param service_name  业务名，用作寻址，可传入IP地址或业务名("login.web" or "10.0.0.1:1000@udp")
#        method_name   方法名，pb规定的业务名，带namespace("echo.EchoService.EchoTest")
#        request       pb格式请求包
#        timeout       超时时间，单位毫秒
# @return 回复包体+错误信息的数组
# @notice 1. 需要首先判断返回值的 return['ret']是否等于0,0表示成功，其它表示失败，失败可以查看return['errmsg']
#         2. 如果等于0，则可以取出序列化后的报文return['response']
#
def CallMethod(service_name, method_name, request, timeout):
```

### 获取配置接口

如果业务有自己的配置，且为ini格式，则可以直接调用get_config接口获取配置，默认使用系统的配置文件，即../etc/config.ini。

```python
#
# @brief  读取ini配置接口
# @param  session     session名字
# @       key         key名字
# @       filename    配置文件路径
# @return None		  没有相关配置，或者配置文件不存在
# @		  !=None	  字符串类型数据
#
def get_config(session, key, filename='../etc/config.ini'):
```

### 日志打印接口

日志提供设置选项和各个日志级别的打印接口，会打印到本地和远程日志。

```python
#
# @brief 设置日志选项接口
# @parm  key   选项key
# @      val   选项value
#
def log_set_option(key, val):

#
# @brief 打印日志接口
# @param log  打印的日志字符串
#
def log_error(log):
def log_info(log):
def log_debug(log):
def log_fatal(log):

```

### 监控上报接口

监控上报提供两个接口，一个上报即时值，一个上报1分钟内的累加值。

```python
#
# @brief 监控上报累加值
# @param attr   上报属性值
# @      value  即时值，默认1
#
def attr_report(attr, value=1):

#
# @brief 监控上报即时值
# @param attr   上报属性值
# @      value  即时值，默认1
#
def attr_set(attr, value=1):
```

### 路由接口

路由接口包含获取路由(getroute)和更新路由(updateroute)两个接口，更新路由用于回包统计，根据请求信息做路由策略。

```python
#
# @brief 获取路由接口
# @param service_name   业务名，msec中通过web_console设置的两级业务名，如"Login.ptlogin"
# @return =Null         获取路由失败
# @       !=Null        路由信息，返回类型为词典,如:{'ip':'10.0.0.1'; 'port': 7963; 'type': 'tcp'}
#
def getroute(service_name):

#
# @brief  更新路由信息
# @param service_name       业务名，msec中通过web_console设置的两级业务名，如"Login.ptlogin"
# @      ip                 IP地址
# @      failed             业务网络调用是否失败
# @      cost               如果成功，需要上报时延，暂时没有对时延上报做路由策略，所以，可以设置为0
#
def updateroute(service_name, ip, failed, cost):

```

### cgi调用接口

如果一个非MSEC的业务需要调用MSEC业务，可以使用下面的打解包接口。对于机器上有安装的nlbagent的机器，可以通过[路由接口](#路由接口)一节提到的接口获取后端服务器IP、Port。

```python
#
# @brief SRPC序列化报文接口
# @param methodname     方法名，"echo.EchoService.EchoTest"
# @      body           包体，pb序列化后的包体
# @      seq            本次收发包的序列号，用于判断是否串包
# @return =None         打包错误
#         !=None        打包完后的二进制数据
#
def srpc_serialize(methodname, body, seq):

#
# @brief SRPC反序列化报文接口
# @param pkg         报文包体
# @return 返回值为词典类型，需要判断 ret['errmsg'] == 'success'来判断是否成功
# @       如果成功，返回值为{'errmsg':'success', 'seq':100, 'body': 'hello world'}
#
def srpc_deserialize(pkg):

#
# @brief SRPC检查报文是否完整接口
# @param pkg    报文包体
# @return <0    非法报文
# @       ==0   报文不完整
# @       >0    报文完整，长度为返回值
#
def srpc_check_pkg(pkg):

```

## 框架配置

配置同c++，详见cpp_dev_manual.md中的[SRPC配置说明](cpp_dev_manual.md#srpc配置说明)一节。









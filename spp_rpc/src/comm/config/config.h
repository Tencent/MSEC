
/**
 * Tencent is pleased to support the open source community by making MSEC available.
 *
 * Copyright (C) 2016 THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the GNU General Public License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. You may 
 * obtain a copy of the License at
 *
 *     https://opensource.org/licenses/GPL-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the 
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions
 * and limitations under the License.
 */


#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <signal.h>
#include <string>
#include <vector>
#include <map>
#include "inifile.h"
#include "tlog.h"

using namespace tbase::tlog;
using namespace std;

#define SPP_GROUP "SRPC"
#define NLB_GROUP "NLB"
#define LOG_GROUP "LOG"

// 日志配置
struct Log
{
    int level;       // 日志等级
    int type;        // 日志类型
    int maxfilesize; // 日志文件大小
    int maxfilenum;  // 日志文件个数

    Log():level(tbase::tlog::LOG_ERROR), type(tbase::tlog::LOG_TYPE_CYCLE), maxfilesize(10240000), maxfilenum(10) {}
};

// 监听地址
struct Listen
{
    std::string type;       // 监听地址类型  TCP/UDP
    std::string intf;       // 接口地址      eth0/eth1...
    int         port;       // 监听端口
    int         oob;        // 是否打开oob

    Listen(): port(-1), oob(0) {}
};

// 所有配置信息
struct Config
{
    std::vector<Listen> listens;    // 监听地址
    //std::vector<std::string> nlb_preload;
    std::string         service;    // 业务名
    std::string         module;     // 业务so
    std::string         conf;       // 业务配置文件路径
    Log  log;                       // 日志
    int  timeout;                   // 空闲连接超时时间 单位: 秒
    int  msg_timeout;               // 消息过载保护事件 单位: ms
    int  global;                    // dlopen 标志 RTLD_GLOBAL
    int  procnum;                   // 进程数
    int  shmsize;                   // 共享内存大小
    int  heartbeat;                 // 心跳时间
    int  reload;                    // 热加载标记

    Config(): module("./msec.so"), timeout(60), msg_timeout(800), global(1), procnum(4), shmsize(16), heartbeat(60), reload(0) {}
};

// 配置读取类
class ConfigLoader
{
public:
    ConfigLoader()  {};
    ~ConfigLoader() {};

    void SetDefaultListen(void);
    void SetDefaultService(void);
    void SetDefaultProcnum(void);
    int Init(const char *filename);
    int Reload(const char *filename);
    Config &GetConfig(void);

private:
    Config      config_;

public:
    static string filename_;
};

// 获取业务名
// "/msec/s1/s2/bin", 生成"s1.s2"
int GetServiceName(string &name);

int GetConfig(const string &session, const string &key, string &val);

#if 0
// 全局配置
struct Global
{
    std::vector<Listen> listens;  // 监听地址
    std::string         service;  // 业务名
    int  timeout;    // 空闲连接超时时间 单位: 秒
    int  TOS;        // 是否打开TOS
    bool udpclose;   // udp回包关闭标志

    Global(): timeout(60), TOS(0), udpclose(true) {}
};

// controller配置
struct CtrlConf
{
    Log  flog;       // 日志
};

// proxy配置
struct ProxyConf
{
    int interval;       // 上报心跳时间间隔
    int global;         // dlopen 标志 RTLD_GLOBAL
    int maxconn;        // 最大连接数
    int maxpkg;
    int sendcache_limit;
    Log flog;           // 框架日志
    Log log;            // 业务日志

    std::string module; // 业务so
    std::string result;
    std::string conf;   // 业务私有配置文件路径
    std::string type;   // 

    ProxyConf(): interval(20), global(0), maxconn(100000), sendcache_limit(-1) {}
};

// worker配置
struct WorkerConf
{
    int id;         // worker组ID
    int procnum;    // 进程数
    int interval;   // 上报心跳时间间隔
    int reload;     // 热重启标记
    int global;     // dlopen 标志 RTLD_GLOBAL
    int shmsize;    // 共享内存大小 单位:MB
    int timeout;    // 过载时间 单位:ms
    int TOS;        // 后端交互是否打开TOS
    int exitsignal; // 退出信号

    Log flog;       // 框架日志
    Log log;        // 业务日志

    std::string module;
    std::string result;
    std::string conf;
    std::string type;

    WorkerConf(): id(-1), procnum(0), interval(20), reload(0), global(0), shmsize(16), timeout(800), exitsignal(SIGUSR1) {}
};

typedef std::map<int, WorkerConf> WorkersConf;

struct Config
{
    Global      global;
    CtrlConf    ctrl;
    ProxyConf   proxy;
    WorkersConf workers;
};

class ConfigLoader
{
public:
    ConfigLoader()  {};
    ~ConfigLoader() {};

    int Init(const char *filename);
    int Reload(void);

public:
    std::string filename_;
    Config      config_;
};

#endif


#endif


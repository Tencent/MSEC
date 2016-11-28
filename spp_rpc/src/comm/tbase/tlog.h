
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


#ifndef _TBASE_TLOG_H_
#define _TBASE_TLOG_H_
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
/**********************************************************
当多线程共享tlog对象的时候,需要使用_THREAD_SAFE编译本组件
避免在切换文件的时候发生混乱
***********************************************************/
#ifdef _THREAD_SAFE
#include <pthread.h>
#endif

//#define TEST_LOG
#define  INTERNAL_LOG  SingleTon<CTLog, CreateByProto>::Instance()

/**********************************************************
当多进程或者多线程分别使用独立的tlog对象但是相同的配置时要使用_MP_MODE编译本组件
避免在切换文件的时候发生混乱
***********************************************************/
#ifdef _MP_MODE
#endif

#define DEFAULT_MAX_FILE_SIZE  (1 << 30)     //最大文件SIZE为1G
#define DEFAULT_MAX_FILE_NO    1000          //默认最大文件编号
#define MAX_PATH_LEN           256           //最大路径长度
#define MAX_LOG_LEN            4096          //一次日志最大长度
#define LOG_FLAG_TIME          0x01          //打印时间戳
#define LOG_FLAG_TID           0x02			 //打印线程ID
#define LOG_FLAG_LEVEL         0x04			 //打印日志级别
#define ERR_FD_NO              0x00          //初始文件句柄号，默认是标准输出

namespace tbase
{
namespace tlog
{

//日志类型
enum LOG_TYPE
{
    LOG_TYPE_CYCLE = 0,
    LOG_TYPE_DAILY,
    LOG_TYPE_HOURLY,
    LOG_TYPE_CYCLE_DAILY,
    LOG_TYPE_CYCLE_HOURLY,

#ifdef TEST_LOG
    LOG_TYPE_CYCLE_MIN,
    LOG_TYPE_MIN
#endif

};
//日志级别
enum LOG_LEVEL
{
    LOG_TRACE = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_ERROR,
    LOG_FATAL,
    LOG_NONE    //当要禁止写任何日志的时候,可以设置tlog的日志级别为LOG_NONE
};

//钩子函数原型
//fmt:		格式字符串
//ap:       可变参数集合
//返回值:   返回0表示不记录到日志文件,否则记录到日志文件
typedef int (*log_hook)(const char *fmt, va_list ap);

//日志类
class CTLog
{
public:
    CTLog();
    ~CTLog();

    //初始化日志
    //log_level:		日志级别
    //log_type:			日志类型
    //log_path:			日志存放目录
    //name_preifx:		日志文件名前缀
    //max_file_size:	每个日志文件的最大长度
    //max_file_no:		日志文件最大个数
    //返回值:			0成功,否则失败
    int log_open(int log_level, int log_type, const char* log_path, const char* name_prefix, int max_file_size, int max_file_no);
    //设置日志级别
    //level:		新的日志级别
    //返回值:		老的日志级别
    int log_level(int level);
    //打印格式化日志
    void log_i(int flag, int log_level, const char *fmt, ...);
    void log_i_va(int flag, int log_level, const char* fmt, va_list va);
    //打印bin日志
    void log_i_bin(int log_level, const char* buf, int len);
    //设置钩子函数
    void log_set_hook(log_hook hook);
    //把二进制数据转换为可显示的hex字符串
    //bin:		二进制数据
    //len:      数据长度
    //buf:      字符串缓冲区
    //size:		字符串缓冲区的大小
    //返回值:   字符串指针
    static const char* to_hex(const char* bin, int len, char* buf, int size);
protected:
    int log_level_;
    int log_type_;
    char log_path_[MAX_PATH_LEN];
    char name_prefix_[MAX_PATH_LEN];
    int max_file_size_;
    unsigned int max_file_no_; //unsigned

    int log_fd_;
    int cur_file_size_;
    unsigned int cur_file_no_; //unsigned
    time_t pre_time_;
    char filename_[MAX_PATH_LEN];
    char buffer_[MAX_LOG_LEN];
    log_hook hook_;

    void close_file();
    void log_file_name(char* filepath, char* filename);
    int open_file();
    void init_cur_file_no();
    void update_cur_file_no();
    unsigned int find_max_file_no(const char* filename);
    //void get_cur_file_no();
    void force_rename(const char* src_pathname, const char* dst_pathname);
    void remove_file(const char* filename);
    int shift_file();	//按日志滚动周期切换日志文件

    void get_time(int& buff_len);
    void get_tid(int& buff_len);
    //void get_file(int& buff_len);
    void get_level(int& buff_len, int log_level);
#ifdef _THREAD_SAFE
    class CLogLock
    {
    public:
        CLogLock()
        {
            pthread_mutex_lock(&mutex_);
        }
        ~CLogLock()
        {
            pthread_mutex_unlock(&mutex_);
        }
    private:
        static pthread_mutex_t mutex_;
    };
#endif
};
// namespace
}
}
/***********************************************
用户使用应该调用如下跟函数对应的宏
************************************************/
#define LOG_OPEN						log_open
#define LOG_P(lvl, fmt, args...)  		log_i(LOG_FLAG_TIME, lvl, fmt, ##args)
#define LOG_P_NOTIME(lvl, fmt, args...) log_i(0, lvl, fmt, ##args)
#define LOG_P_PID(lvl, fmt, args...) 	log_i(LOG_FLAG_TIME | LOG_FLAG_TID, lvl, fmt, ##args)
#define LOG_P_LEVEL(lvl, fmt, args...)	log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL, lvl, fmt, ##args)
#define LOG_P_FILE(lvl, fmt, args...)	log_i(LOG_FLAG_TIME, lvl, "[%-10s][%-4d][%-10s]"fmt, __FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG_P_ALL(lvl, fmt, args...)	log_i(LOG_FLAG_TIME | LOG_FLAG_LEVEL | LOG_FLAG_TID, lvl, "[%-10s][%-4d][%-10s]"fmt,__FILE__, __LINE__, __FUNCTION__, ##args)
#define LOG_P_BIN						log_i_bin
#define SET_HOOK   						log_set_hook

#endif

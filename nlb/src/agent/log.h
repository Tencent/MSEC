
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


/**
 * @filename log.h
 */

#ifndef _LOG_H__
#define _LOG_H__

enum {
    ERROR = 1,
    WARN  = 2,
    INFO  = 3,
    DEBUG = 4,
};

void setLogLevel(int level);

int getLogLevel(void);

void currentTime2Str(char * str, int max_len);

void logger(const char * fmt, ...);

#define NLOG_ERROR(fmt, args...) do { \
        if (getLogLevel() >= ERROR) { \
            char TimeStr[64];\
            currentTime2Str(TimeStr, sizeof(TimeStr)); \
            logger("[%s] [ERROR] [%s] [%d]"fmt"\n", TimeStr, __FILE__, __LINE__, ##args); \
        }\
    } while (0) 

#define NLOG_WARN(fmt, args...) do { \
        if (getLogLevel() >= WARN) { \
            char TimeStr[64];\
            currentTime2Str(TimeStr, sizeof(TimeStr)); \
            logger("[%s] [WARN ] [%s] [%d]"fmt"\n", TimeStr, __FILE__, __LINE__, ##args); \
        }\
    } while (0) 

#define NLOG_INFO(fmt, args...) do { \
        if (getLogLevel() >= INFO) { \
            char TimeStr[64];\
            currentTime2Str(TimeStr, sizeof(TimeStr)); \
            logger("[%s] [INFO ] [%s] [%d]"fmt"\n", TimeStr, __FILE__, __LINE__, ##args); \
        }\
    } while (0) 

#define NLOG_DEBUG(fmt, args...) do { \
        if (getLogLevel() >= DEBUG) { \
            char TimeStr[64];\
            currentTime2Str(TimeStr, sizeof(TimeStr)); \
            logger("[%s] [DEBUG] [%s] [%d]"fmt"\n", TimeStr, __FILE__, __LINE__, ##args); \
        }\
    } while (0) 


#endif




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


#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "logsys_api.h"

using namespace std;

int main(int argc, char* argv[]) {
    msec::LogsysApi* log_api = msec::LogsysApi::GetInstance();
    std::map<std::string, std::string>  headers;

    int num = 1;
    if (argc > 1 && atoi(argv[1]) > 1)
        num = atoi(argv[1]);

    srand(time(NULL));
    int  random = 0;
    char buff[64];
    char body[256];
    for (int i = 0; i < num; ++i)
    {
        random = rand();
        snprintf(buff, sizeof(buff), "%d", random);
        headers["ReqID"] = buff;
        headers["IP"] = "26.19.3.243";
        headers["ServerIP"] = "8.8.8.8";
        headers["ClientIP"] = "8.8.8.8";
        headers["Level"] = "ERROR";

        random = rand();
        snprintf(buff, sizeof(buff), "rpc%d", random%1000);
        headers["RPCName"] = buff;

        random = rand();
        snprintf(buff, sizeof(buff), "line%d", random % 100);
        headers["FileLine"] = buff;
        headers["Other"] = "OtherValue";

        
        for (int k = 0; k < (int)sizeof(body) - 1; ++k)
            body[k] = rand() % 26 + 'a';
        body[sizeof(body) - 1] = '\0';
        log_api->Log(msec::LogsysApi::ERROR, headers, body);
    }
    
    //用户接口，代码简洁
    random = rand();
    snprintf(buff, sizeof(buff), "%d", random);
    //msec::LogSetHeader("ReqID", buff).LogSetHeader("IP", "26.19.3.243").LogSetHeader("ServerIP", "8.8.8.8");
    //msec::LOG(msec::LogsysApi::ERROR, "%s", "something");
    return 0;
}


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


#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "frozen.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// send successful response to server
// with file length information
static void returnResultJsonString(int sock, off_t sz)
{
        char jsonStr[1024];
	char lenStr[100];

	snprintf(jsonStr+10, sizeof(jsonStr)-10, "{\"status\":0, \"message\":\"success\", \"fileLength\":%d}", sz);
	int len = strlen(jsonStr+10);

	snprintf(lenStr, sizeof(lenStr), "%-10d", len);
	memcpy(jsonStr, lenStr, 10);

	logger("write resultJsonStr:%d,%s\n", len+10, jsonStr);	
        write(sock, jsonStr, len+10);
}


// the main entry of this action
void GetFileFromAgent(int sock, const char * jsonStr, int jsonStrLen)
{

    // request example:
    // {"handleClass":"GetFileFromAgent", "requestBody":{"fileFullName":"d.doc"}}

	struct json_token * obj, *requestBody, *fileFullName;
	char filename[1024];

    // get request body field in json string
    obj = parse_json2(jsonStr, jsonStrLen);
    if (obj == NULL)
    {
        goto l_end;
    }
	requestBody = find_json_token(obj, "requestBody");
	if (requestBody == NULL || requestBody->type != JSON_TYPE_OBJECT)
	{
		returnErrorMessage(sock, "can NOT find requestBody element in json string");
        goto l_end;
	}

    // get file name
    fileFullName =  find_json_token(requestBody, "fileFullName");
    if ( fileFullName == NULL || fileFullName->type != JSON_TYPE_STRING || fileFullName->len >= sizeof(filename) )
    {
		returnErrorMessage(sock, "can NOT find fileFullName element in json string");
        goto l_end;
    }
	memcpy(filename, fileFullName->ptr, fileFullName->len);
	filename[fileFullName->len] = 0;

	logger("fileFullName:%s\n", filename);


    // Is the directory to save this file exists?
    // if not, create it
	checkFileDirExist();




	int max_bytes = 800000000; // max file size allowed is 800M
    // get file properties
    struct stat st;
    if (stat(filename, &st) != 0)
    {
		returnErrorMessage(sock, "can not get file state");
        goto l_end;
    }
    if (!(S_ISREG(st.st_mode) || S_ISLNK(st.st_mode))  )
    {
		returnErrorMessage(sock, "is no a regular file or symbolink");
        goto l_end;
    }
    if  (st.st_size > max_bytes)
    {
		returnErrorMessage(sock, "is is too large, maximum size is 800M");
        goto l_end;
    }
    
    // return discriptional information back in json string format
	returnResultJsonString(sock, st.st_size);

    // send back file content in succession
	int fd = open(filename, O_RDONLY);
	if (fd < 0)
	{
		returnErrorMessage(sock, "failed to open file");
        goto l_end;
	}
	logger("open file success:%s\n", filename);
	int total_bytes = 0;
	char buf[10240];
	int len;
	while (total_bytes < max_bytes)
	{
		len = read(	fd, buf, sizeof(buf));
		if (len <= 0 )
		{
			break;
		}
		total_bytes += len;
		write(sock, buf, len);
	}
	close(fd);
    if (total_bytes != st.st_size)
    {
	    logger("read file failed, only read %d bytes\n", total_bytes);
    }
    else
    {
	    logger("read file success, size=%d\n", total_bytes);
    }


l_end:
	if (obj != NULL) { free(obj);}	
	return;
}

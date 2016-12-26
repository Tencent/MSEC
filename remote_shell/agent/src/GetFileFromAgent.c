
/*
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
#include "my_rsa.h"
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <log.h>
#include <openssl/sha.h>

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

extern RSA * rsa_key;

int checkTimestamp(const unsigned char * hashBytes);

// decrypt the signature field with public key,
// the plain text is digest of command file(32B) and timestamp(32B)
static int getHashFrmSignature(const char * sig, char * hashStr, int maxLen)
{
	unsigned char sigBytes[1024];
	int sigBytesNum = sizeof(sigBytes);

	if (hexStr2bytes(sig, sigBytes, &sigBytesNum) != 0)
	{
		logger("hexStr2bytes() failed!\n");
		return -1;
	}
	logger("signature byte number:%d\n", sigBytesNum);

	unsigned char hashBytes[1024];
	int hashBytesNum = decrypt(sigBytes, sigBytesNum, hashBytes, rsa_key);
	if (hashBytesNum <= 0)
	{
		logger("decrypt() failed!\n");
		return -1;
	}
	// hashBytes = file digest(32B) + timestamp(32)
	logger("hash bytes number:%d\n", hashBytesNum);

	if (checkTimestamp(hashBytes))
	{
		logger("signature is not fresh, timeout!");
		return -1;
	}


	hashBytesNum = 32; // cut off the timestamp

	if (bytes2hexStr(hashBytes,  hashBytesNum, hashStr, maxLen) < 0)
	{
		logger("bytes2hexStr() failed!\n");
		return -1;
	}
	logger("hash hex string:%s\n", hashStr);
	return 0;
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


	// check the signature
	if (rsa_key != NULL)
	{
		// get signature field from json string
		struct json_token * signature;
		char sigStr[1024];
		signature=  find_json_token(requestBody, "signature");
		if ( signature == NULL || signature->type != JSON_TYPE_STRING || signature->len >= sizeof(sigStr) )
		{
			returnErrorMessage(sock, "signature invalid");
			goto l_end;
		}
		memcpy(sigStr, signature->ptr, signature->len);
		sigStr[signature->len] = 0;
		logger("sigature:%s\n", sigStr);

		// decrypt it and the plain text is file content digest
		char hashStr[1024];
		if (getHashFrmSignature(sigStr, hashStr, sizeof(hashStr)) != 0)
		{
			returnErrorMessage(sock, "decrypt signature failed");
			goto l_end;
		}

		// calculate the digest of the file name actually recved
		unsigned char fileNameHash[SHA256_DIGEST_LENGTH];
		if (getHashOfByteArray(fileFullName->ptr,
							   fileFullName->len,
							   fileNameHash) != 0)
		{
			returnErrorMessage(sock, "calc hash of file failed!");
			goto l_end;
		}
		char hashStr2[1024];
		if (bytes2hexStr(fileNameHash,  SHA256_DIGEST_LENGTH, hashStr2, sizeof(hashStr2)) < 0)
		{
			returnErrorMessage(sock, "fail to convert file hash to hexstring");
			goto l_end;
		}
		logger("file name hash:%s\n", hashStr2);

		// compare
		// if not equal, refuse the request
		if (strcmp(hashStr2, hashStr) != 0)
		{
			returnErrorMessage(sock, "signature invalid");
			goto l_end;
		}
	}


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

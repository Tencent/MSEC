
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
#include "log.h"
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
#include <unistd.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include "my_rsa.h"

extern RSA * rsa_key;

int checkTimestamp(const unsigned char * hashBytes);

static void returnResultJsonString(int sock)
{
        char jsonStr[1024];
	char lenStr[100];

	strcpy(jsonStr+10, "{\"status\":0, \"message\":\"success\"}");
	int len = strlen(jsonStr+10);

	snprintf(lenStr, sizeof(lenStr), "%-10d", len);
	memcpy(jsonStr, lenStr, 10);

	logger("write resultJsonStr:%d,%s\n", len+10, jsonStr);	
        write(sock, jsonStr, len+10);
}
// decrypt the signature field with public key,
// the plain text is digest of  file and timestamp
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


void SendFileToAgent(int sock, const char * jsonStr, int jsonStrLen)
{
	struct json_token * obj, *requestBody, *fileFullName, *fileLength;
	char fileLengthStr[30];
	char filename[1024];

    filename[0] = 0;

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


    // get file name from json string

    fileFullName =  find_json_token(requestBody, "fileFullName");
    if ( fileFullName == NULL || fileFullName->type != JSON_TYPE_STRING || fileFullName->len >= sizeof(filename) )
    {
		returnErrorMessage(sock, "can NOT find fileFullName element in json string");
        goto l_end;
    }
	memcpy(filename, fileFullName->ptr, fileFullName->len);
	filename[fileFullName->len] = 0;
	logger("fileFullName:%s\n", filename);


    // get file length from json string

    fileLength =  find_json_token(requestBody, "fileLength");
    if ( fileLength == NULL || fileLength->type != JSON_TYPE_NUMBER || fileLength->len >= sizeof(fileLengthStr) )
    {
		returnErrorMessage(sock, "can NOT find fileLength element in json string");
        goto l_end;
    }
    memcpy(fileLengthStr, fileLength->ptr, fileLength->len);
    fileLengthStr[fileLength->len] = 0;
    int fileLengthInt = atoi(fileLengthStr);

    if (fileLengthInt > 100000000)
    {
		returnErrorMessage(sock, "file is too large, max 100M allowed");
        goto l_end;
    }


	checkFileDirExist();


	int fd = open(filename, O_TRUNC|O_CREAT|O_WRONLY, S_IRUSR|S_IWUSR);
	if (fd < 0)
	{
		returnErrorMessage(sock, "failed to open file");
        goto l_end;
	}
	logger("open file success:%s\n", filename);
	int total_bytes = 0;
	char buf[10240];
	int len;

    // recv the file content from socket and save it into file
	while (total_bytes < fileLengthInt)
	{
		len = read(	sock, buf, sizeof(buf));
		if (len <= 0 )
		{
			break;
		}
		total_bytes += len;
		write(fd, buf, len);
	}
	close(fd);

    if (total_bytes != fileLengthInt)
    {
		returnErrorMessage(sock, "file length mismatch");
        logger("file length mismatch, %d, %d\n", total_bytes, fileLengthInt);
        goto l_end;
    }
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

        // calculate the digest of the file actually recved 
        unsigned char fileHash[SHA256_DIGEST_LENGTH];
        if (getHash(filename, fileHash) != 0)
        {
		    returnErrorMessage(sock, "calc hash of file failed!");
            goto l_end;
        }
        char hashStr2[1024];
        if (bytes2hexStr(fileHash,  SHA256_DIGEST_LENGTH, hashStr2, sizeof(hashStr2)) < 0)
        {
		    returnErrorMessage(sock, "fail to convert file hash to hexstring");
            goto l_end;
        }
        logger("file hash:%s\n", hashStr2);

        // compare 
        // if not equal, refuse the request
        if (strcmp(hashStr2, hashStr) != 0)
        {
		    returnErrorMessage(sock, "signature invalid");
            goto l_end;
        }
    }

	returnResultJsonString(sock);

	if (obj != NULL) { free(obj);}	
	return;


l_end:
	if (obj != NULL) { free(obj);}	
    if (filename[0] != 0) { unlink(filename); }
	return;
}


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
#include <openssl/sha.h>   
#include <openssl/crypto.h>  

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
#include <time.h>
#include <sys/time.h>
#include "my_rsa.h"


// send success response back to server
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

extern RSA * rsa_key;

int checkTimestamp(const unsigned char * hashBytes)
{
    char timestamp[33];
    int i;
    for (i = 0; i < 32; ++i)
    {
        timestamp[i] = hashBytes[32+i];
    }
    timestamp[32] = 0;
    //trim
    for (i = 31; i >=0 ; i--)
    {
        if (timestamp[i] == ' ') {timestamp[i] = 0;}
    }
    int serverTime = atoi(timestamp);

    //check if timestamp is fresh
    int currentTime = time(NULL);
    logger("check time:%d vs %d\n", serverTime, currentTime);
    if (abs(serverTime-currentTime) < (1*3600))
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

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
void SendCmdsToAgentAndRun(int sock, const char * jsonStr, int jsonStrLen)
{

    // request example:
    // {"handleClass":"SendCmdsToAgentAndRun", "requestBody":{"cmdFileLength":8, 
    //    "outputFileName":"output_1375455199.txt", "signature":"7294282...."}

    char cmdFileName[255];
    cmdFileName[0] = 0;


	struct json_token * obj, *requestBody, *outputFileName, *fileLength;
	char outFileName[1024];
	char fileLengthStr[30];

    // parse request json string 
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

    // the name of file to save the result of cmd execution 
    outputFileName =  find_json_token(requestBody, "outputFileName");
    if ( outputFileName == NULL || outputFileName->type != JSON_TYPE_STRING || outputFileName->len >= sizeof(outFileName) )
    {
		returnErrorMessage(sock, "can NOT find outputFileName element in json string");
        goto l_end;
    }
	memcpy(outFileName, outputFileName->ptr, outputFileName->len);
	outFileName[outputFileName->len] = 0;
	logger("outputFileName:%s\n", outFileName);

    // get command file length from json string
    fileLength =  find_json_token(requestBody, "cmdFileLength");
    if ( fileLength == NULL || fileLength->type != JSON_TYPE_NUMBER || fileLength->len >= sizeof(fileLengthStr) )
    {
		returnErrorMessage(sock, "can NOT find cmdFileLength element in json string");
        goto l_end;
    }
    memcpy(fileLengthStr, fileLength->ptr, fileLength->len);
    fileLengthStr[fileLength->len] = 0;
    int fileLengthInt = atoi(fileLengthStr);
    logger("cmd fileLength:%d\n", fileLengthInt);

    if (fileLengthInt > 100000000)
    {
		returnErrorMessage(sock, "file is too large, max 100M allowed");
        goto l_end;
    }


    // Does the directory to save file exist?
    // if not, create it.
	checkFileDirExist();

	char cmd[1024];

    // cmd file full path
    snprintf(cmdFileName, sizeof(cmdFileName), "/tmp/cmd_%d.sh", getFileDir(),  getRandUInt());


    // cmd that will be executed
	snprintf(cmd, sizeof(cmd), "%s >%s 2>&1", cmdFileName, outFileName);

    // recv file content from socket, and save it into cmd file
	int fd = open(cmdFileName, O_TRUNC|O_CREAT|O_WRONLY, S_IRWXU);
	if (fd < 0)
	{
		returnErrorMessage(sock, "failed to open cmds file");
        goto l_end;
	}
	logger("open file success:%s\n", cmdFileName);
	int total_bytes = 0;
	char buf[10240];
	int len;
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
        logger("file length mismatch, %d, %d\n", total_bytes, fileLengthInt);
		returnErrorMessage(sock, "file length mismatch");
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
        if (getHash(cmdFileName, fileHash) != 0)
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
        logger("cmd file hash:%s\n", hashStr2);

        // compare 
        // if not equal, refuse the request
        if (strcmp(hashStr2, hashStr) != 0)
        {
		    returnErrorMessage(sock, "signature invalid");
            goto l_end;
        }
    }

    // execute the command file
    int pid = fork();
    if (pid == 0)
    {
        // do Not inherit the opened fd
        int i; 
        for (i = 0; i < 1024; ++i)
        {
            close(i);
        }
        system(cmd); // this will block current process until cmd finishs
        exit(0);
    }

    waitpid(pid, NULL, 0); // wait the child process exiting

	returnResultJsonString(sock);
	if (obj != NULL) { free(obj);}	
	return;


l_end:
	if (obj != NULL) { free(obj);}	
    if (cmdFileName[0] != 0) {unlink(cmdFileName);}
	return;
}

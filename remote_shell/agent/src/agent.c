
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


/*
 * agent main entry
 */
#include <stdio.h>
#include <openssl/pem.h>
#include "log.h"
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <bits/waitflags.h>
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
#include "openssl/rsa.h"
#include "my_rsa.h"




#include <string.h>
#include <asm/errno.h>
#include <errno.h>

static void do_processing(int sock);
static void daemon_init();
static void sig_handle(int sig);






char currentDir[255];
static char srv_ip1[24];
static char srv_ip2[24];

/*
 *  get the directory of current exe file
 *  file transported and log file are all relative to the directory
 */
static int getWorkingDir()
{
	int len = readlink ("/proc/self/exe", currentDir, sizeof(currentDir) );  
	if (len <= 0)
	{
		perror("can NOT locate the director of this agent.\n");
		return -1;
	}

	char * c = strrchr(currentDir, '/');
	if (c == NULL)
	{
		fprintf(stderr, "can NOT locate / at %s\n", currentDir);
		return -1;
	}
	*c = 0;
	return 0;
}

int bindAndListen()
{
	int socket_fd;
	struct sockaddr_in svr_addr;
	if(( socket_fd = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP)) < 0 ){  
		perror("socket() failed");  
		return -1;
	}  
	memset(&svr_addr,0,sizeof(svr_addr));  
	svr_addr.sin_family = AF_INET;  
	svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);  
	svr_addr.sin_port = htons(9981);  
  
	if( bind(socket_fd,(struct sockaddr*) &svr_addr,sizeof(svr_addr)) < 0 ) {  
		perror("bind() failed");
		return -1;
	}  
  
	if( listen(socket_fd,10) < 0 ) {  
		perror( "listen() failed");  
		return -1;
	}  
	return socket_fd;
}

/*
 * for security reason, only accept the requests from two servers
 */
static checkCliIP(const struct sockaddr_in * addr)
{
	char ip[24];

	strncpy(ip, inet_ntoa(addr->sin_addr), sizeof(ip));
	if ((strncmp(ip, srv_ip1, sizeof(ip)) == 0) ||
			strncmp(ip, srv_ip2, sizeof(ip) ) == 0)
	{
		return 0;
	}
	else
	{
		return -1;
	}
	
}

RSA *rsa_key = NULL;

static char * proc_name = NULL;
static int proc_name_max_len = 0;

void change_proc_name(const char * new_name)
{
    if (proc_name != NULL && strlen(new_name) < proc_name_max_len)
    {
        memset(proc_name, 0, proc_name_max_len);
        strncpy(proc_name, new_name, proc_name_max_len );
    }
}

void setNonblock(int sockfd)
{
	int flag = fcntl(sockfd, F_GETFL, NULL);
	fcntl(sockfd, F_SETFL, flag | O_NONBLOCK);
}
static int acceptWithTMO(int sockfd, struct sockaddr* addr, socklen_t  *len)
{
	fd_set fds;
	int n;
	struct timeval tv;

	FD_ZERO(&fds);
	FD_SET(sockfd, &fds);

	tv.tv_sec = 0;
	tv.tv_usec = 100000;

	n = select(sockfd+1, &fds, NULL, NULL, &tv);
	if (n == 0){
		return -2; // timeout!
	}
	if (n < 0){
		if (errno == EINTR)
		{
			return -2;
		}

		return -1; // error
	}
	int fd = accept(sockfd, addr, len);
	if (fd < 0)
	{
		return -1;
	}
	return fd;
}
static void checkAgentProcess()
{
	static time_t last_check_time = 0;
	time_t now = time(NULL);

	if (now - last_check_time < 10)
	{
		return;
	}

	last_check_time = now;

	//char cmd[] = "cnt=`ps auxw|grep -e 'java.*flume.node.Application' -e 'monitor_agent.*conf' -e 'nlbagent.*mix.*\\-i'|grep -v grep|wc -l` ; if [ $cnt -lt 3 ]; then cd /ngse/agent;./stop.sh;./start.sh;  fi";
	//system(cmd);
}
static void doLoop()
{
	printf("doLoop...\n");
}


int main(int argc, char *argv[])
{
    int n, pid;
	int sockfd;
	int newsock;

    proc_name = argv[0];
    proc_name_max_len = strlen(argv[0]);



    // if public key file exists, then load it
    // and the server identity will be checked ,
    //  so we can avoid hacker attacking
    if (access("./pub.txt", R_OK|F_OK) == 0)
    {
        rsa_key = loadKeyFromFile("./pub.txt");
        if (rsa_key == NULL)
        {
            return -1;
        }
        printf("load rsa public key from pub.txt\n");
    }

	if (getWorkingDir())
	{
		return -1;
	}	

	if (argc != 3)
	{
		fprintf(stderr, "usage: %s srv_ip1 srv_ip2\n", argv[0]);
		return -1;
	}
	strncpy(srv_ip1, argv[1], sizeof(srv_ip1) );
	strncpy(srv_ip2, argv[2], sizeof(srv_ip2) );

    daemon_init();

    signal(SIGCHLD, sig_handle);

    // create tcp listening socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	setNonblock(sockfd);
	sockfd = bindAndListen();
	if (sockfd < 0) {return -1;}

    while (1) {
			struct sockaddr_in cliaddr;
			socklen_t addrlen = sizeof(cliaddr);

			newsock = acceptWithTMO(sockfd, (struct sockaddr *)&cliaddr, &addrlen);
			if (newsock < 0)
			{
				if (newsock == -2)// time out
				{
					doLoop();
				}
				else{
					perror("acceptWithTMO:");
				}

				continue;

			}

			if (checkCliIP(&cliaddr) != 0)
			{
				close(newsock);
				continue;
			}


	        /* Create child process */
	        pid = fork();
	
	        if (pid < 0) {
	            perror("ERROR on fork");
	            exit(1);
	        }
	
	        if (pid == 0) {
	            /* This is the client process */
                change_proc_name("doWork");
	            close(sockfd);
	            do_processing(newsock);
	            exit(0);
	        } else {
	            close(newsock);
	        }
    }                            /* end of while */
}

static void daemon_init()
{
    pid_t pid;
    int i;

    if ((pid = fork()) != 0) {
        exit(0);
    }

    setsid();

    if ((pid = fork()) != 0) {
        exit(0);
    }

	/*
    chdir("/");

    for (i = 0; i <= 2; i++) {
        close(i);
    }
	*/
}

static void sig_handle(int sig)
{
    if (sig == SIGCHLD) {
        int status;
        waitpid(-1, &status, WNOHANG);
    }
}

// request = jsonStrLength(10bytes) + jsonString + others(optional)
static int getJsonStr(int sock, char * jsonStr, int maxLen)
{
	unsigned char buf[1024];
	int recv_len;
	int total_len ;


    // recv the jsonStrLength field
	total_len = 0;
	while (total_len < 10)
	{
		recv_len = recv(sock, buf+total_len, 10-total_len, 0);
		if (recv_len < 0)
		{
			perror("recv:");
			return -1;
		}
		if (recv_len == 0)
		{
			return -1;
		}
		total_len += recv_len;
	}	
	buf[10] = 0;
	int jsonLen =  atoi(buf);
	if (jsonLen > maxLen || jsonLen < 1) 
    { 
        logger("json string is invalid:%d\n", jsonLen); 
        return -1;
    } 

    // recv the jsonString field
	total_len = 0;
	while (total_len < jsonLen)
	{
		recv_len = recv(sock, jsonStr+total_len, jsonLen-total_len, 0);
		if (recv_len < 0)
		{
			perror("recv:");
			return -1;
		}
		if (recv_len == 0)
		{
			return -1;
		}
		total_len += recv_len;
	}
	return jsonLen;
}
void SendFileToAgent(int sock, const char * jsonStr, int jsonStrLen);
void GetFileFromAgent(int sock, const char * jsonStr, int jsonStrLen);
void SendCmdsToAgentAndRun(int sock, const char * jsonStr, int jsonStrLen);

// the dir to save files transported
// check and create it if necessary
void checkFileDirExist()
{
    char path[255];
    snprintf(path, sizeof(path), "%s/../files", currentDir);
	if (opendir(path) == NULL)
	{
		mkdir(path, S_IRWXU);
	}
}

// the dir to save files transported
const char * getFileDir()
{
    static char path[255];
    snprintf(path, sizeof(path), "%s/../files", currentDir);
	return path;
}

// return error message back to server
void returnErrorMessage(int sock, const char * msg)
{
	char jsonStr[1024];
    char lenStr[11];

    logger("send error message:%s\n", msg);

    int len = snprintf(jsonStr+10, sizeof(jsonStr)-10, "{\"status\":100, \"message\":\"%s\"}", msg);
    logger("json msg len:%d\n", len);
    snprintf(lenStr, sizeof(lenStr), "%-10d", len);
    memcpy(jsonStr, lenStr, 10);
    
	int wn = write(sock, jsonStr, len+10);
    logger("returnErrMsg write %d bytes to socket!\n", wn);
}


// process request from server
static void do_processing(int sock)
{
	char jsonStr[10240];

	int jsonStrLen = getJsonStr(sock, jsonStr, sizeof(jsonStr)-1 );
	if (jsonStrLen < 0)
	{
		return;
	}
	jsonStr[jsonStrLen] = 0;

	struct json_token * obj, *handleClass;
	char handleClassStr[30];


    // get handleClass field which determines what action will be taken
	logger("received json string:%s\n", jsonStr); 
	obj = parse_json2(jsonStr, jsonStrLen);
	if (obj == NULL)
	{
		logger("invalid json string\n");
		return;
	}
	handleClass =  find_json_token(obj, "handleClass");
	if (handleClass == NULL )
	{
		logger("can NOT find handleClass element in json string\n");
		free(obj);
		return;
	}
	if (handleClass->type != JSON_TYPE_STRING  || handleClass->len >= sizeof(handleClassStr) )
	{
		free(obj);
		return; 
	}
	memcpy(handleClassStr, handleClass->ptr, handleClass->len);
	handleClassStr[handleClass->len] = 0;
	logger("handleClass:[%s]\n", handleClassStr);


	if (strncmp(handleClassStr, "GetFileFromAgent", 
				sizeof(handleClassStr) ) ==0)
	{
		GetFileFromAgent(sock, jsonStr, jsonStrLen);
		free(obj);
		return; 
	}
	if (strncmp(handleClassStr, "SendFileToAgent", 
				sizeof(handleClassStr) ) ==0)
	{
		SendFileToAgent(sock, jsonStr, jsonStrLen);
		free(obj);
		return; 
	}
	if (strncmp(handleClassStr, "SendCmdsToAgentAndRun", 
				sizeof(handleClassStr)) ==0)
	{
		SendCmdsToAgentAndRun(sock, jsonStr, jsonStrLen);
		free(obj);
		return; 
	}
	free(obj);
	
	return;
}



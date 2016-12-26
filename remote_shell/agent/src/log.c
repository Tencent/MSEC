
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

/*
 * log utility
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <stdarg.h>
#include "log.h"

#define LOG_PREFIX "remote_shell.log."

static int logLevel = ERROR;
static char currentDir[255];

void setLogLevel(int level)
{
    logLevel = level;
}

int getLogLevel(void)
{
    return logLevel;
}

void currentTime2Str(char * str, int max_len)
{
    time_t t = time(NULL);
    struct tm * p = gmtime(&t);
    snprintf(str, max_len, "%04d%02d%02d-%02d%02d%02d",
            p->tm_year + 1900,
            p->tm_mon+1,
            p->tm_mday,
            p->tm_hour,
            p->tm_min,
            p->tm_sec);
}

int comparFileName(const void * a, const void *b)
{
    const char * aa = *((const char **)a);
    const char * bb = *((const char **)b);
    int prefix_len  = strlen(LOG_PREFIX);
    long long alen, blen;

    if (strlen(aa) > prefix_len) {
        alen = atoll(aa + prefix_len);
    } else {
        alen = 0;
    }

    if (strlen(bb) > prefix_len) {
        blen = atoll(bb + prefix_len);
    } else {
        blen = 0;
    }

    return alen > blen;
}

static void clearLogDir(const char * dirStr)
{
    DIR * dp;
    struct dirent * dirEntry;
    if( NULL ==  (dp = opendir(dirStr)))
    {
     return ;
    }

    const char * fileName[1024];
    int fileNumber = 0; 
    
    while( NULL != (dirEntry = readdir(dp))  && fileNumber < 1024)
    {
            if (strstr(dirEntry->d_name, LOG_PREFIX) != NULL)
            {
                fileName[fileNumber] = dirEntry->d_name;
                fileNumber++;
            }
    }
    qsort(fileName, fileNumber, sizeof(const char *), comparFileName);
    int i;

    //printf("sort result:\n");
    for  (i = 0; i < fileNumber; ++i)
    {
        //printf( "[%s]\n", fileName[i]);
    }

    for (i = 0; i < fileNumber-5; ++i)
    {
        char fullName[255];
        snprintf(fullName, sizeof(fullName), "%s/%s", dirStr, fileName[i]);
        //printf("delete %s\n", fullName);
        remove(fullName);   
    }

    closedir(dp);
    return;
}

void logger(const char * fmt, ...)
{
    va_list ap; 
    va_start(ap, fmt); 

    if (currentDir[0] == '\0')
    {
        getcwd(currentDir, sizeof(currentDir));
    }

    char fileName[255];
    snprintf(fileName, sizeof(fileName), "%s/../log", currentDir);
    DIR * dp; 
    if ( (dp = opendir (fileName)) == NULL)
    {
            mkdir (fileName, S_IRWXU);
    }
    closedir(dp);
    clearLogDir(fileName);

    snprintf(fileName, sizeof(fileName), "%s/../log/remote_shell.log", currentDir);
    FILE * f = fopen(fileName, "a");
    if (f == NULL)
    {
        return;
    }
    vfprintf(f, fmt, ap);
    fclose(f);

    struct stat status;

    if (stat(fileName, &status))
    {
        return;
    }
    if (status.st_size > 10000000)
    {
        char newFileName[255];
        snprintf(newFileName, sizeof(newFileName), "%s.%llu", fileName, (unsigned long long)time(NULL));
        rename(fileName, newFileName);

    }
    

}

#if 0
int main()
{
    strcpy(currentDir, "/tmp");
    int i = 0; 
    for (i = 0; i < 100;++i)
    {
        logger("hello world");
    }
    return 0;
    
}
#endif

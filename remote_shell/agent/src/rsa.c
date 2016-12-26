
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
 * this file supplies rsa functions 
 *
 * agent use rsa to authenticate the server and the valid of command file:
 * 1. server will encrypt the digest of command file with private key, and 
 *     send the ciphered text as signature with command file
 * 2. agent will decrypt the signature field with public key and check  if
 *     the plain text equals the digest of command file.
 */
#include "openssl/rsa.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include <openssl/crypto.h>  
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>


/*
 * get the digest of file content
 */
int getHash(const char * filename, unsigned char * md)
{  

    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        return -1;
    }
    
    SHA256_CTX c;  

    SHA256_Init(&c);  

    while (1)
    {
        unsigned char buf[10240];

        int len = read(fd, buf, sizeof(buf));
        if (len <= 0)
        {
            break;
        }
        SHA256_Update(&c, buf, len);
    }
    SHA256_Final(md, &c);  
    OPENSSL_cleanse(&c, sizeof(c));  
    return 0;

}

/*
 * get the digest of file content
 */
int getHashOfByteArray(const unsigned char * buf, int len, unsigned char * md)
{



    SHA256_CTX c;

    SHA256_Init(&c);


    SHA256_Update(&c, buf, len);

    SHA256_Final(md, &c);

    OPENSSL_cleanse(&c, sizeof(c));
    return 0;

}


RSA *  loadKeyFromFile(const char * filename)
{
  RSA *p_rsa;
  FILE *file;
  int flen, rsa_len;
  if ((file = fopen (filename, "r")) == NULL)
    {
      perror ("open key file error");
      return NULL;
    }
  if ((p_rsa = PEM_read_RSA_PUBKEY (file, NULL, NULL, NULL)) == NULL)
    {
      perror ("read key");
      return NULL;
    }
  return p_rsa;
}


int decrypt(const unsigned char * in, unsigned int  inlen, unsigned char * out, RSA * key)
{
   return RSA_public_decrypt(inlen, in, out, key, RSA_PKCS1_PADDING);  
}

int hexChr2Int(char c)
{
    char v[] = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};

    int i;
    for (i = 0; i < 16; ++i)
    {
        if (v[i] == c)
        {
            return i;
        }
    }
    return 17;
}

int hexStr2bytes(const char * str, unsigned char * out, int *outlen)
{
    int len = strlen(str);
    if ( ((len%2) != 0) || (len/2) > *outlen)
    {
        return -1;
    }
    int i, j;

    for (i = 0, j = 0; i < len ; i+=2, j++)
    {
        char c1 = str[i];
        char c2 = str[i+1];
        int v1 = hexChr2Int(c1);
        int v2 = hexChr2Int(c2);
        if (v1 > 16 || v2 > 16)
        {
            printf("invalid hex char\n");
            return -1;
        }
        out[j] = v1*16 + v2;
    }
    *outlen = j;
    return 0;
}

int bytes2hexStr(const unsigned char * bytes,  int byteNum, char * out, int outlen)
{
    if (outlen < (2*byteNum))
    {
        return -1;
    }
    int i, j;
    const char chrs[] = {'0', '1','2','3', '4','5','6','7','8','9','a','b','c','d','e','f'};
    for (i = 0, j = 0; i < byteNum; ++i)
    {
        unsigned char b = bytes[i];

        out[j++] = chrs[b>>4];
        out[j++] = chrs[b&0x0f];
    }
    out[j++] = 0;
    return j;
}
unsigned int getRandUInt()
{
    static int initflag = 0;
    if (initflag == 0)
    {
        initflag = 1;
        unsigned long  t[2];
        t[0] = time(NULL);
        t[1] = t[0];

        unsigned short s[3];
        memcpy(&s, &t, sizeof(s));
        seed48(s);
    }
    return (int)(drand48()*INT32_MAX);
}

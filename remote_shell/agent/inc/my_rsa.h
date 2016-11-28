
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


#ifndef _MY_RSA_H_
#define  _MY_RSA_H_

#include <openssl/rsa.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

RSA *  loadKeyFromFile(const char * filename);
int getHash(const char * filename, unsigned char * md);
int decrypt(const unsigned char * in, unsigned int  inlen, unsigned char * out, RSA * key);
int hexChr2Int(char c);
int hexStr2bytes(const char * str, unsigned char * out, int *outlen);
int bytes2hexStr(const unsigned char * bytes,  int byteNum, char * out, int outlen);

#endif

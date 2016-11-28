
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


#include "multi_hash_table.h"
#include <stdio.h>
#include <string>
#include <stdlib.h>

int main()
{
	MultiHashTable t;
	MhtInitParam param;
	memset((char*)&param, 0, sizeof(MhtInitParam));
	param.cMaxKeyLen = 24;
	param.ddwShmKey = 2000005;
	param.ddwBufferSize = 5000000;
	param.dwExpiryRatio = 4;
		
	int ret = t.CreateFromShm(param);
	if(ret != 0)
	{
		printf("error creating|%d|%s\n", ret, t.GetErrorMsg());
		return 0;
	}
	printf("available commands: \n");
	printf("  set key value\n");
	printf("  get key\n");
	printf("  del key\n");
	printf("  iter\n");
	printf("  print\n");
	printf("  quit\n");
	for(int i = 0;i < 1000; i++)
	{
		char buffer [32];
		snprintf(buffer, 32, "%d", i);
		t.SetData(buffer, strlen(buffer), buffer, strlen(buffer));
	}
	while(1)
	{
		static char cmd[1024*1024];
		
		printf("cmd>"); fflush(stdout);

		if(fgets(cmd, sizeof(cmd), stdin)==NULL)
			return 0;
		if(strncmp(cmd, "set ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			char* pkey = pstr;
			while(!isspace(*pstr)) pstr ++;
			int pkeylen  = pstr - pkey;
			while(isspace(*pstr)) pstr ++;
			char* pval = pstr;
			while(!isspace(*pstr)) pstr ++;
			int pvallen  = pstr - pval;
			ret = t.SetData(pkey, pkeylen, pval, pvallen, true);
			if(ret)
				printf("set failed|%d\n", ret);
			else
				printf("set OK\n");
		}
		else if(strncmp(cmd, "get ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			char* pkey = pstr;
			while(!isspace(*pstr)) pstr ++;
			int pkeylen  = pstr - pkey;
			char val[1024];
			int vallen = 1024;
			ret = t.GetData(pkey, pkeylen, val, vallen);
			if(ret)
				printf("get failed|%d\n", ret);
			else
			{
				std::string s(val, vallen);
				printf("get val: %s\n", s.c_str());
			}
		}
		else if(strncmp(cmd, "del ", 4)==0)
		{
			char *pstr = cmd + 4;
			while(isspace(*pstr)) pstr ++;
			char* pkey = pstr;
			while(!isspace(*pstr)) pstr ++;
			int pkeylen  = pstr - pkey;
			ret = t.EraseData(pkey, pkeylen);
			if(ret)
				printf("del failed|%d\n", ret);
			else
			{
				printf("OK\n");
			}
		}
		else if(strncmp(cmd, "loop", 4)==0)
		{
			MhtIterator it = t.Begin();
			for(; it!= t.End(); it = t.Next(it))
			{
				static MhtData data;
				t.Get(it, data);
				std::string s((char*)data.key, data.klen);
				printf("key: %s\n", s.c_str());
			}
		}
		else if(strncmp(cmd, "iter", 4)==0)
		{
			MhtNode* node;
			for(node = t.GetFirstNode(); node != NULL; )
			{
				printf("%s\n",node->acKey);
				node = t.GetNode(node->postPos);
			}
		}
		else if(strncmp(cmd, "print", 5)==0)
		{
			t.PrintInfo();
		}
		else if(strncmp(cmd, "quit", 4)==0 || strncmp(cmd, "exit", 4)==0)
		{
			return 0;
		}
	}
}

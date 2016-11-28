
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


#include"global.h"

using namespace spp::global;
using namespace std;

char* spp_global::arg_start = NULL;
char* spp_global::arg_end =  NULL;
char* spp_global::env_start = NULL;

string spp_global::mem_full_warning_script;

int spp_global::cur_group_id = -1; 


// 保存启动参数
void spp_global::save_arg(int argc, char** argv)
{
    arg_start = argv[0];
    arg_end = argv[argc-1] + strlen(argv[argc - 1]) + 1;
    env_start = environ[0];
}

/*
void spp_global::save_log_info(string& path,string& prefix,string& level,int size)
{
	if(path=="")
	{
		log_path=default_sb_log_path;
	}
	else
		log_path=path;

	if(prefix=="")
	{
		sb_log_prefix=default_sb_log_prefix;
	}
	else
	sb_log_prefix=prefix+"_"+default_sb_log_prefix;

	if(level=="")
	{
		sb_log_level=APP_TRACE;
	}
	else
		sb_log_level=atoi(level.c_str());
	if(size<=0)
	{
		log_size=default_sb_log_size;
	}
	log_size=size;
}
*/

void spp_global::daemon_set_title(const char* fmt, ...)
{
    char title [64];
    int i, tlen;
    va_list ap;

    va_start(ap, fmt);
    vsnprintf(title, sizeof(title) - 1, fmt, ap);
    va_end(ap);

	//title length
    tlen = strlen(title) + 1;

    if (arg_end - arg_start < tlen && env_start == arg_end) {
        char *env_end = env_start;

        for (i = 0; environ[i]; i++) {
            if (env_end == environ[i]) {
                env_end = environ[i] + strlen(environ[i]) + 1;
                environ[i] = strdup(environ[i]);
            } else
                break;
        }

        arg_end = env_end;
        env_start = NULL;
    }

    i = arg_end - arg_start;

    if (tlen == i) {
		if(arg_start) {
			strncpy(arg_start, title, strlen(arg_start));
		}
    } else if (tlen < i) {
		if(arg_start) {
			strncpy(arg_start, title, strlen(arg_start));
        	memset(arg_start + tlen, 0, i - tlen);
		}
    } else {
        stpncpy(arg_start, title, i - 1)[0] = '\0';
    }
}

void spp_global::set_cur_group_id(int group_id)
{
    spp_global::cur_group_id = group_id;
}
int spp_global::get_cur_group_id()
{
    return spp_global::cur_group_id;
}


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

#include "exception.h"

void cb(int signo, void *args)
{
    printf("signal callback!!\n");
}

void c(void)
{
    int *p = NULL;
    *p = 0;
}

void b(void)
{
    c();
}

void a(void)
{
    b();
}

int main()
{
    int ret;
    ret = install_signal_handler("/var/exception", cb, NULL);
    if (ret < 0) {
        printf("install signal handler failed. ret [%d]\n", ret);
        return -1;
    }

    set_memlog_maxsize(2);
    add_memlog("abc", 3);
    add_memlog("abc", 3);
    add_memlog("abc", 3);
    add_memlog("abc", 3);

    a();

    return ret;
}

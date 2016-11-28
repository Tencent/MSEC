
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


#include <time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * @modify from linux-2.6.32/lib/random32.c
 */

struct rnd_state {
    unsigned int s1, s2, s3;
};

static __thread int init_flag;
static __thread struct rnd_state thread_state;

static unsigned int __random32(struct rnd_state *state)
{
    #define TAUSWORTHE(s,a,b,c,d) ((s&c)<<d) ^ (((s <<a) ^ s)>>b)

    state->s1 = TAUSWORTHE(state->s1, 13, 19, 4294967294UL, 12);
    state->s2 = TAUSWORTHE(state->s2, 2, 25, 4294967288UL, 4);
    state->s3 = TAUSWORTHE(state->s3, 3, 11, 4294967280UL, 17);

    return (state->s1 ^ state->s2 ^ state->s3);
}

static inline unsigned int __seed(unsigned int x, unsigned int m)
{
    return (x < m) ? x + m : x;
}

static int random32_init(struct rnd_state *state)
{
    #define LCG(x)  ((x) * 69069)   /* super-duper LCG */
    state->s1 = __seed(LCG((time(NULL) ^ syscall(SYS_gettid))), 1);
    state->s2 = __seed(LCG(state->s1), 7);
    state->s3 = __seed(LCG(state->s2), 15);

    /* "warm it up" */
    __random32(state);
    __random32(state);
    __random32(state);
    __random32(state);
    __random32(state);
    __random32(state);
    return 0;
}

unsigned int nlb_rand(void)
{
    unsigned int r;
    struct rnd_state *state = &thread_state;

    if (!init_flag) {
        random32_init(state);
        init_flag = 1;
    }

    r = __random32(state);
    return r;
}



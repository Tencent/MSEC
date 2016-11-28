#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <hiredis.h>
#include "mt_capi.h"

static const char *g_host = "127.0.0.1";
static const char *g_port = "6379";
static int mthread_cnt = 3;

void hiredis_work(void *args);

int main(int argc, char **argv) {
    int i;
    if (argc > 1) {
        g_host = strdup(argv[1]);
    }

    if (argc > 2) {
        g_port = strdup(argv[2]);
    }


    /* Init microthread frame */
    if (mtc_init_frame() < 0) {
        printf("Init frame failed.\n");
        exit(2);
    }

    mtc_sleep(1);

    for (i = 0; i < mthread_cnt; i++) {
        mtc_start_thread((void *)hiredis_work, NULL);
    }

    while (mthread_cnt) {
        mtc_sleep(10);
    }

    return 0;
}

void hiredis_work(void *args)
{
    unsigned int j;
    redisContext *c;
    redisReply *reply;
    const char *hostname = g_host;
    int port = atoi(g_port);

    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    c = redisConnectWithTimeout(hostname, port, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    /* PING server */
    reply = redisCommand(c,"PING");
    printf("PING: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key */
    reply = redisCommand(c,"SET %s %s", "foo", "hello world");
    printf("SET: %s\n", reply->str);
    freeReplyObject(reply);

    /* Set a key using binary safe API */
    reply = redisCommand(c,"SET %b %b", "bar", (size_t) 3, "hello", (size_t) 5);
    printf("SET (binary API): %s\n", reply->str);
    freeReplyObject(reply);

    /* Try a GET and two INCR */
    reply = redisCommand(c,"GET foo");
    printf("GET foo: %s\n", reply->str);
    freeReplyObject(reply);

    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);
    /* again ... */
    reply = redisCommand(c,"INCR counter");
    printf("INCR counter: %lld\n", reply->integer);
    freeReplyObject(reply);

    /* Create a list of numbers, from 0 to 9 */
    reply = redisCommand(c,"DEL mylist");
    freeReplyObject(reply);
    for (j = 0; j < 10; j++) {
        char buf[64];

        snprintf(buf,64,"%d",j);
        reply = redisCommand(c,"LPUSH mylist element-%s", buf);
        freeReplyObject(reply);
    }

    /* Let's check what we have inside the list */
    reply = redisCommand(c,"LRANGE mylist 0 -1");
    if (reply->type == REDIS_REPLY_ARRAY) {
        for (j = 0; j < reply->elements; j++) {
            printf("%u) %s\n", j, reply->element[j]->str);
        }
    }
    freeReplyObject(reply);

    /* Disconnects and frees the context */
    redisFree(c);

    mtc_sleep(1);

    mthread_cnt--;

    return 0;
}

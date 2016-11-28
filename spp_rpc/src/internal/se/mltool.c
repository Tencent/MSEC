
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


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef struct _tag_cmd_info
{
    int   type;
    int   ml_index;
    int   resend_fd;
    int   connect_type;
    int   prot_type;
    char *host;
    char *prefix;
    char *dump_file;
    struct sockaddr_in addr;
}cmd_info_t;

typedef void (*ml_cb)(void *mem, int len);

enum {
    CMD_TYPE_SHOW = 1,
    CMD_TYPE_RESEND,
    CMD_TYPE_DUMP,
    CMD_TYPE_UNKOWN = 100,

    CONNECT_TYPE_LONG  = 1,
    CONNECT_TYPE_SHORT = 2,

    PROT_TYPE_UDP = 1,
    PROT_TYPE_TCP = 2,
};

static cmd_info_t  cmd_info = {
    .type           = CMD_TYPE_UNKOWN,
    .ml_index       = -1,
    .resend_fd      = -1,
    .connect_type   = CONNECT_TYPE_SHORT,
    .host           = NULL,
    .prefix         = NULL,
    .dump_file      = NULL,
};

// "10.1.1.1:7963@udp"
int parse_resend_host(const char *host)
{
    int ret;
    int fd;
    char buf[100];
    char *alt_pos;
    char *colon_pos;

    memset(&cmd_info.addr, 0, sizeof(cmd_info.addr));
    cmd_info.addr.sin_family = AF_INET;

    // IP地址
    colon_pos = strchr(host, ':');
    if (NULL == colon_pos) {
        return -1;
    }

    memcpy(buf, host, colon_pos - host);
    buf[colon_pos - host] = '\0';
    ret = inet_aton(buf, (struct in_addr *)&cmd_info.addr.sin_addr.s_addr);
    if (!ret) {
        return -2;
    }

    // 端口
    alt_pos = strchr(colon_pos, '@');
    if (NULL == alt_pos) {
        return -3;
    }

    memcpy(buf, colon_pos+1, alt_pos-colon_pos-1);
    buf[alt_pos-colon_pos-1] = '\0';
    cmd_info.addr.sin_port = htons((unsigned short)atoi(buf));

    // TCP or UDP
    if (!strcmp(alt_pos+1, "udp")) {
        cmd_info.prot_type = PROT_TYPE_UDP;
    } else if (!strcmp(alt_pos+1, "tcp")) {
        cmd_info.prot_type = PROT_TYPE_TCP;
    } else {
        return -4;
    }

    return 0;
}

static void print_usage(const char *name)
{
    printf(" This is a program for se memlog.\n");
    printf(" Usage:  %s [-r host | -s | -d path] [-i memlog-index] prefix\n", name);
    printf("        -h  --help          Print this usage\n");
    printf("        -r  --resend        Send memlog to host, host's format like \"IP:Port@udp\"\n");
    printf("        -s  --show          Display memlog\n");
    printf("        -d  --dump          Dump memlog to Specified file\n");
    printf("        -m  --memlog-index  Specify memlog index, begin from 1, default is all memlog\n");
    printf("        -c  --connect-type  Specify connect type, [long|short], default short\n\n");
    printf(" Example for use:\n");
    printf(" 1. Display all memlog\n");
    printf("    %s -s -m all /data/test/1463799542-18756-18756\n", name);
    printf(" 2. Display the second memlog\n");
    printf("    %s -s -m 2 /data/test/1463799542-18756-18756\n", name);
    printf(" 3. Send memlog to a specified server\n");
    printf("    %s -r 127.0.0.1:7963@udp /data/test/1463799542-18756-18756\n", name);
    printf(" 4. Send the second memlog to a specified server\n");
    printf("    %s -r 127.0.0.1:7963@udp -m 2 /data/test/1463799542-18756-18756\n", name);
    printf(" 5. Send memlog to a specified server with long connection\n");
    printf("    %s -r 127.0.0.1:7963@udp -c long /data/test/1463799542-18756-18756\n", name);
    printf(" 6. Dump the first memlog to local file\n");
    printf("    %s -d ./memlog.dat -m 1 /data/test/1463799542-18756-18756\n", name);
}

void parse_args(int32_t argc, char **argv)
{
    int  c;
    int  index;
    const char *short_opts = "hr:sd:m:c:";
    const struct option long_opts[] = {
            {"help", no_argument, NULL, 'h'},
            {"resend", required_argument, NULL, 'r'},
            {"show", no_argument, NULL, 's'},
            {"dump", required_argument, NULL, 'd'},
            {"memlog-indx", required_argument, NULL, 'm'},
            {"connect-type", required_argument, NULL, 'c'},
            {0,0,0,0}
    };

    if (argc < 3) {
        print_usage(argv[0]);
        exit(1);
    }

    cmd_info.prefix = strdup(argv[argc - 1]);
    
    for (index = 1; index < (argc - 1);) {
        if (!strcmp(argv[index], "-h")
            || !strcmp(argv[index], "--help")) {
            print_usage(argv[0]);
            exit(1);
        }

        if (!strcmp(argv[index], "-s")
            || !strcmp(argv[index], "--show")) {
            cmd_info.type     = CMD_TYPE_SHOW;
            index++;
            continue;
        }

        if (!strcmp(argv[index], "-r")
            || !strcmp(argv[index], "--resend")) {
            if (parse_resend_host(argv[index+1]) < 0) {
                printf("Invalid host [%s]!\n", argv[index+1]);
                print_usage(argv[0]);
                exit(1);
            }
            cmd_info.type     = CMD_TYPE_RESEND;
            cmd_info.host     = strdup(argv[index+1]);
            index            += 2;
            continue;
        }

        if (!strcmp(argv[index], "-c")
            || !strcmp(argv[index], "--connect-type")) {
            if (!strcmp(argv[index+1], "long")) {
                cmd_info.connect_type = CONNECT_TYPE_LONG;
            } else if (!strcmp(argv[index+1], "short")) {
                cmd_info.connect_type = CONNECT_TYPE_SHORT;
            } else {
                printf("Invalid connection type [%s]!\n", argv[index+1]);
                print_usage(argv[0]);
                exit(1);
            }

            index += 2;
            continue;
        }

        if (!strcmp(argv[index], "-d")
            || !strcmp(argv[index], "--dump")) {
            cmd_info.type      = CMD_TYPE_DUMP;
            cmd_info.dump_file = strdup(argv[index+1]);
            index             += 2;
            continue;
        }

        if (!strcmp(argv[index], "-m")
            || !strcmp(argv[index], "--memlog-indx")) {
            if (strcmp(argv[index+1], "all")) {
                cmd_info.ml_index = atoi(argv[index+1]);
            }

            index += 2;
            continue;
        }

        printf("Error: unknown option %s\n", argv[index]);
    }

    if (NULL == cmd_info.prefix) {
        printf("Please set prefix argment");
        print_usage(argv[0]);
        exit(1);
    }

    if (cmd_info.type == CMD_TYPE_UNKOWN) {
        print_usage(argv[0]);
        exit(1);
    }
}

int write_all(int fd, void *mem, int len)
{
    char *buf = (char *)mem;
    int ret;
    int wlen = 0;

    while (wlen != len) {
        ret = write(fd, buf + wlen, len - wlen);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -2;
        }

        wlen += ret;
    }

    return 0;
}

void dump_memlog(void *mem, int len)
{
    int ret;
    int fd;
    
    fd = open(cmd_info.dump_file, O_RDWR | O_CREAT, 0666);
    if (fd == -1) {
        printf("Open dump file failed, %m\n");
        return;
    }

    ret = write_all(fd, mem, len);
    if (ret < 0) {
        close(fd);
        printf("Write dump file failed, %m\n");
        return;
    }

    close(fd);
}

int create_udp_socket(void)
{
    int fd;
    if (cmd_info.connect_type == CONNECT_TYPE_LONG) {
        if (cmd_info.resend_fd == -1) {
            cmd_info.resend_fd = socket(AF_INET, SOCK_DGRAM, 0);
        }

        return cmd_info.resend_fd;
    }

    return socket(AF_INET, SOCK_DGRAM, 0);
}

int udp_send(int fd, void *mem, int len)
{
    int ret;

    while (1) {
        ret = sendto(fd, mem, len, 0, (struct sockaddr *)&cmd_info.addr, sizeof(struct sockaddr_in));
        if (ret == -1 ) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }

            return -1;
        }

        break;
    }

    return 0;
}

void socket_close(int fd)
{
    if (CONNECT_TYPE_SHORT == cmd_info.connect_type)
        close(fd);
}

int udp_send_memlog(void *mem, int len)
{
    int fd;
    int ret;

    fd = create_udp_socket();
    if (fd < 0) {
        return -1;
    }

    ret = udp_send(fd, mem, len);
    if (ret < 0) {
        close(fd);
        cmd_info.resend_fd = -1;
        return -2;
    }

    socket_close(fd);

    return 0;
}

int cc_tcp_socket(void)
{
    int ret;
    int fd;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        return -1;
    }

    ret = connect(fd, (struct sockaddr *)&cmd_info.addr, sizeof(struct sockaddr_in));
    if (ret == -1) {
        close(fd);
        return -1;
    }

    return fd;
}

int create_tcp_socket(void)
{
    int fd;

    if (CONNECT_TYPE_LONG == cmd_info.connect_type) {
        if (cmd_info.resend_fd < 0) {
            cmd_info.resend_fd = cc_tcp_socket();
        }

        return cmd_info.resend_fd;
    }

    return cc_tcp_socket();
}

int tcp_send(int fd, void *mem, int len)
{
    int ret;
    int left = len;

    while (left > 0) {
        ret = send(fd, (char *)mem + len - left, left, 0);
        if (ret == -1) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }

            return -1;
        }

        left -= ret;
    }

    return 0;
}

int tcp_send_memlog(void *mem, int len)
{
    int ret;
    int fd;

    fd = create_tcp_socket();
    if (fd < 0) {
        return -1;
    }

    ret = tcp_send(fd, mem, len);
    if (ret < 0) {
        close(fd);
        cmd_info.resend_fd = -1;
        return -2;
    }

    socket_close(fd);
    return 0;
}

void resend_memlog(void *mem, int len)
{
    int ret;

    if (cmd_info.prot_type == PROT_TYPE_UDP) {
        ret = udp_send_memlog(mem, len);
    } else if (cmd_info.prot_type == PROT_TYPE_TCP) {
        ret = tcp_send_memlog(mem, len);
    } else {
        printf("Invalid protocal type: %d\n", cmd_info.prot_type);
        return;
    }

    if (ret < 0) {
        printf("Send memlog failed!\n");
    }
}

static inline char P(char c)
{
    if (isprint(c)) {
        return c;
    }

    return '.';
}


void print_memlog(void *mem, int len)
{
    char *mm = (char *)mem;
    int left = len;
    char hex_buf[64];
    char asc_buf[32];

    while (left > 0) {
        if (left >= 16) {
            snprintf(hex_buf, sizeof(hex_buf), 
                     "%08x: %02x%02x %02x%02x %02x%02x %02x%02x"
                          " %02x%02x %02x%02x %02x%02x %02x%02x",
                     (int)(mm-(char *)mem),
                     mm[0], mm[1], mm[2], mm[3], mm[4], mm[5], mm[6], mm[7],
                     mm[8], mm[9], mm[10], mm[11], mm[12], mm[13], mm[14], mm[15]);

            snprintf(asc_buf, sizeof(asc_buf),
                     "%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",
                     P(mm[0]), P(mm[1]), P(mm[2]), P(mm[3]), P(mm[4]), 
                     P(mm[5]), P(mm[6]), P(mm[7]), P(mm[8]), P(mm[9]),
                     P(mm[10]), P(mm[11]), P(mm[12]), P(mm[13]), P(mm[14]), P(mm[15]));

            left -= 16;
            mm   += 16;
        } else {
            int i;
            int hex_pos;
            int asc_pos = 0;

            hex_pos = snprintf(hex_buf, sizeof(hex_buf), "%08x:", (int)(mm - (char *)mem));;
            for (i = 0; i < left; i++) {
                if (i%2) {
                    hex_pos += snprintf(hex_buf + hex_pos, sizeof(hex_buf) - hex_pos, "%02x", mm[i]);
                } else {
                    hex_pos += snprintf(hex_buf + hex_pos, sizeof(hex_buf) - hex_pos, " %02x", mm[i]);
                }

                asc_pos += snprintf(asc_buf + asc_pos, sizeof(asc_buf) - asc_pos, "%c", P(mm[i]));
            }

            left = 0;
        }

        printf("%-50s %s\n", hex_buf, asc_buf);
    }
}

static int read_fd(int fd, void *mem, int len)
{
    char *buf = (char *)mem;
    int ret;
    int rlen = 0;

    while (rlen != len) {
        ret = read(fd, buf + rlen, len - rlen);
        if (ret == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            }
            return -2;
        }

        if (ret == 0) {
            break;
        }

        rlen += ret;
    }

    return rlen;
}


int read_file(const char *path, void **mem, int *size)
{
    int ret;
    int fd;
    char *buf;
    struct stat s;

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        return -1;
    }

    ret = fstat(fd, &s);
    if (ret == -1) {
        close(fd);
        return -2;
    }

    *size = (int)s.st_size;

    buf = malloc(*size);
    if (NULL == buf) {
        close(fd);
        return -3;
    }

    ret = read_fd(fd, buf, *size);
    if (ret < 0) {
        close(fd);
        free(buf);
        return -4;
    }

    *mem = (void *)buf;
    close(fd);

    return 0;
}

void handle_memlog(ml_cb cb)
{
    int ret;
    int left;
    int buf_len;
    int mem_index;
    char  path[256];
    void *buf;

    snprintf(path, sizeof(path), "%s/memlog.dat", cmd_info.prefix);

    ret = read_file(path, &buf, &buf_len);
    if (ret < 0 || NULL == buf || buf_len <= 0) {
        printf("[ERROR] Read memlog file error\n");
        return;
    }

    mem_index = 1;
    left      = buf_len;
    do {
        char *mem;
        int ml_len;

        mem     = (char *)buf + buf_len - left;
        ml_len  = *(int *)mem;

        if ((cmd_info.ml_index == mem_index) || (cmd_info.ml_index == -1)) {
            printf("MEMLOG %d\n", mem_index);
            cb(mem+4, ml_len);
        }

        mem_index++;
        left -= 4+ml_len;
    } while(left > 4);

    free(buf);
}

int main(int argc, char **argv)
{
    parse_args(argc, argv);

    switch (cmd_info.type) {
        case CMD_TYPE_SHOW:
            handle_memlog(print_memlog);
            break;
        case CMD_TYPE_RESEND:
            handle_memlog(resend_memlog);
            break;
        case CMD_TYPE_DUMP:
            handle_memlog(dump_memlog);
            break;
        default:
            printf("[ERROR] invalid cmd : %d\n", cmd_info.type);
            break;
    }

    return 0;
}




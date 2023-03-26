/**
 * @file example.c
 * @brief SNTP client example
 * @author herve-ves
 * @date 2023-03-26
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "sntpc.h"

static double systime(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000;
}

static void usage()
{
    printf("Usage: example [-h] [-t milliseconds] [-s server_1:server_2:server_3:...]\n");
    printf("\n");
    printf("    -h  Show this help message\n");
    printf("    -t  Set wait server response timeout milliseconds (default 1000 seconds)\n");
    printf("    -s  Set servers name (default pool.ntp.org)\n");
    printf("\n");
    exit(0);
}

static void do_sntpc_by_server_name(const char *server, int timeout_ms)
{
    struct sntpc_result result;
    int ret = sntpc_perform(server, timeout_ms, systime, &result);
    if (ret != 0)
        printf("server[%s]: sntpc_perform() error=%d\n", server, ret);
    else 
        printf("server[%s]: offset=%f, delay=%f, c_time=%f\n", server, result.offset, result.delay, result.c_time);
}

int main(int argc, char **argv)
{
    int timeout_ms = 1000;
    char *servers = "pool.ntp.org";

    int c;
    while ((c = getopt(argc, argv, "ht:s:")) != -1) {
        switch (c) {
        case 'h':
            usage();
            break;
        case 't':
            timeout_ms = atoi(optarg);
            break;
        case 's':
            servers = optarg;
            break;
        default:
            exit(1);
        }
    }
    char *sub = servers;
    while (*sub++ != '\0') {
        if (*sub == ':') {
            *sub++ = '\0';
        } else if (*sub != '\0') {
            continue;
        }
        do_sntpc_by_server_name(servers, timeout_ms);
        servers = sub;
    }
    return 0;
}

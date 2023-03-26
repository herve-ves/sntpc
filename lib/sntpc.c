/**
 * @file sntpc.c
 * @brief SNTP client implementation
 * @author herve-ves
 * @date 2023-03-26
 */

#include <stdint.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#include "sntpc.h"

#define SNTP_PORT "123"
#define SNTP_TIMESTAMP_DELTA (2208988800ULL)
#define SNTP_LI_MASK (0xC0)
#define SNTP_LI_NO_WARNING (0x00 << 6)
#define SNTP_VERSION_MASK (0x38)
#define SNTP_VERSION (4 << 3)
#define SNTP_MODE_MASK (0x07)
#define SNTP_MODE_CLIENT (0x03)
#define SNTP_MODE_SERVER (0x04)
#define SNTP_STRATUM_KOD (0x00)
#define SNTP_TIMESTAMP2TIME(_secs, _frac, _time) \
    do { \
        (_time) = (double)(_secs - SNTP_TIMESTAMP_DELTA) + (double)(_frac) / (1ULL << 32); \
    } while (0)
#define TIME2SNTP_TIMESTAMP(_time, _secs, _frac) \
    do { \
        uint64_t _sts = (uint64_t)((_time) * (1ULL << 32)); \
        (_frac) = (uint32_t)(_sts); \
        (_secs) = (uint32_t)((_sts) >> 32) + SNTP_TIMESTAMP_DELTA; \
    } while (0)

#pragma pack(push, 1)
struct sntp_data_packet
{
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t reference_identifier;
    uint32_t reference_timestamp[2];
    uint32_t originate_timestamp[2];
    uint32_t receive_timestamp[2];
    uint32_t transmit_timestamp[2];
};
#pragma pack(pop)

static struct sntp_data_packet new_sntp_data_packet(double time)
{
    uint32_t secs, frac;
    TIME2SNTP_TIMESTAMP(time, secs, frac);
    return (struct sntp_data_packet){
        .li_vn_mode = SNTP_LI_NO_WARNING | SNTP_VERSION | SNTP_MODE_CLIENT,
        .transmit_timestamp[0] = htonl(secs),
        .transmit_timestamp[1] = htonl(frac),
    };
}

static int new_transport_fd(int *fd, const char *server)
{
    int ret = 0, sockfd = -1;
    struct addrinfo hints = {
        .ai_socktype = SOCK_DGRAM,
    }, *ai = NULL;
    if (0 != getaddrinfo(server, SNTP_PORT, &hints, &ai)) {
        ret = SNTPC_ERR_DNS_FAILED;
        goto EXIT;
    }
    if (0 > (sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol))) {
        ret = SNTPC_ERR_SOCKET_FAILED;
        goto EXIT;
    }
    if (0 > connect(sockfd, ai->ai_addr, ai->ai_addrlen))
        ret = SNTPC_ERR_CONNECT_FAILED;
    else
        *fd = sockfd;
EXIT:
    freeaddrinfo(ai);
    if (ret != 0)
        close(sockfd);
    return ret;
}

int sntpc_perform_by_fd(int fd, int timeout_ms, sntpc_systime_fn_t systime_fn, struct sntpc_result *result)
{
    if (!systime_fn || !result)
        return SNTPC_ERR_INVALID_ARGS;
    struct sntp_data_packet req = new_sntp_data_packet(systime_fn()), resp = {0};
    if (0 > send(fd, &req, sizeof(req), 0))
        return SNTPC_ERR_SEND_FAILED;
    fd_set rd_fds;
    FD_SET(fd, &rd_fds);
    struct timeval to_tv = {
        .tv_sec = (timeout_ms > 0) ? (timeout_ms / 1000) : 0,
        .tv_usec = (timeout_ms > 0) ? ((timeout_ms % 1000) * 1000) : 0,
    };
    int s = select(fd + 1, &rd_fds, NULL, NULL, (timeout_ms >= 0) ? &to_tv : NULL);
    if (s <= 0)
        return (0 == s) ? SNTPC_ERR_TIMEOUTED : SNTPC_ERR_SELECT_FAILED;
    ssize_t n = recv(fd, &resp, sizeof(resp), 0);
    if (n < 0)
        return SNTPC_ERR_RECV_FAILED;
    if (n != sizeof(resp))
        return SNTPC_ERR_INVALID_RESP;
    if (SNTP_MODE_SERVER != (resp.li_vn_mode & SNTP_MODE_MASK))
        return SNTPC_ERR_INVALID_MODE;
    if (resp.stratum == SNTP_STRATUM_KOD)
        return SNTPC_ERR_KISS_OF_DEATH;
    if (resp.originate_timestamp[0] != req.transmit_timestamp[0] || resp.originate_timestamp[1] != req.transmit_timestamp[1])
        return SNTPC_ERR_INVALID_ORG_TS;
    double t0, t1, t2, t3 = systime_fn();
    SNTP_TIMESTAMP2TIME(ntohl(resp.originate_timestamp[0]), ntohl(resp.originate_timestamp[1]), t0);
    SNTP_TIMESTAMP2TIME(ntohl(resp.receive_timestamp[0]), ntohl(resp.receive_timestamp[1]), t1);
    SNTP_TIMESTAMP2TIME(ntohl(resp.transmit_timestamp[0]), ntohl(resp.transmit_timestamp[1]), t2);
    result->offset = ((t1 - t0) + (t2 - t3)) / 2;
    result->delay = (t3 - t0) - (t2 - t1);
    result->c_time = t3 + result->offset;
    return 0;
}

int sntpc_perform(const char *server, int timeout_ms, sntpc_systime_fn_t systime_fn, struct sntpc_result *result)
{
    int fd, ret = new_transport_fd(&fd, server);
    if (ret != 0)
        goto EXIT;
    ret = sntpc_perform_by_fd(fd, timeout_ms, systime_fn, result);
    close(fd);
EXIT:
    return ret;
}

/**
 * @file sntpc.h
 * @brief SNTP client APIs declaration
 * @author herve-ves
 * @date 2023-03-26
 */

#ifndef __SNTPC_H__
#define __SNTPC_H__

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief   Error return code definitions
 */
enum sntpc_err {
    SNTPC_ERR_INVALID_ARGS = -0xff,
    SNTPC_ERR_SOCKET_FAILED,
    SNTPC_ERR_DNS_FAILED,
    SNTPC_ERR_CONNECT_FAILED,
    SNTPC_ERR_SEND_FAILED,
    SNTPC_ERR_SELECT_FAILED,
    SNTPC_ERR_TIMEOUTED,
    SNTPC_ERR_RECV_FAILED,
    SNTPC_ERR_INVALID_RESP,
    SNTPC_ERR_INVALID_MODE,
    SNTPC_ERR_INVALID_ORG_TS,
    SNTPC_ERR_KISS_OF_DEATH,
};

/**
 * @brief   The type definition of the function of get system time
 * @return      double                  [Seconds since 1970-01-01]
 */
typedef double (*sntpc_systime_fn_t)(void);

/**
 * @brief   The definition of result of SNTPC request
 */
struct sntpc_result {
    double offset; // The calculated time offset (server's time minus client's time)
    double delay;  // The calculated round-trip delay
    double c_time; // The compensated client time
};

/**
 * @brief   Send SNTP request by given socket FD and return result
 * @param[in]   fd                      [UDP socket FD]
 * @param[in]   timeout_ms              [Timeout MS of waiting server response]
 * @param[in]   systime_fn              [Get system time function]
 * @param[out]  result                  [Result]
 * @return      int                     [0: OK, negative: enum sntpc_err]
 */
int sntpc_perform_by_fd(int fd, int timeout_ms, sntpc_systime_fn_t systime_fn, struct sntpc_result *result);

/**
 * @brief   Send SNTP request by given server host-name and return result
 * @param[in]   server                  [SNTP server host-name]
 * @param[in]   timeout_ms              [Timeout MS of waiting server response]
 * @param[in]   systime_fn              [Get system time function]
 * @param[out]  result                  [Result]
 * @return      int                     [0: OK, negative: enum sntpc_err]
 */
int sntpc_perform(const char *server, int timeout_ms, sntpc_systime_fn_t systime_fn, struct sntpc_result *result);

#if defined(__cplusplus)
}
#endif
#endif /* __SNTPC_H__ */

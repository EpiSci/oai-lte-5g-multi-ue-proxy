#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>

#ifndef __cplusplus
#include <stdbool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include "nfapi_interface.h"
#include "nfapi_pnf_interface.h"
#include "nfapi.h"
#include "debug.h"

#define ERR strerror(errno)
#define NUM_ITEMS(A) (sizeof(A) / sizeof((A)[0]))

typedef struct phy_info
{
    bool enabled;
    uint16_t phy_id;
    uint16_t sfn_sf;

    pthread_t thread;

    int pnf_p7_port;
    char *pnf_p7_addr;

    int vnf_p7_port;
    char *vnf_p7_addr;

    nfapi_pnf_p7_config_t *config;

} phy_info_t;

typedef struct pnf_info
{
    uint8_t num_phys;
    phy_info_t phys[8];

} pnf_info_t;

typedef struct
{
    bool enabled;
    bool started;
    uint16_t phy_id;

    int p7_rx_port;
    char *p7_rx_addr;

    struct sockaddr_in p7_rx_sockaddr;
    int p7_rx_sock;

    struct sockaddr_in p7_tx_sockaddr;
    int p7_tx_sock;
} pnf_config_phy_t;

typedef struct
{
    bool enabled;

    char const *vnf_p5_addr;
    int vnf_p5_port;

    struct sockaddr_in p5_tx_sockaddr;
    int p5_sock;

    pnf_config_phy_t phys[4];
    char name[16];

} pnf_config_t;

typedef enum
{
    None = 0,
    P5Msg = 1,
    P7Msg = 2

} MsgType;

typedef struct
{
    uint32_t message_size;
    uint8_t read_buffer[1024];
    MsgType mType;
    int header_id;
} nf_msg;

void nfapi_logfile(char const *caller, char const *label,
                   const char *format, ...)
__attribute__((format(printf, 3, 4)));
#define nfapi_error(...) nfapi_logfile(__func__, "ERROR", __VA_ARGS__)
#define nfapi_info(...) nfapi_logfile(__func__, "INFO", __VA_ARGS__)
#define nfapi_debug(...) nfapi_logfile(__func__, "DEBUG", __VA_ARGS__)

void show_backtrace(void);

pnf_config_phy_t *find_pnf_phy_config(pnf_config_t *config, uint16_t phy_id);

const char *hexdump(const void *data, size_t data_len, char *out, size_t out_len);
const char *hexdumpP5(const void *data, size_t data_len, char *out, size_t out_len);
const char *hexdumpP7(const void *data, size_t data_len, char *out, size_t out_len);

int create_p5_listen_socket(pnf_config_t *config);
void pnf_create_p5_sock(pnf_config_t *config);
int pnf_p5_connect(pnf_config_t *config);

void create_p7_rx_socket(pnf_config_t *config, int phy_id, int port);
void create_p7_tx_socket(pnf_config_t *config, int phy_id, int port);

int SendOAIMsg(pnf_config_t *config, uint8_t *msgBuffer, uint32_t msgLen);
int SendOAIP7Msg(pnf_config_t *config, uint8_t *msgBuffer, uint32_t msgLen);
int PnfReadMsg(pnf_config_t *pnf, nf_msg *pnf_msg);
int ReadP5Msg(pnf_config_t *pnf, nf_msg *p5nf_msg);
int ReadP7Msg(pnf_config_phy_t *pnf_p7, nf_msg *p7nf_msg);
int selectReadSocket(pnf_config_t *pnf, nf_msg *pnf_msg);
int checkMsgType(nfapi_message_id_e header);

pnf_config_phy_t *get_pnf_phy_from_p7_rx_socket(pnf_config_t *, int);
void close_pnf_p5_socket(pnf_config_t *, int);
void close_pnf_p7_socket(pnf_config_t *, int);

int Get_p7_rnti(nfapi_p7_message_header_t * header, uint8_t * buffer, size_t bufferLen);

uint64_t clock_usec(void);

void log_scheduler(const char* label);

const char *nfapi_get_message_id(const void *msg, size_t length);
uint16_t nfapi_get_sfnsf(const void *msg, size_t length);

#ifdef __cplusplus
}
#endif


/*
#ifdef __cplusplus

#include <string>

inline std::string hexdump(const void *data, size_t data_len)
{
    char buf[512];
    return hexdump(data, data_len, buf, sizeof(buf));
}

inline std::string hexdumpP5(const void *data, size_t data_len)
{
    char buf[512];
    return hexdumpP5(data, data_len, buf, sizeof(buf));
}

inline std::string hexdumpP7(const void *data, size_t data_len)
{
    char buf[512];
    return hexdumpP7(data, data_len, buf, sizeof(buf));
}

#endif // __cplusplus
*/
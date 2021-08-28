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
    nf_msg_type_none = 0,
    nf_msg_type_p5 = 1,
    nf_msg_type_p7 = 2

} nf_msg_type_t;

typedef struct nf_msg_t
{
    uint32_t msg_size;
    uint8_t read_buffer[1024];
    nf_msg_type_t msg_type;
    int header_id;
} nf_msg_t;

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
int PnfReadMsg(pnf_config_t *pnf, nf_msg_t *pnf_msg);
int ReadP5Msg(pnf_config_t *pnf, nf_msg_t *p5nf_msg);
int ReadP7Msg(pnf_config_phy_t *pnf_p7, nf_msg_t *p7nf_msg);
int selectReadSocket(pnf_config_t *pnf, nf_msg_t *pnf_msg);
int checkMsgType(nfapi_message_id_e header);

pnf_config_phy_t *get_pnf_phy_from_p7_rx_socket(pnf_config_t *, int);
void close_pnf_p5_socket(pnf_config_t *, int);
void close_pnf_p7_socket(pnf_config_t *, int);

int Get_p7_rnti(nfapi_p7_message_header_t * header, uint8_t * buffer, size_t bufferLen);

uint64_t clock_usec(void);

void log_scheduler(const char* label);

const char *nfapi_get_message_id(const void *msg, size_t length);
const char *nfapi_nr_get_message_id(const void *msg, size_t length);
uint16_t nfapi_get_sfnsf(const void *msg, size_t length);
uint16_t nfapi_get_sfnslot(const void *msg, size_t length);

#ifdef __cplusplus
}
#endif

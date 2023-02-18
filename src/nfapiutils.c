#/*
# * Licensed to the EPYSYS SCIENCE (EpiSci) under one or more
# * contributor license agreements.
# * The EPYSYS SCIENCE (EpiSci) licenses this file to You under
# * the Episys Science (EpiSci) Public License (Version 1.1) (the "License"); you may not use this file
# * except in compliance with the License.
# * You may obtain a copy of the License at
# *
# *      https://github.com/EpiSci/oai-lte-5g-multi-ue-proxy/blob/master/LICENSE
# *
# * Unless required by applicable law or agreed to in writing, software
# * distributed under the License is distributed on an "AS IS" BASIS,
# * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# * See the License for the specific language governing permissions and
# * limitations under the License.
# *-------------------------------------------------------------------------------
# * For more information about EPYSYS SCIENCE (EpiSci):
# *      bo.ryu@episci.com
# */

#define _GNU_SOURCE

#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <execinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "nfapiutils.h"
#include "debug.h"

const char *nfapi_get_message_id(const void *msg, size_t length)
{
    uint16_t phy_id = ~0;
    uint16_t message_id = ~0;
    uint8_t *in = (void *)msg;
    uint8_t *end = in + length;
    pull16(&in, &phy_id, end);
    pull16(&in, &message_id, end);

    switch (message_id)
    {
    case NFAPI_RACH_INDICATION: return "RACH";
    case NFAPI_CRC_INDICATION: return "CRC";
    case NFAPI_RX_ULSCH_INDICATION: return "RX";
    case NFAPI_RX_CQI_INDICATION: return "CQI";
    case NFAPI_HARQ_INDICATION: return "HARQ";
    case NFAPI_RX_SR_INDICATION: return "SR";
    case 0: return "Dummy";
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return "Unknown";
    }
}

const char *nfapi_nr_get_message_id(const void *msg, size_t length)
{
    uint16_t phy_id = ~0;
    uint16_t message_id = ~0;
    uint8_t *in = (void *)msg;
    uint8_t *end = in + length;
    pull16(&in, &phy_id, end);
    pull16(&in, &message_id, end);

    switch (message_id)
    {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION: return "RACH_NR";
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION: return "CRC_NR";
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION: return "RX_DATA_NR";
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION: return "UCI_NR";
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION: return "SRS_NR";
    case 0: return "Dummy";
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return "Unknown";
    }
}

uint16_t nfapi_get_sfnsf(const void *msg, size_t length)
{
    uint8_t *in = (void *)msg;
    uint8_t *end = in + length;
    uint16_t phy_id;
    uint16_t message_id;
    if (!pull16(&in, &phy_id, end) ||
        !pull16(&in, &message_id, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve message_id");
        return ~0;
    }

    switch (message_id)
    {
    case NFAPI_RACH_INDICATION:
    case NFAPI_CRC_INDICATION:
    case NFAPI_RX_ULSCH_INDICATION:
    case NFAPI_RX_CQI_INDICATION:
    case NFAPI_HARQ_INDICATION:
    case NFAPI_RX_SR_INDICATION:
    case 0:
        break;
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return ~0;
    }

    in = (uint8_t *)msg + sizeof(nfapi_p7_message_header_t);
    uint16_t sfn_sf;
    if (!pull16(&in, &sfn_sf, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve sfn_sf");
        return ~0;
    }
    return sfn_sf;
}

uint16_t nfapi_get_sfnslot(const void *msg, size_t length)
{
    uint8_t *in = (void *)msg;
    uint8_t *end = in + length;
    uint16_t phy_id;
    uint16_t message_id;
    if (!pull16(&in, &phy_id, end) ||
        !pull16(&in, &message_id, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve message_id");
        return ~0;
    }

    switch (message_id)
    {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
    case 0:
        break;
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return ~0;
    }

    in = (uint8_t *)msg + sizeof(nfapi_p7_message_header_t);
    uint16_t sfn, slot;
    if (!pull16(&in, &sfn, end) ||
        !pull16(&in, &slot, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve sfn and slot");
        return ~0;
    }
    return NFAPI_SFNSLOT2HEX(sfn, slot);
}

pnf_config_phy_t *find_pnf_phy_config(pnf_config_t *config,
                                      uint16_t phy_id)
{
    for (size_t i = 0; i < NUM_ITEMS(config->phys); ++i)
    {
        if (config->phys[i].phy_id == phy_id)
        {
            return &config->phys[i];
        }
    }
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "No pnf phy found - phy_id %d", phy_id);
    return NULL;
}

void log_scheduler(const char* label)
{
    int policy = sched_getscheduler(0);
    struct sched_param param;
    if (sched_getparam(0, &param) == -1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "sched_getparam: %s", strerror(errno));
        abort();
    }

    cpu_set_t cpu_set;
    if (sched_getaffinity(0, sizeof(cpu_set), &cpu_set) == -1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "sched_getaffinity: %s", strerror(errno));
        abort();
    }
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_cpus < 1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "sysconf(_SC_NPROCESSORS_ONLN): %s", strerror(errno));
        abort();
    }
    char buffer[num_cpus];
    for (int i = 0; i < num_cpus; i++)
    {
        buffer[i] = CPU_ISSET(i, &cpu_set) ? 'Y' : '-';
    }

    NFAPI_TRACE(NFAPI_TRACE_INFO, "Scheduler policy=%d priority=%d affinity=[%d]%.*s label=%s",
                policy,
                param.sched_priority,
                num_cpus,
                num_cpus,
                buffer,
                label);
}

int create_p5_listen_socket(pnf_config_t *config)
{
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sd == -1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
        abort();
    }

    config->p5_tx_sockaddr.sin_family = AF_INET;
    config->p5_tx_sockaddr.sin_port = htons(config->vnf_p5_port);
    config->p5_tx_sockaddr.sin_addr.s_addr = inet_addr(config->vnf_p5_addr);

    if (bind(sd, (struct sockaddr *)&config->p5_tx_sockaddr,
             sizeof(config->p5_tx_sockaddr)) < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "bind: %s", ERR);
        close(sd);
        return -1;
    }

    NFAPI_TRACE(NFAPI_TRACE_INFO, "Bind for %s %s:%d",
                config->name,
                inet_ntoa(config->p5_tx_sockaddr.sin_addr),
                ntohs(config->p5_tx_sockaddr.sin_port));
    return sd;
}

void pnf_create_p5_sock(pnf_config_t *config)
{
    int sd = socket(AF_INET, SOCK_STREAM, IPPROTO_SCTP);
    if (sd == -1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
        abort();
    }
    config->p5_sock = sd;
    config->p5_tx_sockaddr.sin_family = AF_INET;
    config->p5_tx_sockaddr.sin_port = htons(config->vnf_p5_port);
    config->p5_tx_sockaddr.sin_addr.s_addr = inet_addr(config->vnf_p5_addr);
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Got socket for %s %s:%d",
                config->name,
                inet_ntoa(config->p5_tx_sockaddr.sin_addr),
                ntohs(config->p5_tx_sockaddr.sin_port));
}

int pnf_p5_connect(pnf_config_t *config)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "connect %d to %s:%d...",
                config->p5_sock,
                inet_ntoa(config->p5_tx_sockaddr.sin_addr),
                ntohs(config->p5_tx_sockaddr.sin_port));
    int sd = connect(config->p5_sock,
                     (struct sockaddr *)&config->p5_tx_sockaddr,
                     sizeof(config->p5_tx_sockaddr));
    if (sd == -1)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "connect: %s", ERR);
        return -1;
    }
    NFAPI_TRACE(NFAPI_TRACE_INFO, "connect...done: %d", sd);
    return sd;
}

void create_p7_rx_socket(pnf_config_t *config, int phy_id, int port)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "phy id=%d port=%d", phy_id, port);

    pnf_config_phy_t *phy = find_pnf_phy_config(config, phy_id);
    if (!phy)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "find_pnf_phy_config");
        return;
    }

    // If connection was already created, don't repeat
    if (phy->started)
    {
        NFAPI_TRACE(NFAPI_TRACE_INFO, "already started");
        return;
    }

    phy->p7_rx_port = port;
    phy->started = true;
    phy->p7_rx_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (phy->p7_rx_sock < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
        return;
    }

    int reuseaddr_enable = 1;
    if (setsockopt(phy->p7_rx_sock, SOL_SOCKET, SO_REUSEADDR,
                   &reuseaddr_enable, sizeof(reuseaddr_enable)) < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "setsockopt: %s", ERR);
        return;
    }

    phy->p7_rx_sockaddr.sin_family = AF_INET;
    phy->p7_rx_sockaddr.sin_port = htons(phy->p7_rx_port);
    phy->p7_rx_sockaddr.sin_addr.s_addr = INADDR_ANY;
    //sock_addr.sin_addr.s_addr = inet_addr(addr);

    if (bind(phy->p7_rx_sock, (struct sockaddr *)&phy->p7_rx_sockaddr,
             sizeof(phy->p7_rx_sockaddr)) < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "bind: %s", ERR);
        return;
    }

    NFAPI_TRACE(NFAPI_TRACE_INFO, "Got socket for phy %d %s %s:%d",
                phy_id, config->name,
                inet_ntoa(phy->p7_rx_sockaddr.sin_addr),
                ntohs(phy->p7_rx_sockaddr.sin_port));
}

void create_p7_tx_socket(pnf_config_t *config, int phy_id, int port)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "phy id=%d port=%d", phy_id, port);

    pnf_config_phy_t *phy = find_pnf_phy_config(config, phy_id);
    if (!phy)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "find_pnf_phy_config");
        return;
    }

    int sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sd < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
        return;
    }

    phy->p7_tx_sock = sd;
    phy->p7_tx_sockaddr.sin_family = AF_INET;
    phy->p7_tx_sockaddr.sin_port = htons(port);
    phy->p7_tx_sockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");  // TODO: Is this the right IP?

    NFAPI_TRACE(NFAPI_TRACE_INFO, "Got socket for %s on %s:%d",
                config->name,
                inet_ntoa(phy->p7_tx_sockaddr.sin_addr),
                ntohs(phy->p7_tx_sockaddr.sin_port));
}

static const char log_name[] = "nfapi.log";

#define MSG( X ) r = write(fd, X, sizeof(X) - 1)

void show_backtrace(void)
{
    void *buffer[100];
    __attribute__((unused)) int r;
    int nptrs = backtrace(buffer, sizeof(buffer) / sizeof(buffer[0]));
    int fd = open(log_name, O_APPEND|O_CREAT|O_WRONLY, 0666);
    MSG("---stack trace---\n");
    backtrace_symbols_fd(buffer, nptrs, fd);
    MSG("---end stack trace---\n");
    close(fd);
}

#define X(ID) case NFAPI_ ## ID: return #ID

const char *nfap_message_id_to_string(int id)
{
    switch (id)
    {
        X(DL_CONFIG_REQUEST);
        X(UL_CONFIG_REQUEST);
        X(SUBFRAME_INDICATION);
        X(HI_DCI0_REQUEST);
        X(TX_REQUEST);
        X(HARQ_INDICATION);
        X(CRC_INDICATION);
        X(RX_ULSCH_INDICATION);
        X(RACH_INDICATION);
        X(SRS_INDICATION);
        X(RX_SR_INDICATION);
        X(RX_CQI_INDICATION);
        X(LBT_DL_CONFIG_REQUEST);
        X(LBT_DL_INDICATION);
        X(NB_HARQ_INDICATION);
        X(NRACH_INDICATION);
        X(UE_RELEASE_REQUEST);
        X(UE_RELEASE_RESPONSE);
        X(PNF_PARAM_REQUEST);
        X(PNF_PARAM_RESPONSE);
        X(PNF_CONFIG_REQUEST);
        X(PNF_CONFIG_RESPONSE);
        X(PNF_START_REQUEST);
        X(PNF_START_RESPONSE);
        X(PNF_STOP_REQUEST);
        X(PNF_STOP_RESPONSE);
        X(PARAM_REQUEST);
        X(PARAM_RESPONSE);
        X(CONFIG_REQUEST);
        X(CONFIG_RESPONSE);
        X(START_REQUEST);
        X(START_RESPONSE);
        X(STOP_REQUEST);
        X(STOP_RESPONSE);
        X(MEASUREMENT_REQUEST);
        X(MEASUREMENT_RESPONSE);
        X(UL_NODE_SYNC);
        X(DL_NODE_SYNC);
        X(TIMING_INFO);
        X(RSSI_REQUEST);
        X(RSSI_RESPONSE);
        X(RSSI_INDICATION);
        X(CELL_SEARCH_REQUEST);
        X(CELL_SEARCH_RESPONSE);
        X(CELL_SEARCH_INDICATION);
        X(BROADCAST_DETECT_REQUEST);
        X(BROADCAST_DETECT_RESPONSE);
        X(BROADCAST_DETECT_INDICATION);
        X(SYSTEM_INFORMATION_SCHEDULE_REQUEST);
        X(SYSTEM_INFORMATION_SCHEDULE_RESPONSE);
        X(SYSTEM_INFORMATION_SCHEDULE_INDICATION);
        X(SYSTEM_INFORMATION_REQUEST);
        X(SYSTEM_INFORMATION_RESPONSE);
        X(SYSTEM_INFORMATION_INDICATION);
        X(NMM_STOP_REQUEST);
        X(NMM_STOP_RESPONSE);
        X(VENDOR_EXT_MSG_MIN);
        X(VENDOR_EXT_MSG_MAX);
    }
    return "*UNKNOWN ID*";
}

#undef X

const char *hexdumpP5(const void *data, size_t data_len, char *out, size_t out_len)
{
    nfapi_p4_p5_message_header_t header;
    if (nfapi_p5_message_header_unpack((void *) data, data_len,
                                       &header, sizeof(header), NULL) < 0)
    {
        snprintf(out, out_len, "P5 *MALFORMED* ");
    }
    else
    {
        snprintf(out, out_len, "P5 %s ", nfap_message_id_to_string(header.message_id));
    }
    int n = strlen(out);
    hexdump(data, data_len, out + n, out_len - n);
    return out;
}

const char *hexdumpP7(const void *data, size_t data_len, char *out, size_t out_len)
{
    nfapi_p7_message_header_t header;
    if (nfapi_p7_message_header_unpack((void *) data, data_len,
                                       &header, sizeof(header), NULL) < 0)
    {
        snprintf(out, out_len, "P7 *MALFORMED* ");
    }
    else
    {
        snprintf(out, out_len, "P7 %s ", nfap_message_id_to_string(header.message_id));
    }
    int n = strlen(out);
    hexdump(data, data_len, out + n, out_len - n);
    return out;
}

const char *hexdump(const void *data, size_t data_len, char *out, size_t out_len)
{
    char *p = out;
    char *endp = out + out_len;
    const uint8_t *q = data;
    snprintf(p, endp - p, "[%zu]", data_len);
    p += strlen(p);
    for (size_t i = 0; i < data_len; ++i)
    {
        if (p >= endp)
        {
            static const char ellipses[] = "...";
            char *s = endp - sizeof(ellipses);
            if (s >= p)
            {
                strcpy(s, ellipses);
            }
            break;
        }
        snprintf(p, endp - p, " %02X", *q++);
        p += strlen(p);
    }
    return out;
}

// TODO: What are we actually checking here?
// And what does the return value mean?
// Consider renaming this function to make these things clearer.
int checkMsgType(nfapi_message_id_e header)
{
    switch (header)
    {
    case NFAPI_UL_NODE_SYNC:
    case NFAPI_TIMING_INFO:
    case NFAPI_HARQ_INDICATION:
    case NFAPI_CRC_INDICATION:
    case NFAPI_RX_ULSCH_INDICATION:
    case NFAPI_RACH_INDICATION:
    case NFAPI_SRS_INDICATION:
    case NFAPI_RX_SR_INDICATION:
    case NFAPI_RX_CQI_INDICATION:
    case NFAPI_LBT_DL_INDICATION:
    case NFAPI_NB_HARQ_INDICATION:
    case NFAPI_NRACH_INDICATION:
    case NFAPI_UE_RELEASE_RESPONSE:
        return 0;

    default:
        return 1;
    }
}

pnf_config_phy_t *get_pnf_phy_from_p7_rx_socket(pnf_config_t *config, int sd)
{
    for (size_t i = 0; i < NUM_ITEMS(config->phys); ++i)
    {
        pnf_config_phy_t *phy = &config->phys[i];
        if (sd == phy->p7_rx_sock)
        {
            return phy;
        }
    }
    return NULL;
}

void close_pnf_p5_socket(pnf_config_t *config, int sd)
{
    if (sd != config->p5_sock)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "wrong socket %d != %d", sd, config->p5_sock);
        return;
    }
    config->p5_sock = -1;
    close(sd);
}

void close_pnf_p7_socket(pnf_config_t *config, int sd)
{
    for (size_t i = 0; i < NUM_ITEMS(config->phys); ++i)
    {
        pnf_config_phy_t *phy = &config->phys[i];
        if (sd == phy->p7_rx_sock)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "close p7_rx_sock %d", sd);
            phy->p7_rx_sock = -1;
            close(sd);
            return;
        }
        if (sd == phy->p7_tx_sock)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "close p7_tx_sock %d", sd);
            phy->p7_tx_sock = -1;
            close(sd);
            return;
        }
    }
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "wrong socket %d", sd);
}

int Get_p7_rnti(nfapi_p7_message_header_t *header, uint8_t *buffer, size_t bufferLen)
{
    uint8_t lBuffer[1024];

    if (nfapi_p7_message_unpack(buffer, bufferLen, lBuffer, sizeof(lBuffer), 0) < 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message Unpack Error");
        return -1;
    }

    switch (header->message_id)
    {
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 GetRNTI Error - Unkown msg id: %d", header->message_id);
        return -1;

    case NFAPI_DL_CONFIG_REQUEST:
    {
        //nfapi_dl_config_request_t* dl_config = (nfapi_dl_config_request_t*)lBuffer;
        return -2;
    }

    case NFAPI_UL_CONFIG_REQUEST:
    {
        nfapi_ul_config_request_t *ul_config = (nfapi_ul_config_request_t *)lBuffer;
        NFAPI_TRACE(NFAPI_TRACE_INFO, "ulsch_pdu: %d", ul_config->ul_config_request_body.ul_config_pdu_list->ulsch_pdu.ulsch_pdu_rel8.rnti);
        NFAPI_TRACE(NFAPI_TRACE_INFO, "srs_pdu: %d", ul_config->ul_config_request_body.ul_config_pdu_list->srs_pdu.srs_pdu_rel8.rnti);
        return 0;
    }

    case NFAPI_TX_REQUEST:
        // Don't see a RNTI field
        return -1;

    case NFAPI_HI_DCI0_REQUEST:
        // Don't see a RNTI field
        return -1;

    case NFAPI_DL_NODE_SYNC:
        // Don't see a RNTI field
        return -1;

    case NFAPI_RACH_INDICATION:
        // Don't see a RNTI field
    {
        nfapi_rach_indication_t *rach_ind = (nfapi_rach_indication_t *)lBuffer;
        NFAPI_TRACE(NFAPI_TRACE_INFO, "rach indic: %d", rach_ind->rach_indication_body.preamble_list->preamble_rel8.rnti);

        return 0;
    }
    }

    return -1;
}

uint64_t clock_usec()
{
    struct timespec t;
    if (clock_gettime(CLOCK_MONOTONIC, &t) == -1)
    {
        abort();
    }
    return (uint64_t)t.tv_sec * 1000000 + (t.tv_nsec / 1000);
}

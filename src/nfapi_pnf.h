/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "nfapi_pnf_interface.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "nfapiutils.h"

#include "nfapi.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <assert.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "nfapi_interface.h"

#include "fapi_stub.h"

#define MAX_SUBFRAME_MSGS 8
#define MAX_SLOT_MSGS 8

//typedef int64_t openair0_timestamp;


typedef struct
{
    uint8_t enabled;
    uint32_t rx_port;
    uint32_t tx_port;
    char tx_addr[80];
} udp_data;

typedef struct
{

} PHY_VARS_eNB;

typedef struct
{

} PHY_VARS_gNB;

typedef struct
{
    uint16_t index;
    uint16_t id;
    uint8_t rfs[2];
    uint8_t excluded_rfs[2];

    udp_data udp;

    char local_addr[80];
    int local_port;

    char *remote_addr;
    int remote_port;

    uint8_t duplex_mode;
    uint16_t dl_channel_bw_support;
    uint16_t ul_channel_bw_support;
    uint8_t num_dl_layers_supported;
    uint8_t num_ul_layers_supported;
    uint16_t release_supported;
    uint8_t nmm_modes_supported;

    uint8_t dl_ues_per_subframe;
    uint8_t ul_ues_per_subframe;

    uint8_t first_subframe_ind;

    // timing information recevied from the vnf
    uint8_t timing_window;
    uint8_t timing_info_mode;
    uint8_t timing_info_period;

} phy_info;

typedef struct
{
    //public:
    uint16_t index;
    uint16_t band;
    int16_t max_transmit_power;
    int16_t min_transmit_power;
    uint8_t num_antennas_supported;
    uint32_t min_downlink_frequency;
    uint32_t max_downlink_frequency;
    uint32_t max_uplink_frequency;
    uint32_t min_uplink_frequency;
} rf_info;

typedef struct
{

    int release;
    phy_info phys[2];
    rf_info rfs[2];

    uint8_t sync_mode;
    uint8_t location_mode;
    uint8_t location_coordinates[6];
    uint32_t dl_config_timing;
    uint32_t ul_config_timing;
    uint32_t tx_timing;
    uint32_t hi_dci0_timing;

    uint16_t max_phys;
    uint16_t max_total_bw;
    uint16_t max_total_dl_layers;
    uint16_t max_total_ul_layers;
    uint8_t shared_bands;
    uint8_t shared_pa;
    int16_t max_total_power;
    uint8_t oui;

    uint8_t wireshark_test_mode;

} pnf_info;

typedef struct
{
    uint16_t phy_id;
    nfapi_pnf_config_t *config;
    phy_info *phy;
    nfapi_pnf_p7_config_t *p7_config;
} pnf_phy_user_data_t;

typedef struct message_buffer_t
{
    uint32_t magic;             // for sanity checking
#   define MESSAGE_BUFFER_MAGIC 0x45504953 // arbitrary value
    size_t length;              // number of valid bytes in .data[]
    uint8_t data[NFAPI_RX_IND_DATA_MAX];
} message_buffer_t;

// subframe_msgs_t holds all of the messages
// for a specific UE per sfn_sf
typedef struct subframe_msgs_t
{
    size_t num_msgs;
    message_buffer_t *msgs[MAX_SUBFRAME_MSGS];
} subframe_msgs_t;

// slot_msgs_t holds all of the messages
// for a specific UE per sfn_sf
typedef struct slot_msgs_t
{
    size_t num_msgs;
    message_buffer_t *msgs[MAX_SLOT_MSGS];
} slot_msgs_t;

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind);
int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *ind);
int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);

int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind);
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind);
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind);
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind);
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind);

void oai_subframe_ind(uint16_t sfn, uint16_t sf);
void oai_slot_ind(uint16_t sfn, uint16_t slot);

void configure_nfapi_pnf(const char *vnf_ip_addr, int vnf_p5_port, const char *pnf_ip_addr, int pnf_p7_port,
                         int vnf_p7_port);
void configure_nr_nfapi_pnf(const char *vnf_ip_addr, int vnf_p5_port, const char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port);

void init_eNB_afterRU(void);
void init_UE_stub(int nb_inst, int, int);

void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB, int frame, int subframe, L1_rxtx_proc_t *proc,
                             nfapi_dl_config_request_pdu_t *dl_config_pdu);
void handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame, uint8_t subframe, uint8_t srs_present);
void handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB, int frame, int subframe, L1_rxtx_proc_t *proc,
                            nfapi_dl_config_request_pdu_t *dl_config_pdu, uint8_t codeword_index, uint8_t *sdu);
void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB, int frame, int subframe, L1_rxtx_proc_t *proc,
                                  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);
void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB, int frame, int subframe, L1_rxtx_proc_t *proc,
                                 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);
void handle_nfapi_bch_pdu(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, nfapi_dl_config_request_pdu_t *dl_config_pdu,
                          uint8_t *sdu);

void handle_nfapi_nr_ul_dci_pdu(PHY_VARS_gNB *gNB,
                   int frame, int slot,
                   nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu);
void handle_nfapi_nr_pdcch_pdu(PHY_VARS_gNB *gNB,
                   int frame, int slot,
                   nfapi_nr_dl_tti_pdcch_pdu *pdcch_pdu);
void handle_nr_nfapi_pdsch_pdu(PHY_VARS_gNB *gNB,int frame,int slot,
                            nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                            uint8_t *sdu);
void handle_nr_nfapi_ssb_pdu(PHY_VARS_gNB *gNB,int frame,int slot,
                             nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdu);

void *oai_subframe_task(void *context);
void *oai_slot_task(void *context);
void oai_subframe_init();
void oai_slot_init();
void oai_subframe_flush_msgs_from_ue();
void oai_subframe_handle_msg_from_ue(const void *msg, size_t len, uint16_t pktsrc_index);
void oai_slot_handle_msg_from_ue(const void *msg, size_t len, uint16_t pktsrc_index);

void transfer_downstream_nfapi_msg_to_emane(void *msg);
void transfer_downstream_sfn_sf_to_emane(uint16_t sfn_sf);

uint16_t sfn_sf_add(uint16_t a, uint16_t add_val);
void sfn_slot_add(uint16_t *sfn, uint16_t *slot, uint16_t add_val);

int get_sf_delta(uint16_t a, uint16_t b);
int get_slot_delta(uint16_t a, uint16_t b);
uint16_t sfn_sf_subtract(uint16_t a, uint16_t sub_val);


bool dequeue_ue_msgs(subframe_msgs_t *subframe_msgs, uint16_t sfn_sf_tx);
void add_sleep_time(uint64_t start, uint64_t poll, uint64_t send, uint64_t agg);

extern int num_ues;

#define MAX_UES 64

#ifdef __cplusplus
}
#endif

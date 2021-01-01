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

#ifdef __cplusplus
extern "C" {
#endif

#include "nfapi_pnf.h"
#include "nfapi.h"
#include "queue.h"
#include <inttypes.h>

char uecap_xer_in;

nfapi_tx_request_pdu_t *tx_request_pdu[1023][10][10]; // [frame][subframe][max_num_pdus]
nfapi_ue_release_request_body_t release_rntis;
nfapi_pnf_param_response_t g_pnf_param_resp;
nfapi_pnf_p7_config_t *p7_config_g = NULL;

uint8_t tx_pdus[32][8][4096];
uint16_t phy_antenna_capability_values[] = { 1, 2, 4, 8, 16 };

static epi_pnf_info pnf;
static pthread_t pnf_start_pthread;

void *pnf_allocate(size_t size)
{
    return malloc(size);
}

void pnf_deallocate(void *ptr)
{
    free(ptr);
}

void epi_pnf_nfapi_trace(nfapi_trace_level_t nfapi_level, const char *message, ...)
{
    (void)nfapi_level;
    va_list args;
    va_start(args, message);
    va_end(args);
}

void pnf_set_thread_priority(int priority)
{
    struct sched_param schedParam =
    {
        .sched_priority = priority,
    };

    if (sched_setscheduler(0, SCHED_RR, &schedParam) != 0)
    {
        printf("failed to set SCHED_RR\n");
    }
}

void *pnf_p7_thread_start(void *ptr)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] P7 THREAD %s\n", __FUNCTION__);
    pnf_set_thread_priority(79);
    log_scheduler(__func__);
    nfapi_pnf_p7_config_t *config = (nfapi_pnf_p7_config_t *)ptr;
    nfapi_pnf_p7_start(config);

    return 0;
}

int pnf_param_request(nfapi_pnf_config_t *config, nfapi_pnf_param_request_t *req)
{

    (void)req;
    printf("[PNF] pnf param request\n");
    nfapi_pnf_param_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    resp.pnf_param_general.tl.tag = NFAPI_PNF_PARAM_GENERAL_TAG;
    resp.pnf_param_general.nfapi_sync_mode = pnf->sync_mode;
    resp.pnf_param_general.location_mode = pnf->location_mode;
    resp.pnf_param_general.dl_config_timing = pnf->dl_config_timing;
    resp.pnf_param_general.tx_timing = pnf->tx_timing;
    resp.pnf_param_general.ul_config_timing = pnf->ul_config_timing;
    resp.pnf_param_general.hi_dci0_timing = pnf->hi_dci0_timing;
    resp.pnf_param_general.maximum_number_phys = pnf->max_phys;
    resp.pnf_param_general.maximum_total_bandwidth = pnf->max_total_bw;
    resp.pnf_param_general.maximum_total_number_dl_layers = pnf->max_total_dl_layers;
    resp.pnf_param_general.maximum_total_number_ul_layers = pnf->max_total_ul_layers;
    resp.pnf_param_general.shared_bands = pnf->shared_bands;
    resp.pnf_param_general.shared_pa = pnf->shared_pa;
    resp.pnf_param_general.maximum_total_power = pnf->max_total_power;
    resp.pnf_phy.tl.tag = NFAPI_PNF_PHY_TAG;
    resp.pnf_phy.number_of_phys = 1;

    for (int i = 0; i < 1; ++i)
    {
        resp.pnf_phy.phy[i].phy_config_index = pnf->phys[i].index;
        resp.pnf_phy.phy[i].downlink_channel_bandwidth_supported = pnf->phys[i].dl_channel_bw_support;
        resp.pnf_phy.phy[i].uplink_channel_bandwidth_supported = pnf->phys[i].ul_channel_bw_support;
        resp.pnf_phy.phy[i].number_of_dl_layers_supported = pnf->phys[i].num_dl_layers_supported;
        resp.pnf_phy.phy[i].number_of_ul_layers_supported = pnf->phys[i].num_ul_layers_supported;
        resp.pnf_phy.phy[i].maximum_3gpp_release_supported = pnf->phys[i].release_supported;
        resp.pnf_phy.phy[i].nmm_modes_supported = pnf->phys[i].nmm_modes_supported;
        resp.pnf_phy.phy[i].number_of_rfs = 2;

        for (int j = 0; j < 1; ++j)
        {
            resp.pnf_phy.phy[i].rf_config[j].rf_config_index = pnf->phys[i].rfs[j];
        }

        resp.pnf_phy.phy[i].number_of_rf_exclusions = 0;

        for (int j = 0; j < 0; ++j)
        {
            resp.pnf_phy.phy[i].excluded_rf_config[j].rf_config_index = pnf->phys[i].excluded_rfs[j];
        }
    }

    resp.pnf_rf.tl.tag = NFAPI_PNF_RF_TAG;
    resp.pnf_rf.number_of_rfs = 2;

    for (int i = 0; i < 2; ++i)
    {
        resp.pnf_rf.rf[i].rf_config_index = pnf->rfs[i].index;
        resp.pnf_rf.rf[i].band = pnf->rfs[i].band;
        resp.pnf_rf.rf[i].maximum_transmit_power = pnf->rfs[i].max_transmit_power;
        resp.pnf_rf.rf[i].minimum_transmit_power = pnf->rfs[i].min_transmit_power;
        resp.pnf_rf.rf[i].number_of_antennas_suppported = pnf->rfs[i].num_antennas_supported;
        resp.pnf_rf.rf[i].minimum_downlink_frequency = pnf->rfs[i].min_downlink_frequency;
        resp.pnf_rf.rf[i].maximum_downlink_frequency = pnf->rfs[i].max_downlink_frequency;
        resp.pnf_rf.rf[i].minimum_uplink_frequency = pnf->rfs[i].min_uplink_frequency;
        resp.pnf_rf.rf[i].maximum_uplink_frequency = pnf->rfs[i].max_uplink_frequency;
    }

    if (pnf->release >= 10)
    {
        resp.pnf_phy_rel10.tl.tag = NFAPI_PNF_PHY_REL10_TAG;
        resp.pnf_phy_rel10.number_of_phys = 1;

        for (int i = 0; i < 1; ++i)
        {
            resp.pnf_phy_rel10.phy[i].phy_config_index = pnf->phys[i].index;
            resp.pnf_phy_rel10.phy[i].transmission_mode_7_supported = 0;
            resp.pnf_phy_rel10.phy[i].transmission_mode_8_supported = 1;
            resp.pnf_phy_rel10.phy[i].two_antenna_ports_for_pucch = 0;
            resp.pnf_phy_rel10.phy[i].transmission_mode_9_supported = 1;
            resp.pnf_phy_rel10.phy[i].simultaneous_pucch_pusch = 0;
            resp.pnf_phy_rel10.phy[i].four_layer_tx_with_tm3_and_tm4 = 1;
        }
    }

    if (pnf->release >= 11)
    {
        resp.pnf_phy_rel11.tl.tag = NFAPI_PNF_PHY_REL11_TAG;
        resp.pnf_phy_rel11.number_of_phys = 1;

        for (int i = 0; i < 1; ++i)
        {
            resp.pnf_phy_rel11.phy[i].phy_config_index = pnf->phys[i].index;
            resp.pnf_phy_rel11.phy[i].edpcch_supported = 0;
            resp.pnf_phy_rel11.phy[i].multi_ack_csi_reporting = 1;
            resp.pnf_phy_rel11.phy[i].pucch_tx_diversity = 0;
            resp.pnf_phy_rel11.phy[i].ul_comp_supported = 1;
            resp.pnf_phy_rel11.phy[i].transmission_mode_5_supported = 0;
        }
    }

    if (pnf->release >= 12)
    {
        resp.pnf_phy_rel12.tl.tag = NFAPI_PNF_PHY_REL12_TAG;
        resp.pnf_phy_rel12.number_of_phys = 1;

        for (int i = 0; i < 1; ++i)
        {
            resp.pnf_phy_rel12.phy[i].phy_config_index = pnf->phys[i].index;
            resp.pnf_phy_rel12.phy[i].csi_subframe_set = 0;
            resp.pnf_phy_rel12.phy[i].enhanced_4tx_codebook = 2;
            resp.pnf_phy_rel12.phy[i].drs_supported = 0;
            resp.pnf_phy_rel12.phy[i].ul_64qam_supported = 1;
            resp.pnf_phy_rel12.phy[i].transmission_mode_10_supported = 0;
            resp.pnf_phy_rel12.phy[i].alternative_bts_indices = 1;
        }
    }

    if (pnf->release >= 13)
    {
        resp.pnf_phy_rel13.tl.tag = NFAPI_PNF_PHY_REL13_TAG;
        resp.pnf_phy_rel13.number_of_phys = 1;

        for (int i = 0; i < 1; ++i)
        {
            resp.pnf_phy_rel13.phy[i].phy_config_index = pnf->phys[i].index;
            resp.pnf_phy_rel13.phy[i].pucch_format4_supported = 0;
            resp.pnf_phy_rel13.phy[i].pucch_format5_supported = 1;
            resp.pnf_phy_rel13.phy[i].more_than_5_ca_support = 0;
            resp.pnf_phy_rel13.phy[i].laa_supported = 1;
            resp.pnf_phy_rel13.phy[i].laa_ending_in_dwpts_supported = 0;
            resp.pnf_phy_rel13.phy[i].laa_starting_in_second_slot_supported = 1;
            resp.pnf_phy_rel13.phy[i].beamforming_supported = 0;
            resp.pnf_phy_rel13.phy[i].csi_rs_enhancement_supported = 1;
            resp.pnf_phy_rel13.phy[i].drms_enhancement_supported = 0;
            resp.pnf_phy_rel13.phy[i].srs_enhancement_supported = 1;
        }

        resp.pnf_phy_rel13_nb_iot.tl.tag = NFAPI_PNF_PHY_REL13_NB_IOT_TAG;
        resp.pnf_phy_rel13_nb_iot.number_of_phys = 1;

        for (int i = 0; i < 1; ++i)
        {
            resp.pnf_phy_rel13_nb_iot.phy[i].phy_config_index = pnf->phys[i].index;
            resp.pnf_phy_rel13_nb_iot.phy[i].number_of_rfs = 1;

            for (int j = 0; j < 1; ++j)
            {
                resp.pnf_phy_rel13_nb_iot.phy[i].rf_config[j].rf_config_index = pnf->phys[i].rfs[j];
            }

            resp.pnf_phy_rel13_nb_iot.phy[i].number_of_rf_exclusions = 1;

            for (int j = 0; j < 1; ++j)
            {
                resp.pnf_phy_rel13_nb_iot.phy[i].excluded_rf_config[j].rf_config_index = pnf->phys[i].excluded_rfs[j];
            }

            resp.pnf_phy_rel13_nb_iot.phy[i].number_of_dl_layers_supported = pnf->phys[i].num_dl_layers_supported;
            resp.pnf_phy_rel13_nb_iot.phy[i].number_of_ul_layers_supported = pnf->phys[i].num_ul_layers_supported;
            resp.pnf_phy_rel13_nb_iot.phy[i].maximum_3gpp_release_supported = pnf->phys[i].release_supported;
            resp.pnf_phy_rel13_nb_iot.phy[i].nmm_modes_supported = pnf->phys[i].nmm_modes_supported;
        }
    }

    nfapi_pnf_pnf_param_resp(config, &resp);

    return 0;
}

int pnf_config_request(nfapi_pnf_config_t *config, nfapi_pnf_config_request_t *req)
{

    printf("[PNF] pnf config request\n");
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    epi_phy_info *phy = pnf->phys;
    phy->id = req->pnf_phy_rf_config.phy_rf_config[0].phy_id;
    printf("[PNF] pnf config request assigned phy_id %d to phy_config_index %d\n", phy->id,
           req->pnf_phy_rf_config.phy_rf_config[0].phy_config_index);
    nfapi_pnf_config_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_pnf_config_resp(config, &resp);
    printf("[PNF] Sent pnf_config_resp\n");

    return 0;
}

void nfapi_send_pnf_start_resp(nfapi_pnf_config_t *config, uint16_t phy_id)
{
    printf("Sending NFAPI_START_RESPONSE config:%p phy_id:%d\n", config, phy_id);
    nfapi_start_response_t start_resp;
    memset(&start_resp, 0, sizeof(start_resp));
    start_resp.header.message_id = NFAPI_START_RESPONSE;
    start_resp.header.phy_id = phy_id;
    start_resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_start_resp(config, &start_resp);
}

int pnf_start_request(nfapi_pnf_config_t *config, nfapi_pnf_start_request_t *req)
{

    (void)req;
    printf("Received NFAPI_PNF_START_REQUEST\n");
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    // start all phys that have been configured
    epi_phy_info *phy = pnf->phys;

    if (phy->id != 0)
    {
        nfapi_pnf_start_response_t resp;
        memset(&resp, 0, sizeof(resp));
        resp.header.message_id = NFAPI_PNF_START_RESPONSE;
        resp.error_code = NFAPI_MSG_OK;
        nfapi_pnf_pnf_start_resp(config, &resp);
        printf("[PNF] Sent NFAPI_PNF_START_RESP\n");
    }

    return 0;
}

int pnf_stop_request(nfapi_pnf_config_t *config, nfapi_pnf_stop_request_t *req)
{

    (void)req;
    printf("[PNF] Received NFAPI_PNF_STOP_REQ\n");
    nfapi_pnf_stop_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_pnf_stop_resp(config, &resp);
    printf("[PNF] Sent NFAPI_PNF_STOP_REQ\n");

    return 0;
}

int param_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_param_request_t *req)
{

    (void)phy;
    printf("[PNF] Received NFAPI_PARAM_REQUEST phy_id:%d\n", req->header.phy_id);

    nfapi_param_response_t nfapi_resp;
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    memset(&nfapi_resp, 0, sizeof(nfapi_resp));
    nfapi_resp.header.message_id = NFAPI_PARAM_RESPONSE;
    nfapi_resp.header.phy_id = req->header.phy_id;
    nfapi_resp.error_code = 0; // DJP - what value???
    struct sockaddr_in pnf_p7_sockaddr;
    pnf_p7_sockaddr.sin_addr.s_addr = inet_addr(pnf->phys[0].local_addr);
    nfapi_resp.nfapi_config.p7_pnf_address_ipv4.tl.tag = NFAPI_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
    memcpy(nfapi_resp.nfapi_config.p7_pnf_address_ipv4.address, &pnf_p7_sockaddr.sin_addr.s_addr, 4);
    nfapi_resp.num_tlv++;
    // P7 PNF Port
    nfapi_resp.nfapi_config.p7_pnf_port.tl.tag = NFAPI_NFAPI_P7_PNF_PORT_TAG;
    nfapi_resp.nfapi_config.p7_pnf_port.value = 32123; // DJP - hard code alert!!!! FIXME TODO
    nfapi_resp.num_tlv++;
    nfapi_pnf_param_resp(config, &nfapi_resp);
    printf("[PNF] Sent NFAPI_PARAM_RESPONSE phy_id:%d number_of_tlvs:%u\n", req->header.phy_id, nfapi_resp.num_tlv);
    printf("[PNF] param request .. exit\n");

    return 0;
}

int config_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_config_request_t *req)
{

    (void)phy;
    printf("[PNF] Received NFAPI_CONFIG_REQ phy_id:%d\n", req->header.phy_id);
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    uint8_t num_tlv = 0;
    epi_phy_info *epi_phy_info = pnf->phys;

    if (req->nfapi_config.timing_window.tl.tag == NFAPI_NFAPI_TIMING_WINDOW_TAG)
    {
        epi_phy_info->timing_window = req->nfapi_config.timing_window.value;
        printf("epi_phy_info:Timing window:%u NFAPI_CONFIG:timing_window:%u\n", epi_phy_info->timing_window,
               req->nfapi_config.timing_window.value);
        num_tlv++;
    }

    if (req->nfapi_config.timing_info_mode.tl.tag == NFAPI_NFAPI_TIMING_INFO_MODE_TAG)
    {
        printf("timing info mode:%d\n", req->nfapi_config.timing_info_mode.value);
        epi_phy_info->timing_info_mode = req->nfapi_config.timing_info_mode.value;
        num_tlv++;
    }
    else
    {
        epi_phy_info->timing_info_mode = 0;
        printf("NO timing info mode provided\n");
    }

    if (req->nfapi_config.timing_info_period.tl.tag == NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG)
    {
        printf("timing info period provided value:%d\n", req->nfapi_config.timing_info_period.value);
        epi_phy_info->timing_info_period = req->nfapi_config.timing_info_period.value;
        num_tlv++;
    }
    else
    {
        epi_phy_info->timing_info_period = 0;
    }

    if (req->rf_config.dl_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG)
    {
        epi_phy_info->dl_channel_bw_support = req->rf_config.dl_channel_bandwidth.value;
        num_tlv++;
    }
    else
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() Missing NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG\n", __FUNCTION__);
    }

    if (req->rf_config.ul_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG)
    {
        epi_phy_info->ul_channel_bw_support = req->rf_config.ul_channel_bandwidth.value;
        num_tlv++;
    }

    if (req->nfapi_config.rf_bands.tl.tag == NFAPI_NFAPI_RF_BANDS_TAG)
    {
        pnf->rfs[0].band = req->nfapi_config.rf_bands.rf_band[0];
        num_tlv++;
    }

    if (req->nfapi_config.earfcn.tl.tag == NFAPI_NFAPI_EARFCN_TAG)
    {
        num_tlv++;
    }

    if (req->subframe_config.duplex_mode.tl.tag == NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG)
    {
        num_tlv++;
    }

    if (req->subframe_config.dl_cyclic_prefix_type.tl.tag == NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG)
    {
        num_tlv++;
    }

    if (req->subframe_config.ul_cyclic_prefix_type.tl.tag == NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG)
    {
        num_tlv++;
    }

    if (req->sch_config.physical_cell_id.tl.tag == NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG)
    {
        num_tlv++;
    }

    if (req->rf_config.tx_antenna_ports.tl.tag == NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG)
    {
        num_tlv++;
    }

    if (req->rf_config.rx_antenna_ports.tl.tag == NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG)
    {
        num_tlv++;
    }

    if (req->phich_config.phich_resource.tl.tag == NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG)
    {
        num_tlv++;
    }

    if (req->phich_config.phich_duration.tl.tag == NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG)
    {
        num_tlv++;
    }

    if (req->phich_config.phich_power_offset.tl.tag == NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG)
    {
        num_tlv++;
    }

    // UL RS Config
    if (req->uplink_reference_signal_config.cyclic_shift_1_for_drms.tl.tag ==
        NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG)
    {
        num_tlv++;
    }

    if (req->uplink_reference_signal_config.uplink_rs_hopping.tl.tag ==
        NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG)
    {
        num_tlv++;
    }

    if (req->uplink_reference_signal_config.group_assignment.tl.tag ==
        NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG)
    {
        num_tlv++;
    }

    if (req->pusch_config.hopping_mode.tl.tag == NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG)
    {
    }  // DJP - not being handled?

    if (req->pusch_config.hopping_offset.tl.tag == NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG)
    {
    }  // DJP - not being handled?

    if (req->pusch_config.number_of_subbands.tl.tag == NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG)
    {
    }  // DJP - not being handled?

    if (req->prach_config.configuration_index.tl.tag == NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG)
    {
        num_tlv++;
    }

    if (req->prach_config.root_sequence_index.tl.tag == NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG)
    {
        num_tlv++;
    }

    if (req->prach_config.zero_correlation_zone_configuration.tl.tag ==
        NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG)
    {
        num_tlv++;
    }

    if (req->prach_config.high_speed_flag.tl.tag == NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG)
    {
        num_tlv++;
    }

    if (req->prach_config.frequency_offset.tl.tag == NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG)
    {
        num_tlv++;
    }

    printf("[PNF] CONFIG_REQUEST[num_tlv:%d] TLVs processed:%d\n", req->num_tlv, num_tlv); // make this an assert
    printf("[PNF] Simulating PHY CONFIG - DJP\n");

    epi_phy_info->remote_port = req->nfapi_config.p7_vnf_port.value;
    struct sockaddr_in vnf_p7_sockaddr;
    memcpy(&vnf_p7_sockaddr.sin_addr.s_addr, &(req->nfapi_config.p7_vnf_address_ipv4.address[0]), 4);
    epi_phy_info->remote_addr = inet_ntoa(vnf_p7_sockaddr.sin_addr);
    printf("[PNF] %d vnf p7 %s:%d timing %d %d %d\n", epi_phy_info->id, epi_phy_info->remote_addr,
           epi_phy_info->remote_port,
           epi_phy_info->timing_window, epi_phy_info->timing_info_mode, epi_phy_info->timing_info_period);
    nfapi_config_response_t nfapi_resp;
    memset(&nfapi_resp, 0, sizeof(nfapi_resp));
    nfapi_resp.header.message_id = NFAPI_CONFIG_RESPONSE;
    nfapi_resp.header.phy_id = epi_phy_info->id;
    nfapi_resp.error_code = 0; // DJP - some value resp->error_code;
    nfapi_pnf_config_resp(config, &nfapi_resp);
    printf("[PNF] Sent NFAPI_CONFIG_RESPONSE phy_id:%d\n", epi_phy_info->id);

    return 0;
}

int pnf_phy_hi_dci0_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_hi_dci0_request_t *req)
{

    (void)proc;
    (void)pnf_p7;
    (void)req;

    return 0;
}

int pnf_phy_dl_config_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_dl_config_request_t *req)
{

    (void)proc;
    (void)pnf_p7;
    (void)req;

    return 0;
}

int pnf_phy_tx_req(nfapi_pnf_p7_config_t *pnf_p7, nfapi_tx_request_t *req)
{

    (void)pnf_p7;
    uint16_t sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
    uint16_t sf = NFAPI_SFNSF2SF(req->sfn_sf);

    if (req->tx_request_body.number_of_pdus == 0)

        if (req->tx_request_body.tl.tag == NFAPI_TX_REQUEST_BODY_TAG)
        {
            for (int i = 0; i < req->tx_request_body.number_of_pdus; i++)
            {
                tx_request_pdu[sfn][sf][i] = &req->tx_request_body.tx_pdu_list[i];
            }
        }

    return 0;
}

int pnf_phy_ul_config_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_ul_config_request_t *req)
{

    (void)proc;
    (void)pnf_p7;


    nfapi_ul_config_request_pdu_t *ul_config_pdu_list = req->ul_config_request_body.ul_config_pdu_list;

    for (int i = 0; i < req->ul_config_request_body.number_of_pdus; i++)
    {
        if (
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_HARQ_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_RI_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_ULSCH_CQI_HARQ_RI_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_SR_PDU_TYPE ||
            ul_config_pdu_list[i].pdu_type == NFAPI_UL_CONFIG_UCI_SR_HARQ_PDU_TYPE
        )
        {
        }
        else
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() PDU:%i UNKNOWN type :%d\n", __FUNCTION__, i, ul_config_pdu_list[i].pdu_type);
        }
    }

    return 0;
}

int pnf_phy_lbt_dl_config_req(nfapi_pnf_p7_config_t *config, nfapi_lbt_dl_config_request_t *req)
{

    (void)config;
    (void)req;

    return 0;
}

int pnf_phy_ue_release_req(nfapi_pnf_p7_config_t *config, nfapi_ue_release_request_t *req)
{

    (void)config;

    if (req->ue_release_request_body.number_of_TLVs == 0)
    {
        return -1;
    }

    release_rntis.number_of_TLVs = req->ue_release_request_body.number_of_TLVs;
    memcpy(&release_rntis.ue_release_request_TLVs_list, req->ue_release_request_body.ue_release_request_TLVs_list,
           sizeof(nfapi_ue_release_request_TLVs_t)*req->ue_release_request_body.number_of_TLVs);

    return 0;
}


nfapi_dl_config_request_t dummy_dl_config_req;
nfapi_tx_request_t dummy_tx_req;
nfapi_pnf_p7_subframe_buffer_t dummy_subframe;

int start_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_start_request_t *req)
{
    printf("[PNF] Received NFAPI_START_REQ phy_id:%d\n", req->header.phy_id);
    nfapi_set_trace_level(NFAPI_TRACE_INFO);
    epi_pnf_info *pnf = (epi_pnf_info *)(config->user_data);
    epi_phy_info *epi_phy_info = pnf->phys;
    nfapi_pnf_p7_config_t *p7_config = nfapi_pnf_p7_config_create();
    p7_config->phy_id = phy->phy_id;
    p7_config->remote_p7_port = epi_phy_info->remote_port;
    p7_config->remote_p7_addr = epi_phy_info->remote_addr;
    p7_config->local_p7_port = 32123; // DJP - good grief cannot seem to get the right answer epi_phy_info->local_port;
    //DJP p7_config->local_p7_addr = (char*)epi_phy_info->local_addr.c_str();
    p7_config->local_p7_addr = epi_phy_info->local_addr;
    printf("[PNF] P7 remote:%s:%d local:%s:%d\n", p7_config->remote_p7_addr, p7_config->remote_p7_port,
           p7_config->local_p7_addr, p7_config->local_p7_port);
    p7_config->user_data = epi_phy_info;
    p7_config->malloc = &pnf_allocate;
    p7_config->free = &pnf_deallocate;
    p7_config->codec_config.allocate = &pnf_allocate;
    p7_config->codec_config.deallocate = &pnf_deallocate;
    p7_config->trace = &epi_pnf_nfapi_trace;
    p7_config->dl_config_req = NULL;
    p7_config->hi_dci0_req = NULL;
    p7_config->tx_req = NULL;
    p7_config->ul_config_req = NULL;
    phy->user_data = p7_config;
    p7_config->subframe_buffer_size = epi_phy_info->timing_window;
    printf("subframe_buffer_size configured using epi_phy_info->timing_window:%d\n", epi_phy_info->timing_window);

    if (epi_phy_info->timing_info_mode & 0x1)
    {
        p7_config->timing_info_mode_periodic = 1;
        p7_config->timing_info_period = epi_phy_info->timing_info_period;
    }

    if (epi_phy_info->timing_info_mode & 0x2)
    {
        p7_config->timing_info_mode_aperiodic = 1;
    }
    p7_config->lbt_dl_config_req = &pnf_phy_lbt_dl_config_req;
    memset(&dummy_dl_config_req, 0, sizeof(dummy_dl_config_req));
    dummy_dl_config_req.dl_config_request_body.tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
    dummy_dl_config_req.dl_config_request_body.number_pdcch_ofdm_symbols = 1;
    dummy_dl_config_req.dl_config_request_body.number_dci = 0;
    dummy_dl_config_req.dl_config_request_body.number_pdu = 0;
    dummy_dl_config_req.dl_config_request_body.number_pdsch_rnti = 0;
    dummy_dl_config_req.dl_config_request_body.transmission_power_pcfich = 6000;
    dummy_dl_config_req.dl_config_request_body.dl_config_pdu_list = 0;
    memset(&dummy_tx_req, 0, sizeof(dummy_tx_req));
    dummy_tx_req.tx_request_body.number_of_pdus = 0;
    dummy_tx_req.tx_request_body.tl.tag = NFAPI_TX_REQUEST_BODY_TAG;
    dummy_subframe.dl_config_req = &dummy_dl_config_req;
    dummy_subframe.tx_req = 0;
    dummy_subframe.ul_config_req = 0;
    dummy_subframe.hi_dci0_req = 0;
    dummy_subframe.lbt_dl_config_req = 0;
    p7_config->dummy_subframe = dummy_subframe;

    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Creating P7 thread %s\n", __FUNCTION__);
    pthread_t p7_thread;
    if (pthread_create(&p7_thread, NULL, &pnf_p7_thread_start, p7_config) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "[PNF] pthread_create: %s\n", strerror(errno));
    }
    if (pthread_setname_np(p7_thread, "PNF_P7") != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_setname_np: %s\n", strerror(errno));
    }

    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Calling l1_north_init_eNB() %s\n", __FUNCTION__);
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] DJP - HACK - Set p7_config global ready for subframe ind%s\n", __FUNCTION__);
    p7_config_g = p7_config;

    printf("[PNF] OAI eNB/RU configured\n");
    printf("[PNF] About to call init_eNB_afterRU()\n");

    printf("[PNF] Sending PNF_START_RESP\n");
    nfapi_send_pnf_start_resp(config, p7_config->phy_id);
    printf("[PNF] Sending first P7 subframe ind\n");
    nfapi_pnf_p7_subframe_ind(p7_config, p7_config->phy_id, 0); // DJP - SFN_SF set to zero - correct???
    printf("[PNF] Sent first P7 subframe ind\n");

    return 0;
}

int measurement_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_measurement_request_t *req)
{

    (void)phy;
    nfapi_measurement_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_MEASUREMENT_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_measurement_resp(config, &resp);
    return 0;

}

int rssi_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_rssi_request_t *req)
{

    (void)phy;
    nfapi_rssi_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_RSSI_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_rssi_resp(config, &resp);
    nfapi_rssi_indication_t ind;
    memset(&ind, 0, sizeof(ind));
    ind.header.message_id = NFAPI_RSSI_INDICATION;
    ind.header.phy_id = req->header.phy_id;
    ind.error_code = NFAPI_P4_MSG_OK;
    ind.rssi_indication_body.tl.tag = NFAPI_RSSI_INDICATION_TAG;
    ind.rssi_indication_body.number_of_rssi = 1;
    ind.rssi_indication_body.rssi[0] = -42;
    nfapi_pnf_rssi_ind(config, &ind);

    return 0;
}

int cell_search_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_cell_search_request_t *req)
{

    (void)phy;
    nfapi_cell_search_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_CELL_SEARCH_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_cell_search_resp(config, &resp);
    nfapi_cell_search_indication_t ind;
    memset(&ind, 0, sizeof(ind));
    ind.header.message_id = NFAPI_CELL_SEARCH_INDICATION;
    ind.header.phy_id = req->header.phy_id;
    ind.error_code = NFAPI_P4_MSG_OK;

    switch (req->rat_type)
    {
    case NFAPI_RAT_TYPE_LTE:
        ind.lte_cell_search_indication.tl.tag = NFAPI_LTE_CELL_SEARCH_INDICATION_TAG;
        ind.lte_cell_search_indication.number_of_lte_cells_found = 1;
        ind.lte_cell_search_indication.lte_found_cells[0].pci = 123;
        ind.lte_cell_search_indication.lte_found_cells[0].rsrp = 123;
        ind.lte_cell_search_indication.lte_found_cells[0].rsrq = 123;
        ind.lte_cell_search_indication.lte_found_cells[0].frequency_offset = 123;
        break;

    case NFAPI_RAT_TYPE_UTRAN:
    {
        ind.utran_cell_search_indication.tl.tag = NFAPI_UTRAN_CELL_SEARCH_INDICATION_TAG;
        ind.utran_cell_search_indication.number_of_utran_cells_found = 1;
        ind.utran_cell_search_indication.utran_found_cells[0].psc = 89;
        ind.utran_cell_search_indication.utran_found_cells[0].rscp = 89;
        ind.utran_cell_search_indication.utran_found_cells[0].ecno = 89;
        ind.utran_cell_search_indication.utran_found_cells[0].frequency_offset = -89;
    }
    break;

    case NFAPI_RAT_TYPE_GERAN:
    {
        ind.geran_cell_search_indication.tl.tag = NFAPI_GERAN_CELL_SEARCH_INDICATION_TAG;
        ind.geran_cell_search_indication.number_of_gsm_cells_found = 1;
        ind.geran_cell_search_indication.gsm_found_cells[0].bsic = 23;
        ind.geran_cell_search_indication.gsm_found_cells[0].rxlev = 23;
        ind.geran_cell_search_indication.gsm_found_cells[0].rxqual = 23;
        ind.geran_cell_search_indication.gsm_found_cells[0].frequency_offset = -23;
        ind.geran_cell_search_indication.gsm_found_cells[0].sfn_offset = 230;
    }
    break;
    }

    ind.pnf_cell_search_state.tl.tag = NFAPI_PNF_CELL_SEARCH_STATE_TAG;
    ind.pnf_cell_search_state.length = 3;
    nfapi_pnf_cell_search_ind(config, &ind);

    return 0;
}

int broadcast_detect_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy,
                             nfapi_broadcast_detect_request_t *req)
{

    (void)phy;
    nfapi_broadcast_detect_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_BROADCAST_DETECT_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_broadcast_detect_resp(config, &resp);
    nfapi_broadcast_detect_indication_t ind;
    memset(&ind, 0, sizeof(ind));
    ind.header.message_id = NFAPI_BROADCAST_DETECT_INDICATION;
    ind.header.phy_id = req->header.phy_id;
    ind.error_code = NFAPI_P4_MSG_OK;

    switch (req->rat_type)
    {
    case NFAPI_RAT_TYPE_LTE:
    {
        ind.lte_broadcast_detect_indication.tl.tag = NFAPI_LTE_BROADCAST_DETECT_INDICATION_TAG;
        ind.lte_broadcast_detect_indication.number_of_tx_antenna = 1;
        ind.lte_broadcast_detect_indication.mib_length = 4;
        ind.lte_broadcast_detect_indication.sfn_offset = 77;
    }
    break;

    case NFAPI_RAT_TYPE_UTRAN:
    {
        ind.utran_broadcast_detect_indication.tl.tag = NFAPI_UTRAN_BROADCAST_DETECT_INDICATION_TAG;
        ind.utran_broadcast_detect_indication.mib_length = 4;
    }
    break;
    }

    ind.pnf_cell_broadcast_state.tl.tag = NFAPI_PNF_CELL_BROADCAST_STATE_TAG;
    ind.pnf_cell_broadcast_state.length = 3;
    nfapi_pnf_broadcast_detect_ind(config, &ind);

    return 0;
}

int system_information_schedule_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy,
                                        nfapi_system_information_schedule_request_t *req)
{

    (void)phy;
    nfapi_system_information_schedule_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_system_information_schedule_resp(config, &resp);
    nfapi_system_information_schedule_indication_t ind;
    memset(&ind, 0, sizeof(ind));
    ind.header.message_id = NFAPI_SYSTEM_INFORMATION_SCHEDULE_INDICATION;
    ind.header.phy_id = req->header.phy_id;
    ind.error_code = NFAPI_P4_MSG_OK;
    ind.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
    ind.lte_system_information_indication.sib_type = 3;
    ind.lte_system_information_indication.sib_length = 5;
    nfapi_pnf_system_information_schedule_ind(config, &ind);

    return 0;
}

int system_information_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy,
                               nfapi_system_information_request_t *req)
{

    (void)phy;
    nfapi_system_information_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_SYSTEM_INFORMATION_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_system_information_resp(config, &resp);
    nfapi_system_information_indication_t ind;
    memset(&ind, 0, sizeof(ind));
    ind.header.message_id = NFAPI_SYSTEM_INFORMATION_INDICATION;
    ind.header.phy_id = req->header.phy_id;
    ind.error_code = NFAPI_P4_MSG_OK;

    switch (req->rat_type)
    {
    case NFAPI_RAT_TYPE_LTE:
    {
        ind.lte_system_information_indication.tl.tag = NFAPI_LTE_SYSTEM_INFORMATION_INDICATION_TAG;
        ind.lte_system_information_indication.sib_type = 1;
        ind.lte_system_information_indication.sib_length = 3;
    }
    break;

    case NFAPI_RAT_TYPE_UTRAN:
    {
        ind.utran_system_information_indication.tl.tag = NFAPI_UTRAN_SYSTEM_INFORMATION_INDICATION_TAG;
        ind.utran_system_information_indication.sib_length = 3;
    }
    break;

    case NFAPI_RAT_TYPE_GERAN:
    {
        ind.geran_system_information_indication.tl.tag = NFAPI_GERAN_SYSTEM_INFORMATION_INDICATION_TAG;
        ind.geran_system_information_indication.si_length = 3;
    }
    break;
    }

    nfapi_pnf_system_information_ind(config, &ind);
    return 0;
}

int nmm_stop_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_nmm_stop_request_t *req)
{

    (void)phy;
    nfapi_nmm_stop_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_NMM_STOP_RESPONSE;
    resp.header.phy_id = req->header.phy_id;
    resp.error_code = NFAPI_P4_MSG_OK;
    nfapi_pnf_nmm_stop_resp(config, &resp);

    return 0;
}

void *pnf_start_thread(void *ptr)
{

    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] IN PNF NFAPI start thread %s\n", __FUNCTION__);
    nfapi_pnf_config_t *config = (nfapi_pnf_config_t *)ptr;

    struct sched_param sp =
    {
        .sched_priority = 79,
    };
    pthread_setschedparam(pthread_self(), SCHED_RR, &sp);

    nfapi_pnf_start(config);

    return NULL;
}

void configure_nfapi_pnf(const char *vnf_ip_addr, int vnf_p5_port, const char *pnf_ip_addr, int pnf_p7_port,
                         int vnf_p7_port)
{

    printf("%s() PNF\n\n\n\n\n\n", __FUNCTION__);

    nfapi_pnf_config_t *config = nfapi_pnf_config_create();
    config->vnf_ip_addr = vnf_ip_addr;
    config->vnf_p5_port = vnf_p5_port;
    pnf.phys[0].udp.enabled = 1;
    pnf.phys[0].udp.rx_port = pnf_p7_port;
    pnf.phys[0].udp.tx_port = vnf_p7_port;
    strcpy(pnf.phys[0].udp.tx_addr, vnf_ip_addr);
    strcpy(pnf.phys[0].local_addr, pnf_ip_addr);
    printf("%s() VNF:%s:%d PNF_PHY[addr:%s UDP:tx_addr:%s:%d rx:%d]\n",
           __FUNCTION__,
           config->vnf_ip_addr, config->vnf_p5_port,
           pnf.phys[0].local_addr,
           pnf.phys[0].udp.tx_addr, pnf.phys[0].udp.tx_port,
           pnf.phys[0].udp.rx_port);
    config->pnf_param_req = &pnf_param_request;
    config->pnf_config_req = &pnf_config_request;
    config->pnf_start_req = &pnf_start_request;
    config->pnf_stop_req = &pnf_stop_request;
    config->param_req = &param_request;
    config->config_req = &config_request;
    config->start_req = &start_request;
    config->measurement_req = &measurement_request;
    config->rssi_req = &rssi_request;
    config->cell_search_req = &cell_search_request;
    config->broadcast_detect_req = &broadcast_detect_request;
    config->system_information_schedule_req = &system_information_schedule_request;
    config->system_information_req = &system_information_request;
    config->nmm_stop_req = &nmm_stop_request;
    config->trace = &epi_pnf_nfapi_trace;
    config->user_data = &pnf;
    // To allow custom vendor extentions to be added to nfapi
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Creating PNF NFAPI start thread %s\n", __FUNCTION__);
    if (pthread_create(&pnf_start_pthread, NULL, &pnf_start_thread, config) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create: %s\n", strerror(errno));
    }
    if (pthread_setname_np(pnf_start_pthread, "NFAPI_PNF") != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_setname_np: %s\n", strerror(errno));
    }
}

void oai_subframe_ind(uint16_t sfn, uint16_t sf)
{
    //TODO FIXME - HACK - DJP - using a global to bodge it in
    if (p7_config_g != NULL)
    {
        uint16_t sfn_sf_tx = sfn << 4 | sf;

        if ((sfn % 100 == 0) && sf == 0)
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] %s %d.%d (sfn:%u sf:%u) SFN/SF(TX):%u\n", __FUNCTION__, ts.tv_sec, ts.tv_nsec, sfn,
                        sf, NFAPI_SFNSF2DEC(sfn_sf_tx));
        }

        int subframe_ret = nfapi_pnf_p7_subframe_ind(p7_config_g, p7_config_g->phy_id, sfn_sf_tx);

        if (subframe_ret)
        {
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] %s(frame:%u subframe:%u) SFN/SF(TX):%u - PROBLEM with pnf_p7_subframe_ind()\n",
                        __FUNCTION__, sfn, sf, sfn_sf_tx, NFAPI_SFNSF2DEC(sfn_sf_tx));
        }
        else
        {

        }
    }
    else
    {
    }
}

// Queues of message_buffer_t pointers, one queue per UE
static queue_t msgs_from_ue[MAX_UES];

void oai_subframe_init()
{
    for (int i = 0; i < num_ues; i++)
    {
        init_queue(&msgs_from_ue[i]);
    }
}

static uint16_t get_message_id(message_buffer_t *msg)
{
    uint16_t phy_id = ~0;
    uint16_t message_id = ~0;
    uint8_t *in = msg->data;
    uint8_t *end = msg->data + msg->length;
    pull16(&in, &phy_id, end);
    pull16(&in, &message_id, end);
    return message_id;
}

static bool get_sfnsf(message_buffer_t *msg, uint16_t *sfnsf)
{
    uint8_t *in = msg->data;
    uint8_t *end = msg->data + msg->length;

    uint16_t phy_id;
    uint16_t message_id;
    if (!pull16(&in, &phy_id, end) ||
        !pull16(&in, &message_id, end))
    {
        nfapi_error("could not retrieve message_id");
        return false;
    }

    switch (message_id)
    {
    case NFAPI_RACH_INDICATION:
    case NFAPI_CRC_INDICATION:
    case NFAPI_RX_ULSCH_INDICATION:
    case NFAPI_RX_CQI_INDICATION:
    case NFAPI_HARQ_INDICATION:
    case NFAPI_RX_SR_INDICATION:
        break;
    default:
        nfapi_error("Message_id is unknown %u", message_id);
        return false;
    }

    in = msg->data + sizeof(nfapi_p7_message_header_t);
    if (!pull16(&in, sfnsf, end))
    {
        nfapi_error("could not retrieve sfn_sf");
        return false;
    }
    return true;
}

static void warn_if_different(const char *label,
                              const nfapi_p7_message_header_t *agg_header,
                              const nfapi_p7_message_header_t *ind_header,
                              uint16_t agg_sfn_sf,
                              uint16_t ind_sfn_sf)
{
    if (agg_header->phy_id != ind_header->phy_id ||
        agg_header->message_id != ind_header->message_id ||
        agg_header->m_segment_sequence != ind_header->m_segment_sequence ||
        agg_sfn_sf != ind_sfn_sf)
    {
        nfapi_logfile(label, "WARN", "mismatch phy_id %u,%u message_id %u,%u m_seg_sequence %u,%u sfn_sf %u,%u",
                      agg_header->phy_id, ind_header->phy_id,
                      agg_header->message_id, ind_header->message_id,
                      agg_header->m_segment_sequence, ind_header->m_segment_sequence,
                      agg_sfn_sf, ind_sfn_sf);
    }
}


/*
  nfapi_rach_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_rach_indication_body_t rach_indication_body
        nfapi_tl_t tl
            uint16_t tag
            uint16_t length
        uint16_t number_of_preambles
        nfapi_preamble_pdu_t *preamble_list
            uint16_t instance_length
            nfapi_preamble_pdu_rel8_t preamble_rel8
                nfapi_tl_t tl
                uint16_t rnti
                uint8_t preamble
                uint16_t timing_advance
            nfapi_preamble_pdu_rel9_t preamble_rel9
                nfapi_tl_t tl
                uint16_t timing_advance_r9
            nfapi_preamble_pdu_rel13_t preamble_rel13
                nfapi_tl_t tl
                uint8_t rach_resource_type
    nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

static void oai_subframe_aggregate_rach_ind(subframe_msgs_t *msgs)
{
    /* Currently we select the first RACH and ignore others.
       We could have various policies to select one RACH over
       the others (e.g., strongest signal) */
    assert(msgs->num_msgs > 0);
    for (int i = 0; i < msgs->num_msgs; ++i)
    {
        message_buffer_t *msg = msgs->msgs[i];
        nfapi_rach_indication_t ind;

        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("rach unpack failed");
            continue;
        }
        ind.sfn_sf = sfn_sf_add(ind.sfn_sf, 5);
        oai_nfapi_rach_ind(&ind);
    }
}


/*
  nfapi_crc_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_crc_indication_body_t crc_indication_body
        nfapi_tl_t tl
            uint16_t tag
            uint16_t length
        uint16_t number_of_crcs
        nfapi_crc_indication_pdu_t* crc_pdu_list
            uint16_t instance_length
            nfapi_rx_ue_information rx_ue_information
            nfapi_tl_t tl
            uint32_t handle
            uint16_t rnti
            nfapi_crc_indication_rel8_t crc_indication_rel8
            nfapi_tl_t tl
            uint8_t crc_flag
        nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

static void oai_subframe_aggregate_crc_ind(subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_crc_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.crc_indication_body.crc_pdu_list = calloc(NFAPI_CRC_IND_MAX_PDU, sizeof(nfapi_crc_indication_pdu_t));
    assert(agg.crc_indication_body.crc_pdu_list);

    size_t pduIndex = 0;
    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_crc_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("crc indication unpack failed, msg[%zu]", n);
            free(agg.crc_indication_body.crc_pdu_list);
            return;
        }

        // Header, sfn_sf, and tl assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different(__func__, &agg.header, &ind.header, agg.sfn_sf, ind.sfn_sf);
        }
        agg.header = ind.header;
        agg.sfn_sf = ind.sfn_sf;
        assert(!ind.vendor_extension); // TODO ignoring vendor_extension for now
        agg.crc_indication_body.tl = ind.crc_indication_body.tl;
        agg.crc_indication_body.number_of_crcs += ind.crc_indication_body.number_of_crcs;

        for (size_t i = 0; i < ind.crc_indication_body.number_of_crcs; ++i)
        {
            if (pduIndex == NFAPI_CRC_IND_MAX_PDU)
            {
                nfapi_error("Too many PDUs to aggregate");
                break;
            }
            agg.crc_indication_body.crc_pdu_list[pduIndex] = ind.crc_indication_body.crc_pdu_list[i];
            pduIndex++;
        }
        free(ind.crc_indication_body.crc_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_CRC_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_crc_indication(&agg);
    free(agg.crc_indication_body.crc_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
}

/*
  nfapi_rx_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_rx_indication_body_t rx_indication_body
        nfapi_tl_t tl
            uint16_t tag
            uint16_t length
        uint16_t number_of_pdus
        nfapi_rx_indication_pdu_t* rx_pdu_list
            nfapi_rx_ue_information rx_ue_information
                nfapi_tl_t tl
                uint32_t handle
                uint16_t rnti
            nfapi_rx_indication_rel8_t rx_indication_rel8
                nfapi_tl_t tl
                uint16_t length
                uint16_t offset
                uint8_t ul_cqi
                uint16_t timing_advance
            nfapi_rx_indication_rel9_t rx_indication_rel9
                nfapi_tl_t tl
                uint16_t timing_advance_r9
            // TODO What is rx_ind_data? Could be NULL.
            // If not NULL, then nfapi_p7.c code seems to leak this memory.
            // We'll ignore it for now.
            // (Was named `data`. Renamed to make it easier to track down)
            uint8_t* rx_ind_data
        nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

// static void print_rx_ind(nfapi_rx_indication_t *p)
// {
//   nfapi_info("Printing RX_IND fields");
//   nfapi_info("header.message_id: %u", p->header.message_id);
//   nfapi_info("header.phy_id: %u", p->header.phy_id);
//   nfapi_info("header.message_id: %u", p->header.message_id);
//   nfapi_info("header.m_segment_sequence: %u", p->header.m_segment_sequence);
//   nfapi_info("header.checksum: %u", p->header.checksum);
//   nfapi_info("header.transmit_timestamp: %u", p->header.transmit_timestamp);
//   nfapi_info("sfn_sf: %u", p->sfn_sf);
//   nfapi_info("rx_indication_body.tl.tag: 0x%x", p->rx_indication_body.tl.tag);
//   nfapi_info("rx_indication_body.tl.length: %u", p->rx_indication_body.tl.length);
//   nfapi_info("rx_indication_body.number_of_pdus: %u", p->rx_indication_body.number_of_pdus);

//   nfapi_rx_indication_pdu_t *pdu = p->rx_indication_body.rx_pdu_list;
//   for (int i = 0; i < p->rx_indication_body.number_of_pdus; i++)
//   {
//     nfapi_info("pdu %d nfapi_rx_ue_information.tl.tag: 0x%x", i, pdu->rx_ue_information.tl.tag);
//     nfapi_info("pdu %d nfapi_rx_ue_information.tl.length: %u", i, pdu->rx_ue_information.tl.length);
//     nfapi_info("pdu %d nfapi_rx_ue_information.handle: %u", i, pdu->rx_ue_information.handle);
//     nfapi_info("pdu %d nfapi_rx_ue_information.rnti: %u", i, pdu->rx_ue_information.rnti);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.tl.tag: 0x%x", i, pdu->rx_indication_rel8.tl.tag);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.tl.length: %u", i, pdu->rx_indication_rel8.tl.length);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.length: %u", i, pdu->rx_indication_rel8.length);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.offset: %u", i, pdu->rx_indication_rel8.offset);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.ul_cqi: %u", i, pdu->rx_indication_rel8.ul_cqi);
//     nfapi_info("pdu %d nfapi_rx_indication_rel8.timing_advance: %u", i, pdu->rx_indication_rel8.timing_advance);
//     nfapi_info("pdu %d nfapi_rx_indication_rel9.tl.tag: 0x%x", i, pdu->rx_indication_rel9.tl.tag);
//     nfapi_info("pdu %d nfapi_rx_indication_rel9.tl.length: %u", i, pdu->rx_indication_rel9.tl.length);
//     nfapi_info("pdu %d nfapi_rx_indication_rel9.timing_advance_r9: %u", i, pdu->rx_indication_rel9.timing_advance_r9);
//   }
// }


static void oai_subframe_aggregate_rx_ind(subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_rx_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.rx_indication_body.rx_pdu_list = calloc(NFAPI_RX_IND_MAX_PDU, sizeof(nfapi_rx_indication_pdu_t));
    assert(agg.rx_indication_body.rx_pdu_list);

    size_t pduIndex = 0;

    if (msgs->num_msgs > 1)
    {
        uint16_t sfn_sf_val;
        message_buffer_t *first_msg = msgs->msgs[0];
        if (!get_sfnsf(first_msg, &sfn_sf_val))
        {
            nfapi_error("Something went very wrong");
            abort();
        }
        nfapi_info("We are aggregating %zu rx_ind's for SFN.SF %u.%u "
                   "trying to aggregate",
                   msgs->num_msgs, sfn_sf_val >> 4,
                   sfn_sf_val & 15);
    }

    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_rx_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("rx indication unpack failed, msg[%zu]", n);
            free(agg.rx_indication_body.rx_pdu_list);
            return;
        }

        if (ind.rx_indication_body.number_of_pdus == 0)
        {
            nfapi_error("empty rx message");
            abort();
        }

        int rnti = ind.rx_indication_body.rx_pdu_list[0].rx_ue_information.rnti;
        bool found = false;
        for (int i = 1; i < agg.rx_indication_body.number_of_pdus; ++i)
        {
            int rnti_i = agg.rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti;
            if (rnti == rnti_i)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            nfapi_error("two rx for single UE rnti: %x", rnti);
        }

        // Header, sfn_sf, and tl assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different(__func__, &agg.header, &ind.header, agg.sfn_sf, ind.sfn_sf);
        }
        agg.header = ind.header;
        agg.sfn_sf = ind.sfn_sf;
        assert(!ind.vendor_extension); // TODO ignoring vendor_extension for now
        agg.rx_indication_body.tl = ind.rx_indication_body.tl;
        nfapi_info("agg.tl.tag: %x agg.tl.length: %d ind.tl.tag: %x ind.tl.length: %u",
                agg.rx_indication_body.tl.tag,agg.rx_indication_body.tl.length,
                ind.rx_indication_body.tl.tag, ind.rx_indication_body.tl.length);
        agg.rx_indication_body.number_of_pdus += ind.rx_indication_body.number_of_pdus;

        for (size_t i = 0; i < ind.rx_indication_body.number_of_pdus; ++i)
        {
            if (pduIndex == NFAPI_RX_IND_MAX_PDU)
            {
                nfapi_error("Too many PDUs to aggregate");
                break;
            }
            agg.rx_indication_body.rx_pdu_list[pduIndex] = ind.rx_indication_body.rx_pdu_list[i];
            pduIndex++;
        }
        free(ind.rx_indication_body.rx_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_RX_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_rx_ind(&agg);
    free(agg.rx_indication_body.rx_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
}

/*
  nfapi_cqi_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_cqi_indication_body_t cqi_indication_body
        nfapi_tl_t tl
        uint16_t number_of_cqis
        nfapi_cqi_indication_pdu_t *cqi_pdu_list
            uint16_t instance_length
            nfapi_rx_ue_information rx_ue_information
                nfapi_tl_t tl
                uint32_t handle
                uint16_t rnti
            nfapi_cqi_indication_rel8_t cqi_indication_rel8
                nfapi_tl_t tl
                uint16_t length
                uint16_t data_offset
                uint8_t ul_cqi
                uint8_t ri
                uint16_t timing_advance
            nfapi_cqi_indication_rel9_t cqi_indication_rel9
                nfapi_tl_t tl
                uint16_t length
                uint16_t data_offset
                uint8_t ul_cqi
                uint8_t number_of_cc_reported
                uint8_t ri[NFAPI_CC_MAX]
                uint16_t timing_advance
                uint16_t timing_advance_r9
            nfapi_ul_cqi_information_t ul_cqi_information;
                nfapi_tl_t tl
                uint8_t ul_cqi
                uint8_t channel
        nfapi_cqi_indication_raw_pdu_t *cqi_raw_pdu_list // number_of_cqis
            uint8_t pdu[NFAPI_CQI_RAW_MAX_LEN]
    nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

static void oai_subframe_aggregate_cqi_ind(subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_cqi_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.cqi_indication_body.cqi_pdu_list = calloc(NFAPI_CQI_IND_MAX_PDU, sizeof(nfapi_cqi_indication_pdu_t));
    assert(agg.cqi_indication_body.cqi_pdu_list);
    agg.cqi_indication_body.cqi_raw_pdu_list = calloc(NFAPI_CQI_IND_MAX_PDU, sizeof(nfapi_cqi_indication_raw_pdu_t));
    assert(agg.cqi_indication_body.cqi_raw_pdu_list);
    size_t pduIndex = 0;

    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_cqi_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("cqi indication unpack failed, msg[%zu]", n);
            free(agg.cqi_indication_body.cqi_pdu_list);
            free(agg.cqi_indication_body.cqi_raw_pdu_list);
            return;
        }

        if (ind.cqi_indication_body.number_of_cqis == 0)
        {
            nfapi_error("empty cqi message");
            abort();
        }

        int rnti = ind.cqi_indication_body.cqi_pdu_list[0].rx_ue_information.rnti;
        bool found = false;
        for (int i = 1; i < agg.cqi_indication_body.number_of_cqis; ++i)
        {
            int rnti_i = agg.cqi_indication_body.cqi_pdu_list[i].rx_ue_information.rnti;
            if (rnti == rnti_i)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            nfapi_error("two cqis for single UE rnti: %x", rnti);
        }

        // Header, sfn_sf, and tl assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different(__func__, &agg.header, &ind.header, agg.sfn_sf, ind.sfn_sf);
        }
        agg.header = ind.header;
        agg.sfn_sf = ind.sfn_sf;
        assert(!ind.vendor_extension); // TODO ignoring vendor_extension for now
        agg.cqi_indication_body.tl = ind.cqi_indication_body.tl;
        agg.cqi_indication_body.number_of_cqis += ind.cqi_indication_body.number_of_cqis;

        for (size_t i = 0; i < ind.cqi_indication_body.number_of_cqis; ++i)
        {
            if (pduIndex == NFAPI_CQI_IND_MAX_PDU)
            {
                nfapi_error("Too many PDUs to aggregate");
                break;
            }

            agg.cqi_indication_body.cqi_pdu_list[pduIndex] = ind.cqi_indication_body.cqi_pdu_list[i];
            agg.cqi_indication_body.cqi_raw_pdu_list[pduIndex] = ind.cqi_indication_body.cqi_raw_pdu_list[i];
            pduIndex++;
        }
        free(ind.cqi_indication_body.cqi_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        free(ind.cqi_indication_body.cqi_raw_pdu_list);
        if (pduIndex == NFAPI_CQI_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_cqi_indication(&agg);
    free(agg.cqi_indication_body.cqi_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
    free(agg.cqi_indication_body.cqi_raw_pdu_list);
}

/*
  nfapi_harq_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_harq_indication_body_t harq_indication_body
        nfapi_tl_t tl
        uint16_t number_of_harqs
        nfapi_harq_indication_pdu_t *harq_pdu_list
            uint16_t instance_length
            nfapi_rx_ue_information rx_ue_information
                nfapi_tl_t tl
                uint32_t handle
                uint16_t rnti
            nfapi_harq_indication_tdd_rel8_t harq_indication_tdd_rel8
                nfapi_tl_t tl
                uint8_t mode
                uint8_t number_of_ack_nack
                union {
                    nfapi_harq_indication_tdd_harq_data_bundling_t bundling
                        uint8_t value_0
                        uint8_t value_1
                    nfapi_harq_indication_tdd_harq_data_multiplexing_t multiplex
                        uint8_t value_0
                        uint8_t value_1
                        uint8_t value_2
                        uint8_t value_3
                    nfapi_harq_indication_tdd_harq_data_special_bundling_t special_bundling
                        uint8_t value_0
                } harq_data
            nfapi_harq_indication_tdd_rel9_t harq_indication_tdd_rel9
                nfapi_tl_t tl
                uint8_t mode
                uint8_t number_of_ack_nack
                union {
                    nfapi_harq_indication_tdd_harq_data_t bundling
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t multiplex
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_special_bundling_t special_bundling
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t channel_selection
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t format_3
                        uint8_t value_0
                } harq_data[NFAPI_MAX_NUMBER_ACK_NACK_TDD];
            nfapi_harq_indication_tdd_rel13_t harq_indication_tdd_rel13
                nfapi_tl_t tl;
                uint8_t mode;
                uint16_t number_of_ack_nack;
                union {
                    nfapi_harq_indication_tdd_harq_data_t bundling
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t multiplex
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_special_bundling_t special_bundling
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t channel_selection
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t format_3
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t format_4
                        uint8_t value_0
                    nfapi_harq_indication_tdd_harq_data_t format_5
                        uint8_t value_0
                } harq_data[NFAPI_MAX_NUMBER_ACK_NACK_TDD];
            nfapi_harq_indication_fdd_rel8_t harq_indication_fdd_rel8
                nfapi_tl_t tl
                uint8_t harq_tb1
                uint8_t harq_tb2
            nfapi_harq_indication_fdd_rel9_t harq_indication_fdd_rel9
                nfapi_tl_t tl
                uint8_t mode
                uint8_t number_of_ack_nack
                uint8_t harq_tb_n[NFAPI_HARQ_ACK_NACK_REL9_MAX]
            nfapi_harq_indication_fdd_rel13_t harq_indication_fdd_rel13
                nfapi_tl_t tl
                uint8_t mode
                uint16_t number_of_ack_nack
                uint8_t harq_tb_n[NFAPI_HARQ_ACK_NACK_REL13_MAX]
        nfapi_ul_cqi_information_t ul_cqi_information
            nfapi_tl_t tl
            uint8_t ul_cqi
            uint8_t channel
    nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

static void oai_subframe_aggregate_harq_ind(subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_harq_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.harq_indication_body.harq_pdu_list = calloc(NFAPI_HARQ_IND_MAX_PDU, sizeof(nfapi_harq_indication_pdu_t));
    assert(agg.harq_indication_body.harq_pdu_list);

    size_t pduIndex = 0;
    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_harq_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("harq indication unpack failed, msg[%zu]", n);
            free(agg.harq_indication_body.harq_pdu_list);
            return;
        }

        // Header, sfn_sf, and tl assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different(__func__, &agg.header, &ind.header, agg.sfn_sf, ind.sfn_sf);
        }
        agg.header = ind.header;
        agg.sfn_sf = ind.sfn_sf;
        assert(!ind.vendor_extension); // TODO ignoring vendor_extension for now
        agg.harq_indication_body.tl = ind.harq_indication_body.tl;
        agg.harq_indication_body.number_of_harqs += ind.harq_indication_body.number_of_harqs;

        for (size_t i = 0; i < ind.harq_indication_body.number_of_harqs; ++i)
        {
            if (pduIndex == NFAPI_HARQ_IND_MAX_PDU)
            {
                nfapi_error("Too many PDUs to aggregate");
                break;
            }
            agg.harq_indication_body.harq_pdu_list[pduIndex] = ind.harq_indication_body.harq_pdu_list[i];
            pduIndex++;
        }
        free(ind.harq_indication_body.harq_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_HARQ_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_harq_indication(&agg);
    free(agg.harq_indication_body.harq_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
}

/*
  nfapi_sr_indication_t
    nfapi_p7_message_header_t header
        uint16_t phy_id
        uint16_t message_id
        uint16_t message_length
        uint16_t m_segment_sequence // 3 fields
        uint32_t checksum
        uint32_t transmit_timestamp
    uint16_t sfn_sf
    nfapi_sr_indication_body_t sr_indication_body
        nfapi_tl_t tl
        uint16_t number_of_srs
        nfapi_sr_indication_pdu_t *sr_pdu_list
            uint16_t instance_length
            nfapi_rx_ue_information rx_ue_information
                nfapi_tl_t tl
                uint32_t handle
                uint16_t rnti
            nfapi_ul_cqi_information_t ul_cqi_information
                nfapi_tl_t tl
                uint8_t ul_cqi
                uint8_t channel
    nfapi_vendor_extension_tlv_t vendor_extension (typedef nfapi_tl_t*)
*/

static void oai_subframe_aggregate_sr_ind(subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_sr_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.sr_indication_body.sr_pdu_list = calloc(NFAPI_SR_IND_MAX_PDU, sizeof(nfapi_sr_indication_pdu_t));
    assert(agg.sr_indication_body.sr_pdu_list);

    size_t pduIndex = 0;
    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_sr_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            nfapi_error("sr indication unpack failed, msg[%zu]", n);
            free(agg.sr_indication_body.sr_pdu_list);
            return;
        }

        // Header, sfn_sf, and tl assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different(__func__, &agg.header, &ind.header, agg.sfn_sf, ind.sfn_sf);
        }
        agg.header = ind.header;
        agg.sfn_sf = ind.sfn_sf;
        assert(!ind.vendor_extension); // TODO ignoring vendor_extension for now
        agg.sr_indication_body.tl = ind.sr_indication_body.tl;
        agg.sr_indication_body.number_of_srs += ind.sr_indication_body.number_of_srs;

        for (size_t i = 0; i < ind.sr_indication_body.number_of_srs; ++i)
        {
            if (pduIndex == NFAPI_SR_IND_MAX_PDU)
            {
                nfapi_error("Too many PDUs to aggregate");
                break;
            }
            agg.sr_indication_body.sr_pdu_list[pduIndex] = ind.sr_indication_body.sr_pdu_list[i];
            pduIndex++;
        }
        free(ind.sr_indication_body.sr_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_SR_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_sr_indication(&agg);
    free(agg.sr_indication_body.sr_pdu_list); // Should actually be nfapi_vnf_p7_release_msg
}

static void oai_subframe_aggregate_message_id(uint16_t msg_id, subframe_msgs_t *msgs)
{
    uint16_t sfn_sf = nfapi_get_sfnsf(msgs->msgs[0]->data, msgs->msgs[0]->length);
    nfapi_info("(EMANE eNB) Aggregating collection of %s uplink messages prior to sending to eNB. Frame: %d, Subframe: %d",
                nfapi_get_message_id(msgs->msgs[0]->data, msgs->msgs[0]->length), NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));
    // Aggregate these messages and send the resulting message to the eNB
    switch (msg_id)
    {
    case NFAPI_RACH_INDICATION:
        oai_subframe_aggregate_rach_ind(msgs);
        break;
    case NFAPI_CRC_INDICATION:
        oai_subframe_aggregate_crc_ind(msgs);
        break;
    case NFAPI_RX_ULSCH_INDICATION:
        oai_subframe_aggregate_rx_ind(msgs);
        break;
    case NFAPI_RX_CQI_INDICATION:
        oai_subframe_aggregate_cqi_ind(msgs);
        break;
    case NFAPI_HARQ_INDICATION:
        oai_subframe_aggregate_harq_ind(msgs);
        break;
    case NFAPI_RX_SR_INDICATION:
        oai_subframe_aggregate_sr_ind(msgs);
        break;
    default:
        return;
    }
}

/*
    subframe_msgs is a pointer to an array of num_ues
*/
static void oai_subframe_aggregate_messages(subframe_msgs_t *subframe_msgs)
{
    for (;;)
    {
        subframe_msgs_t msgs_by_id;
        memset(&msgs_by_id, 0, sizeof(msgs_by_id));
        uint16_t found_msg_id = 0;
        for (int i = 0; i < num_ues; i++)
        {
            for (int j = 0; j < subframe_msgs[i].num_msgs; j++)
            {
                message_buffer_t *msg = subframe_msgs[i].msgs[j];
                if (!msg)
                {
                    // already processed this message
                    continue;
                }
                assert(msg->magic == MESSAGE_BUFFER_MAGIC);

                uint16_t id = get_message_id(msg);
                if (found_msg_id == 0)
                {
                    found_msg_id = id;
                }
                if (found_msg_id == id)
                {
                    subframe_msgs[i].msgs[j] = NULL;
                    if (msgs_by_id.num_msgs == MAX_SUBFRAME_MSGS)
                    {
                        nfapi_error("Too many msgs");
                        msg->magic = 0;
                        free(msg);
                        continue;
                    }
                    msgs_by_id.msgs[msgs_by_id.num_msgs++] = msg;
                }
            }
        }
        if (found_msg_id == 0)
        {
            break;
        }
        oai_subframe_aggregate_message_id(found_msg_id, &msgs_by_id);

        for (int k = 0; k < msgs_by_id.num_msgs; k++)
        {
            message_buffer_t *msg = msgs_by_id.msgs[k];
            assert(msg->magic == MESSAGE_BUFFER_MAGIC);
            msg->magic = 0;
            free(msg);
        }
    }
}

int get_sf_delta(uint16_t a, uint16_t b)
{
    int sfn_a = a >> 4;
    int sf_a = a & 15;
    int sfn_b = b >> 4;
    int sf_b = b & 15;

    return (sfn_a * 10 + sf_a) - (sfn_b * 10 + sf_b);
}

uint16_t sfn_sf_add(uint16_t a, uint16_t add_val)
{
    uint16_t sfn_a = a >> 4;
    uint16_t sf_a = a & 15;
    uint16_t temp = sfn_a * 10 + sf_a + add_val;
    uint16_t sf = temp % 10;
    uint16_t sfn = temp / 10;
    uint16_t sfn_sf = (sfn << 4) | sf;

    return sfn_sf;
}

// TODO: Expand this function to include checking what the different sfn_sf
// mismatchs are
// static void log_num_sfn_mismatches(subframe_msgs_t *subframe_msgs)
// {
//     bool is_mismatch = false;
//     int total_mismatches = 0;
//     // analyze dequed messages from each ue and see if they have sfn_sf mismatchs
//     for (int i = 0; i < num_ues; ++i)
//     {
//         int msg_mismatch_count = 0;

//         for (int j = 0; j < subframe_msgs[i].num_msgs; ++j)
//         {
//             message_buffer_t *msg_j = subframe_msgs[i].msgs[j];
//             uint16_t sfn_sf_j;
//             if (!get_sfnsf(msg_j, &sfn_sf_j))
//             {
//                 nfapi_error("Something went horribly wrong in dequer");
//                 abort();
//             }
//             for (int k = j + 1; k < subframe_msgs[i].num_msgs; ++k)
//             {
//                 message_buffer_t *msg_k = subframe_msgs[i].msgs[k];
//                 uint16_t sfn_sf_k;
//                 if (!get_sfnsf(msg_k, &sfn_sf_k))
//                 {
//                     nfapi_error("Something went horribly wrong in dequer");
//                     abort();
//                 }

//                 if (sfn_sf_j != sfn_sf_k)
//                 {
//                     is_mismatch = true;
//                     msg_mismatch_count++;
//                 }
//             }
//         }
//         if (is_mismatch)
//         {
//             total_mismatches += msg_mismatch_count;
//             nfapi_error("UE's have mismatched sfn_sfs in their message buffers "
//                         "msg_mismatch_count = %d , total mismatches = %d",
//                         msg_mismatch_count,
//                         total_mismatches);
//             is_mismatch = false;
//         }
//     }
// }

bool dequeue_ue_msgs(subframe_msgs_t *subframe_msgs, uint16_t sfn_sf_tx)
{
    // Dequeue for all UE responses, and discard any with the wrong sfn/sf value.
    // There might be multiple messages from a given UE with the same sfn/sf value.
    bool are_queues_empty = true;
    uint16_t master_sfn_sf = 0xFFFF;
    for (int i = 0; i < num_ues; i++)
    {
        for (;;)
        {
            message_buffer_t *msg = get_queue(&msgs_from_ue[i]);
            if (!msg)
            {
                break;
            }
            assert(msg->magic == MESSAGE_BUFFER_MAGIC);
            if (msg->length == 4)
            {
                msg->magic = 0;
                free(msg);
                continue;
            }
            uint16_t msg_sfn_sf;
            if (!get_sfnsf(msg, &msg_sfn_sf))
            {
                msg->magic = 0;
                free(msg);
                continue;
            }
            if (master_sfn_sf == 0xFFFF)
            {
                master_sfn_sf = msg_sfn_sf;
            }
            if (master_sfn_sf != msg_sfn_sf)
            {
                if (!requeue(&msgs_from_ue[i], msg))
                {
                    msg->magic = 0;
                    free(msg);
                }
                are_queues_empty = false;
                break;
            }
            int sf_delta = get_sf_delta(sfn_sf_tx, msg_sfn_sf);
            if (sf_delta > 2)
            {
                nfapi_error("sfn_sf not correct %u.%u - %u.%u = %d message id: %d", sfn_sf_tx >> 4,
                            sfn_sf_tx & 0XF, msg_sfn_sf >> 4, msg_sfn_sf & 0XF, sf_delta, get_message_id(msg));
                msg->magic = 0;
                free(msg);
                continue;
            }
            subframe_msgs_t *p = &subframe_msgs[i];
            if (p->num_msgs == MAX_SUBFRAME_MSGS)
            {
                nfapi_error("Too many msgs from ue");
                msg->magic = 0;
                free(msg);
                continue;
            }
            p->msgs[p->num_msgs++] = msg;
            nfapi_info("p->num_msgs = %zu", p->num_msgs);
        }
    }

    return are_queues_empty;
}

void add_sleep_time(uint64_t start, uint64_t poll, uint64_t send, uint64_t agg)
{
    nfapi_info("polling took %" PRIu64 "usec, subframe ind took %" PRIu64 "usec, aggreg took %" PRIu64 "usec",
                poll - start,
                send - poll,
                agg - send);

    /* See how long this iteration of this loop took, and sleep for the
        balance of the 1ms subframe */
    uint64_t elapsed_usec = agg - start;
    if (elapsed_usec <= 1000)
    {
        usleep(1000 - elapsed_usec);
    }
    else
    {
        nfapi_info("Subframe loop took too long by %" PRIu64 "usec",
                    elapsed_usec - 1000);
    }
}

static uint16_t sfn_sf_counter(uint16_t *sfn, uint16_t *sf)
{
    if (++*sf == 10)
    {
        *sf = 0;
        if (++*sfn == 1024)
        {
            *sfn = 0;
        }
    }
    return (*sfn << 4) | *sf;
}

void *oai_subframe_task(void *context)
{
    pnf_set_thread_priority(79);
    uint16_t sfn = 0;
    uint16_t sf = 0;
    bool are_queues_empty = true;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Subframe Task thread");
    while (true)
    {

        uint16_t sfn_sf_tx = sfn_sf_counter(&sfn, &sf);

        uint64_t iteration_start = clock_usec();

        transfer_downstream_sfn_sf_to_emane(sfn_sf_tx); // send to oai UE
        nfapi_info("Frame %u Subframe %u sent to OAI ue", sfn_sf_tx >> 4,
                   sfn_sf_tx & 0XF);

        if (are_queues_empty)
        {
            usleep(825);
        }

        uint64_t poll_end = clock_usec();
        oai_subframe_ind(sfn, sf);
        uint64_t subframe_sent = clock_usec();

        /*
            Dequeue, collect and aggregate the messages with the same message ID.
        */
        subframe_msgs_t subframe_msgs[MAX_UES];
        memset(subframe_msgs, 0, sizeof(subframe_msgs));
        are_queues_empty = dequeue_ue_msgs(subframe_msgs, sfn_sf_tx);

        oai_subframe_aggregate_messages(subframe_msgs);
        uint64_t aggregation_done = clock_usec();

        if (are_queues_empty)
        {
            add_sleep_time(iteration_start, poll_end, subframe_sent, aggregation_done);
        }
    }
}

void oai_subframe_handle_msg_from_ue(const void *msg, size_t len, uint16_t nem_id)
{
    if (len == 4) // Dummy packet ignore
    {
        return;
    }
    uint16_t sfn_sf = nfapi_get_sfnsf(msg, len);
    nfapi_info("(Proxy) Adding %s uplink message to queue prior to sending to eNB. Frame: %d, Subframe: %d",
                nfapi_get_message_id(msg, len), NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));

    int i = (int)nem_id - MIN_UE_NEM_ID;
    if (i < 0 || i >= num_ues)
    {
        nfapi_error("nem_id out of range: %d", nem_id);
        return;
    }

    message_buffer_t *p = malloc(sizeof(message_buffer_t));
    assert(p != NULL);
    p->magic = MESSAGE_BUFFER_MAGIC;
    p->length = len;
    memcpy(p->data, msg, len);

    if (!put_queue(&msgs_from_ue[i], p))
    {
        p->magic = 0;
        free(p);
    }
}

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind)
{

    rach_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

    nfapi_info("Sent the rach to eNB sf: %u sfn : %u num of preambles: %u",
               rach_ind->sfn_sf & 0xF, rach_ind->sfn_sf >> 4, rach_ind->rach_indication_body.number_of_preambles);

    return nfapi_pnf_p7_rach_ind(p7_config_g, rach_ind);
}

int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind)
{
    harq_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    harq_ind->header.message_id = NFAPI_HARQ_INDICATION;
    nfapi_info("sfn_sf:%d number_of_harqs:%d\n", NFAPI_SFNSF2DEC(harq_ind->sfn_sf),
               harq_ind->harq_indication_body.number_of_harqs);
    int retval = nfapi_pnf_p7_harq_ind(p7_config_g, harq_ind);

    if (retval != 0)
    {
        nfapi_error("sfn_sf:%d number_of_harqs:%d nfapi_pnf_p7_harq_ind()=%d\n", NFAPI_SFNSF2DEC(harq_ind->sfn_sf),
                    harq_ind->harq_indication_body.number_of_harqs, retval);
    }

    return retval;
}

int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind)
{

    crc_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    crc_ind->header.message_id = NFAPI_CRC_INDICATION;

    return nfapi_pnf_p7_crc_ind(p7_config_g, crc_ind);
}

int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *ind)
{
    ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    ind->header.message_id = NFAPI_RX_CQI_INDICATION;

    uint16_t num_cqis = ind->cqi_indication_body.number_of_cqis;
    for (int i = 0; i < num_cqis; ++i)
    {
        for (int j = i + 1; j < num_cqis; ++j)
        {
            int rnti_i = ind->cqi_indication_body.cqi_pdu_list[i].rx_ue_information.rnti;
            int rnti_j = ind->cqi_indication_body.cqi_pdu_list[j].rx_ue_information.rnti;
            if (rnti_i == rnti_j)
            {
                nfapi_error("two cqis for single UE");
            }
        }
    }

    return nfapi_pnf_p7_cqi_ind(p7_config_g, ind);
}

int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind) // msg 3
{

    ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    ind->header.message_id = NFAPI_RX_ULSCH_INDICATION;

    // pack and unpack test of aggregated packets
    char foo[1024];
    char buffer[1024];
    int encoded_size = nfapi_p7_message_pack(ind, buffer, sizeof(buffer), NULL);
    nfapi_error("C. %s", hexdump(buffer, encoded_size, foo, sizeof(foo)));
    if (encoded_size <= 0)
    {
        nfapi_error("Failed to pack aggregated RX_IND for SFN.SF %u.%u",
                    ind->sfn_sf >> 4, ind->sfn_sf & 15);
    }

    nfapi_rx_indication_t test_ind;
    memset(&test_ind, 0, sizeof(test_ind));

    if (nfapi_p7_message_unpack(buffer, encoded_size, &test_ind, sizeof(test_ind), NULL) != 0)
    {
        nfapi_error("Failed to unpack aggregated RX_IND for SFN.SF %u.%u",
                    ind->sfn_sf >> 4, ind->sfn_sf & 15);
    }

    int retval = nfapi_pnf_p7_rx_ind(p7_config_g, ind);

    uint16_t num_rx = ind->rx_indication_body.number_of_pdus;
    for (int i = 0; i < num_rx; ++i)
    {
        for (int j = i + 1; j < num_rx; ++j)
        {
            int rnti_i = ind->rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti;
            int rnti_j = ind->rx_indication_body.rx_pdu_list[j].rx_ue_information.rnti;
            if (rnti_i == rnti_j)
            {
                nfapi_error("two rx for a single UE rnti: %x", rnti_i);
            }
        }
        nfapi_info("rel8 tag[%d]: %x length %u", i, ind->rx_indication_body.rx_pdu_list[i].rx_indication_rel8.tl.tag,
                   ind->rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length);
    }

    return retval;
}

int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind)
{

    ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    int retval = nfapi_pnf_p7_sr_ind(p7_config_g, ind);
    return retval;
}

//DUMMY Functions to help integrate into proxy

void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,
                             int frame,
                             int subframe,
                             L1_rxtx_proc_t *proc,
                             nfapi_dl_config_request_pdu_t *dl_config_pdu)
{
    (void)eNB;
    (void)frame;
    (void)subframe;
    (void)proc;
    (void)dl_config_pdu;
    nfapi_info("TODO implement me");
}

void handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB,
                         L1_rxtx_proc_t *proc,
                         nfapi_ul_config_request_pdu_t *ul_config_pdu,
                         uint16_t frame,
                         uint8_t subframe,
                         uint8_t srs_present)
{
    (void)eNB;
    (void)proc;
    (void)ul_config_pdu;
    (void)frame;
    (void)subframe;
    (void)srs_present;
    nfapi_info("TODO implement me");
}

void handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB,
                            int frame,
                            int subframe,
                            L1_rxtx_proc_t *proc,
                            nfapi_dl_config_request_pdu_t *dl_config_pdu,
                            uint8_t codeword_index,
                            uint8_t *sdu)
{
    (void)eNB;
    (void)frame;
    (void)subframe;
    (void)proc;
    (void)dl_config_pdu;
    (void)codeword_index;
    (void)sdu;
    nfapi_info("TODO implement me");
}

void handle_nfapi_hi_dci0_dci_pdu(
    PHY_VARS_eNB *eNB,
    int frame,
    int subframe,
    L1_rxtx_proc_t *proc,
    nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu)
{
    (void)eNB;
    (void)frame;
    (void)subframe;
    (void)proc;
    (void)hi_dci0_config_pdu;
    nfapi_info("TODO implement me");
}

void handle_nfapi_bch_pdu(PHY_VARS_eNB *eNB,
                          L1_rxtx_proc_t *proc,
                          nfapi_dl_config_request_pdu_t *dl_config_pdu,
                          uint8_t *sdu)
{
    (void)eNB;
    (void)proc;
    (void)dl_config_pdu;
    (void)sdu;
    nfapi_info("TODO implement me");
}

#ifdef __cplusplus
}
#endif

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
#include "proxy.h"
#include <inttypes.h>
#include <assert.h>

#ifdef NDEBUG
#  warning assert is disabled
#endif

/*
#include "common/ran_context.h"
#include "openair2/PHY_INTERFACE/phy_stub_UE.h"
#include "common/utils/LOG/log.h"

#include "PHY/INIT/phy_init.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair1/SCHED_NR/fapi_nr_l1.h"
#include "openair1/PHY/NR_TRANSPORT/nr_dlsch.h"
#include "openair1/PHY/defs_gNB.h"

#define NUM_P5_PHY 2
extern RAN_CONTEXT_t RC;
extern void phy_init_RU(RU_t *);
extern int config_sync_var;

extern pthread_cond_t nfapi_sync_cond;
extern pthread_mutex_t nfapi_sync_mutex;
extern int nfapi_sync_var;

extern int sync_var;
*/
char uecap_xer_in;

nfapi_tx_request_pdu_t *tx_request_pdu[1023][10][10]; // [frame][subframe][max_num_pdus]
nfapi_nr_pdu_t *tx_data_request[1023][20][10]; //[frame][slot][max_num_pdus]
nfapi_ue_release_request_body_t release_rntis;
nfapi_pnf_param_response_t g_pnf_param_resp;
nfapi_pnf_p7_config_t *p7_config_g = NULL;
nfapi_pnf_p7_config_t *p7_nr_config_g = NULL;

uint8_t tx_pdus[32][8][4096];
uint8_t nr_tx_pdus[32][16][4096];
uint16_t phy_antenna_capability_values[] = { 1, 2, 4, 8, 16 };

static pnf_info pnf;
static pnf_info pnf_nr;
static pthread_t pnf_start_pthread;
static pthread_t pnf_nr_start_pthread;

void *pnf_allocate(size_t size)
{
    return malloc(size);
}

void pnf_deallocate(void *ptr)
{
    free(ptr);
}

void pnf_set_thread_priority(int priority)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "This is priority %d\n", priority);

    struct sched_param schedParam =
    {
        .sched_priority = priority,
    };

    if (sched_setscheduler(0, SCHED_RR, &schedParam) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to set SCHED_RR %s\n", strerror(errno));
        abort();
    }
}
/*
void pnf_set_thread_priority(int priority) {
  pthread_attr_t ptAttr;
  struct sched_param schedParam;
  schedParam.__sched_priority = priority;

  if(sched_setscheduler(0, SCHED_RR, &schedParam) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to set SCHED_RR\n");
  }

  if(pthread_attr_setschedpolicy(&ptAttr, SCHED_RR) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to set pthread SCHED_RR %d\n", errno);
  }

  pthread_attr_setinheritsched(&ptAttr, PTHREAD_EXPLICIT_SCHED);
  struct sched_param thread_params;
  thread_params.sched_priority = 20;

  if(pthread_attr_setschedparam(&ptAttr, &thread_params) != 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "failed to set sched param\n");
  }
}
*/

void *pnf_p7_thread_start(void *ptr)
{
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] P7 THREAD %s\n", __FUNCTION__);
    pnf_set_thread_priority(79);
    log_scheduler(__func__);
    nfapi_pnf_p7_config_t *config = (nfapi_pnf_p7_config_t *)ptr;
    nfapi_pnf_p7_start(config);

    return 0;
}

void *pnf_nr_p7_thread_start(void *ptr) {
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] P7 THREAD %s\n", __FUNCTION__);
  pnf_set_thread_priority(79);
  nfapi_pnf_p7_config_t *config = (nfapi_pnf_p7_config_t *)ptr;
  nfapi_nr_pnf_p7_start(config);
  return 0;
}


int pnf_nr_param_request(nfapi_pnf_config_t *config, nfapi_nr_pnf_param_request_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf param request\n");
  nfapi_nr_pnf_param_response_t resp;
  memset(&resp, 0, sizeof(resp));
  resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
  resp.error_code = NFAPI_MSG_OK;
  pnf_info *pnf = (pnf_info *)(config->user_data);
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

  for(int i = 0; i < 1; ++i) {
    resp.pnf_phy.phy[i].phy_config_index = pnf->phys[i].index;
    resp.pnf_phy.phy[i].downlink_channel_bandwidth_supported = pnf->phys[i].dl_channel_bw_support;
    resp.pnf_phy.phy[i].uplink_channel_bandwidth_supported = pnf->phys[i].ul_channel_bw_support;
    resp.pnf_phy.phy[i].number_of_dl_layers_supported = pnf->phys[i].num_dl_layers_supported;
    resp.pnf_phy.phy[i].number_of_ul_layers_supported = pnf->phys[i].num_ul_layers_supported;
    resp.pnf_phy.phy[i].maximum_3gpp_release_supported = pnf->phys[i].release_supported;
    resp.pnf_phy.phy[i].nmm_modes_supported = pnf->phys[i].nmm_modes_supported;
    resp.pnf_phy.phy[i].number_of_rfs = 2;

    for(int j = 0; j < 1; ++j) {
      resp.pnf_phy.phy[i].rf_config[j].rf_config_index = pnf->phys[i].rfs[j];
    }

    resp.pnf_phy.phy[i].number_of_rf_exclusions = 0;

    for(int j = 0; j < 0; ++j) {
      resp.pnf_phy.phy[i].excluded_rf_config[j].rf_config_index = pnf->phys[i].excluded_rfs[j];
    }
  }
  nfapi_nr_pnf_pnf_param_resp(config, &resp);
  return 0;
}

int pnf_param_request(nfapi_pnf_config_t *config, nfapi_pnf_param_request_t *req)
{

    (void)req;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf param request\n");
    nfapi_pnf_param_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_PARAM_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    pnf_info *pnf = (pnf_info *)(config->user_data);
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

    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf config request\n");
    pnf_info *pnf = (pnf_info *)(config->user_data);
    phy_info *phy = pnf->phys;
    phy->id = req->pnf_phy_rf_config.phy_rf_config[0].phy_id;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf config request assigned phy_id %d to phy_config_index %d\n", phy->id,
           req->pnf_phy_rf_config.phy_rf_config[0].phy_config_index);
    nfapi_pnf_config_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_CONFIG_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_pnf_config_resp(config, &resp);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent pnf_config_resp\n");

    return 0;
}

int pnf_nr_config_request(nfapi_pnf_config_t *config, nfapi_nr_pnf_config_request_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf config request\n");
  pnf_info *pnf = (pnf_info *)(config->user_data);
  phy_info *phy = pnf->phys;
  phy->id = req->pnf_phy_rf_config.phy_rf_config[0].phy_id;
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] pnf config request assigned phy_id %d to phy_config_index %d\n", phy->id, req->pnf_phy_rf_config.phy_rf_config[0].phy_config_index);
  nfapi_nr_pnf_config_response_t resp;
  memset(&resp, 0, sizeof(resp));
  resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE;
  resp.error_code = NFAPI_MSG_OK;
  nfapi_nr_pnf_pnf_config_resp(config, &resp);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent pnf_config_resp\n");
  return 0;
}

void nfapi_send_pnf_start_resp(nfapi_pnf_config_t *config, uint16_t phy_id)
{
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Sending NFAPI_START_RESPONSE config:%p phy_id:%d\n", config, phy_id);
    nfapi_start_response_t start_resp;
    memset(&start_resp, 0, sizeof(start_resp));
    start_resp.header.message_id = NFAPI_START_RESPONSE;
    start_resp.header.phy_id = phy_id;
    start_resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_start_resp(config, &start_resp);
}

void nfapi_nr_send_pnf_start_resp(nfapi_pnf_config_t *config, uint16_t phy_id) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Sending NFAPI_START_RESPONSE config:%p phy_id:%d\n", config, phy_id);
  nfapi_nr_start_response_scf_t start_resp;
  memset(&start_resp, 0, sizeof(start_resp));
  start_resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE;
  start_resp.header.phy_id = phy_id;
  start_resp.error_code = NFAPI_MSG_OK;
  nfapi_nr_pnf_start_resp(config, &start_resp);
}

int pnf_start_request(nfapi_pnf_config_t *config, nfapi_pnf_start_request_t *req)
{

    (void)req;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Received NFAPI_PNF_START_REQUEST\n");
    pnf_info *pnf = (pnf_info *)(config->user_data);
    // start all phys that have been configured
    phy_info *phy = pnf->phys;

    if (phy->id != 0)
    {
        nfapi_pnf_start_response_t resp;
        memset(&resp, 0, sizeof(resp));
        resp.header.message_id = NFAPI_PNF_START_RESPONSE;
        resp.error_code = NFAPI_MSG_OK;
        nfapi_pnf_pnf_start_resp(config, &resp);
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PNF_START_RESP\n");
    }

    return 0;
}

int pnf_nr_start_request(nfapi_pnf_config_t *config, nfapi_nr_pnf_start_request_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Received NFAPI_PNF_START_REQUEST\n");
  pnf_info *pnf = (pnf_info *)(config->user_data);
  // start all phys that have been configured
  phy_info *phy = pnf->phys;

  if(phy->id != 0) {
    nfapi_nr_pnf_start_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_nr_pnf_pnf_start_resp(config, &resp);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PNF_START_RESP\n");
  }

  return 0;
}

int pnf_stop_request(nfapi_pnf_config_t *config, nfapi_pnf_stop_request_t *req)
{

    (void)req;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_PNF_STOP_REQ\n");
    nfapi_pnf_stop_response_t resp;
    memset(&resp, 0, sizeof(resp));
    resp.header.message_id = NFAPI_PNF_STOP_RESPONSE;
    resp.error_code = NFAPI_MSG_OK;
    nfapi_pnf_pnf_stop_resp(config, &resp);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PNF_STOP_REQ\n");

    return 0;
}

int param_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_param_request_t *req)
{

    (void)phy;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_PARAM_REQUEST phy_id:%d\n", req->header.phy_id);

    nfapi_param_response_t nfapi_resp;
    pnf_info *pnf = (pnf_info *)(config->user_data);
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
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PARAM_RESPONSE phy_id:%d number_of_tlvs:%u\n", req->header.phy_id, nfapi_resp.num_tlv);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] param request .. exit\n");

    return 0;
}

int nr_param_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_nr_param_request_scf_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_PARAM_REQUEST phy_id:%d\n", req->header.phy_id);
  //pnf_info* pnf = (pnf_info*)(config->user_data);
  nfapi_nr_param_response_scf_t nfapi_resp;
  pnf_info *pnf = (pnf_info *)(config->user_data);
  memset(&nfapi_resp, 0, sizeof(nfapi_resp));
  nfapi_resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE;
  nfapi_resp.header.phy_id = req->header.phy_id;
  nfapi_resp.error_code = 0; // DJP - what value???
  struct sockaddr_in pnf_p7_sockaddr;

  // ASSIGN TAGS
  {
  nfapi_resp.cell_param.release_capability.tl.tag = NFAPI_NR_PARAM_TLV_RELEASE_CAPABILITY_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.cell_param.phy_state.tl.tag =			 NFAPI_NR_PARAM_TLV_PHY_STATE_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.cell_param.skip_blank_dl_config.tl.tag =			 NFAPI_NR_PARAM_TLV_SKIP_BLANK_DL_CONFIG_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.cell_param.skip_blank_ul_config.tl.tag =			 NFAPI_NR_PARAM_TLV_SKIP_BLANK_UL_CONFIG_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.cell_param.num_config_tlvs_to_report .tl.tag =			 NFAPI_NR_PARAM_TLV_NUM_CONFIG_TLVS_TO_REPORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.carrier_param.cyclic_prefix.tl.tag =			 NFAPI_NR_PARAM_TLV_CYCLIC_PREFIX_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.carrier_param.supported_subcarrier_spacings_dl.tl.tag =			 NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_DL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.carrier_param.supported_bandwidth_dl.tl.tag =			 NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_DL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.carrier_param.supported_subcarrier_spacings_ul.tl.tag =			 NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_UL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.carrier_param.supported_bandwidth_ul.tl.tag =			 NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_UL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.cce_mapping_type.tl.tag =			 NFAPI_NR_PARAM_TLV_CCE_MAPPING_TYPE_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.coreset_outside_first_3_of_ofdm_syms_of_slot.tl.tag =			 NFAPI_NR_PARAM_TLV_CORESET_OUTSIDE_FIRST_3_OFDM_SYMS_OF_SLOT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.coreset_precoder_granularity_coreset.tl.tag =			 NFAPI_NR_PARAM_TLV_PRECODER_GRANULARITY_CORESET_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.pdcch_mu_mimo.tl.tag =			 NFAPI_NR_PARAM_TLV_PDCCH_MU_MIMO_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.pdcch_precoder_cycling.tl.tag =			 NFAPI_NR_PARAM_TLV_PDCCH_PRECODER_CYCLING_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdcch_param.max_pdcch_per_slot.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_PDCCHS_PER_SLOT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pucch_param.pucch_formats.tl.tag =			 NFAPI_NR_PARAM_TLV_PUCCH_FORMATS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pucch_param.max_pucchs_per_slot.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_PUCCHS_PER_SLOT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_mapping_type.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_MAPPING_TYPE_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_dmrs_additional_pos.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_DMRS_ADDITIONAL_POS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_allocation_types.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_ALLOCATION_TYPES_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_vrb_to_prb_mapping.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_VRB_TO_PRB_MAPPING_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_cbg.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_CBG_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_dmrs_config_types.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_DMRS_CONFIG_TYPES_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.max_number_mimo_layers_pdsch.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_PDSCH_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.max_mu_mimo_users_dl.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_DL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_data_in_dmrs_symbols.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_DATA_IN_DMRS_SYMBOLS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.premption_support.tl.tag =			 NFAPI_NR_PARAM_TLV_PREMPTION_SUPPORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pdsch_param.pdsch_non_slot_support.tl.tag =			 NFAPI_NR_PARAM_TLV_PDSCH_NON_SLOT_SUPPORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.uci_mux_ulsch_in_pusch.tl.tag =			 NFAPI_NR_PARAM_TLV_UCI_MUX_ULSCH_IN_PUSCH_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.uci_only_pusch.tl.tag =			 NFAPI_NR_PARAM_TLV_UCI_ONLY_PUSCH_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_frequency_hopping.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_FREQUENCY_HOPPING_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_dmrs_config_types.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_DMRS_CONFIG_TYPES_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_dmrs_max_len.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_DMRS_MAX_LEN_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_dmrs_additional_pos.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_DMRS_ADDITIONAL_POS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_cbg.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_CBG_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_mapping_type.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_MAPPING_TYPE_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_allocation_types.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_ALLOCATION_TYPES_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_vrb_to_prb_mapping.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_VRB_TO_PRB_MAPPING_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_max_ptrs_ports.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_MAX_PTRS_PORTS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.max_pduschs_tbs_per_slot.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_PDUSCHS_TBS_PER_SLOT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.max_number_mimo_layers_non_cb_pusch.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_NON_CB_PUSCH_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.supported_modulation_order_ul.tl.tag =			 NFAPI_NR_PARAM_TLV_SUPPORTED_MODULATION_ORDER_UL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.max_mu_mimo_users_ul.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_UL_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.dfts_ofdm_support.tl.tag =			 NFAPI_NR_PARAM_TLV_DFTS_OFDM_SUPPORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.pusch_param.pusch_aggregation_factor.tl.tag =			 NFAPI_NR_PARAM_TLV_PUSCH_AGGREGATION_FACTOR_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.prach_param.prach_long_formats.tl.tag =             NFAPI_NR_PARAM_TLV_PRACH_LONG_FORMATS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.prach_param.prach_short_formats.tl.tag =			 NFAPI_NR_PARAM_TLV_PRACH_SHORT_FORMATS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.prach_param.prach_restricted_sets.tl.tag =			 NFAPI_NR_PARAM_TLV_PRACH_RESTRICTED_SETS_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.prach_param.max_prach_fd_occasions_in_a_slot.tl.tag =			 NFAPI_NR_PARAM_TLV_MAX_PRACH_FD_OCCASIONS_IN_A_SLOT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.measurement_param.rssi_measurement_support.tl.tag =			 NFAPI_NR_PARAM_TLV_RSSI_MEASUREMENT_SUPPORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_vnf_address_ipv4.tl.tag =			 NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV4_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_vnf_address_ipv6.tl.tag =			 NFAPI_NR_NFAPI_P7_VNF_ADDRESS_IPV6_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_vnf_port.tl.tag =			 NFAPI_NR_NFAPI_P7_VNF_PORT_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_pnf_address_ipv4.tl.tag =			 NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV4_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_pnf_address_ipv6.tl.tag =			 NFAPI_NR_NFAPI_P7_PNF_ADDRESS_IPV6_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.p7_pnf_port.tl.tag =			 NFAPI_NR_NFAPI_P7_PNF_PORT_TAG;
  nfapi_resp.num_tlv++;
/*
  nfapi_resp.nfapi_config.dl_ue_per_sf.tl.tag =			 NFAPI_NR_NFAPI_DOWNLINK_UES_PER_SUBFRAME_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.ul_ue_per_sf.tl.tag =			 NFAPI_NR_NFAPI_UPLINK_UES_PER_SUBFRAME_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.rf_bands.tl.tag =			 NFAPI_NR_NFAPI_RF_BANDS_TAG;
  nfapi_resp.num_tlv++;
  nfapi_resp.nfapi_config.max_transmit_power.tl.tag =			 NFAPI_NR_NFAPI_MAXIMUM_TRANSMIT_POWER_TAG;
  nfapi_resp.num_tlv++;
*/
  nfapi_resp.nfapi_config.timing_window.tl.tag =			 NFAPI_NR_NFAPI_TIMING_WINDOW_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.timing_info_mode.tl.tag =			 NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG;
  nfapi_resp.num_tlv++;

  nfapi_resp.nfapi_config.timing_info_period.tl.tag =			 NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG;
  nfapi_resp.num_tlv++;
  }

  nfapi_resp.nfapi_config.p7_pnf_port.value = 50610; //pnf->phys[0].local_port; DJP - hard code alert!!!! FIXME TODO
  nfapi_resp.num_tlv++;
  pnf_p7_sockaddr.sin_addr.s_addr = inet_addr(pnf->phys[0].local_addr);
  memcpy(nfapi_resp.nfapi_config.p7_pnf_address_ipv4.address, &pnf_p7_sockaddr.sin_addr.s_addr, 4);
  nfapi_resp.num_tlv++;
  // P7 PNF Port
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "TAG value :%d",nfapi_resp.cell_param.phy_state.tl.tag);
  nfapi_nr_pnf_param_resp(config, &nfapi_resp);

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PNF_PARAM_RESPONSE phy_id:%d number_of_tlvs:%u\n", req->header.phy_id, nfapi_resp.num_tlv);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] param request .. exit\n");
  return 0;
}

int config_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_config_request_t *req)
{

    (void)phy;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_CONFIG_REQ phy_id:%d\n", req->header.phy_id);
    pnf_info *pnf = (pnf_info *)(config->user_data);
    uint8_t num_tlv = 0;
    phy_info *phy_info = pnf->phys;

    if (req->nfapi_config.timing_window.tl.tag == NFAPI_NFAPI_TIMING_WINDOW_TAG)
    {
        phy_info->timing_window = req->nfapi_config.timing_window.value;
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "phy_info:Timing window:%u NFAPI_CONFIG:timing_window:%u\n", phy_info->timing_window,
               req->nfapi_config.timing_window.value);
        num_tlv++;
    }

    if (req->nfapi_config.timing_info_mode.tl.tag == NFAPI_NFAPI_TIMING_INFO_MODE_TAG)
    {
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info mode:%d\n", req->nfapi_config.timing_info_mode.value);
        phy_info->timing_info_mode = req->nfapi_config.timing_info_mode.value;
        num_tlv++;
    }
    else
    {
        phy_info->timing_info_mode = 0;
        NFAPI_TRACE(NFAPI_TRACE_WARN, "NO timing info mode provided\n");
    }

    if (req->nfapi_config.timing_info_period.tl.tag == NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG)
    {
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info period provided value:%d\n", req->nfapi_config.timing_info_period.value);
        phy_info->timing_info_period = req->nfapi_config.timing_info_period.value;
        num_tlv++;
    }
    else
    {
        phy_info->timing_info_period = 0;
    }

    if (req->rf_config.dl_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG)
    {
        phy_info->dl_channel_bw_support = req->rf_config.dl_channel_bandwidth.value;
        num_tlv++;
    }
    else
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() Missing NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG\n", __FUNCTION__);
    }

    if (req->rf_config.ul_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG)
    {
        phy_info->ul_channel_bw_support = req->rf_config.ul_channel_bandwidth.value;
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

    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] CONFIG_REQUEST[num_tlv:%d] TLVs processed:%d\n", req->num_tlv, num_tlv); // make this an assert
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Simulating PHY CONFIG - DJP\n");

    phy_info->remote_port = req->nfapi_config.p7_vnf_port.value;
    struct sockaddr_in vnf_p7_sockaddr;
    memcpy(&vnf_p7_sockaddr.sin_addr.s_addr, &(req->nfapi_config.p7_vnf_address_ipv4.address[0]), 4);
    phy_info->remote_addr = inet_ntoa(vnf_p7_sockaddr.sin_addr);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] %d vnf p7 %s:%d timing %d %d %d\n", phy_info->id, phy_info->remote_addr,
           phy_info->remote_port,
           phy_info->timing_window, phy_info->timing_info_mode, phy_info->timing_info_period);
    nfapi_config_response_t nfapi_resp;
    memset(&nfapi_resp, 0, sizeof(nfapi_resp));
    nfapi_resp.header.message_id = NFAPI_CONFIG_RESPONSE;
    nfapi_resp.header.phy_id = phy_info->id;
    nfapi_resp.error_code = 0; // DJP - some value resp->error_code;
    nfapi_pnf_config_resp(config, &nfapi_resp);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_CONFIG_RESPONSE phy_id:%d\n", phy_info->id);

    return 0;
}

/*
int config_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_config_request_t *req)
{
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_CONFIG_REQ phy_id:%d\n", req->header.phy_id);
  pnf_info *pnf = (pnf_info *)(config->user_data);
  uint8_t num_tlv = 0;
  //struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  //  In the case of nfapi_mode = 3 (UE = PNF) we should not have dependency on any eNB var. So we aim
  // to keep only the necessary just to keep the nfapi FSM rolling by sending a dummy response.
  LTE_DL_FRAME_PARMS *fp;

  if (NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
    fp = &eNB->frame_parms;
  } else {
    fp = (LTE_DL_FRAME_PARMS *) malloc(sizeof(LTE_DL_FRAME_PARMS));
  }

  phy_info *phy_info = pnf->phys;

  if(req->nfapi_config.timing_window.tl.tag == NFAPI_NFAPI_TIMING_WINDOW_TAG) {
    phy_info->timing_window = req->nfapi_config.timing_window.value;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Phy_info:Timing window:%u NFAPI_CONFIG:timing_window:%u\n", phy_info->timing_window, req->nfapi_config.timing_window.value);
    num_tlv++;
  }

  if(req->nfapi_config.timing_info_mode.tl.tag == NFAPI_NFAPI_TIMING_INFO_MODE_TAG) {
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info mode:%d\n", req->nfapi_config.timing_info_mode.value);
    phy_info->timing_info_mode = req->nfapi_config.timing_info_mode.value;
    num_tlv++;
  } else {
    phy_info->timing_info_mode = 0;
    NFAPI_TRACE(NFAPI_TRACE_WARN, "NO timing info mode provided\n");
  }

  if(req->nfapi_config.timing_info_period.tl.tag == NFAPI_NFAPI_TIMING_INFO_PERIOD_TAG) {
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info period provided value:%d\n", req->nfapi_config.timing_info_period.value);
    phy_info->timing_info_period = req->nfapi_config.timing_info_period.value;
    num_tlv++;
  } else {
    phy_info->timing_info_period = 0;
  }

  if(req->rf_config.dl_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG) {
    phy_info->dl_channel_bw_support = req->rf_config.dl_channel_bandwidth.value;
    fp->N_RB_DL = req->rf_config.dl_channel_bandwidth.value;
    num_tlv++;
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG N_RB_DL:%u\n", __FUNCTION__, fp->N_RB_DL);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() Missing NFAPI_RF_CONFIG_DL_CHANNEL_BANDWIDTH_TAG\n", __FUNCTION__);
  }

  if(req->rf_config.ul_channel_bandwidth.tl.tag == NFAPI_RF_CONFIG_UL_CHANNEL_BANDWIDTH_TAG) {
    phy_info->ul_channel_bw_support = req->rf_config.ul_channel_bandwidth.value;
    fp->N_RB_UL = req->rf_config.ul_channel_bandwidth.value;
    num_tlv++;
  }

  if(req->nfapi_config.rf_bands.tl.tag == NFAPI_NFAPI_RF_BANDS_TAG) {
    pnf->rfs[0].band = req->nfapi_config.rf_bands.rf_band[0];
    fp->eutra_band = req->nfapi_config.rf_bands.rf_band[0];
    num_tlv++;
  }

  if(req->nfapi_config.earfcn.tl.tag == NFAPI_NFAPI_EARFCN_TAG) {
    fp->dl_CarrierFreq = from_earfcn(fp->eutra_band, req->nfapi_config.earfcn.value);
    fp->ul_CarrierFreq = fp->dl_CarrierFreq - (get_uldl_offset(fp->eutra_band) * 1e5);
    num_tlv++;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() earfcn:%u dl_carrierFreq:%u ul_CarrierFreq:%u band:%u N_RB_DL:%u\n",
                __FUNCTION__, req->nfapi_config.earfcn.value, fp->dl_CarrierFreq, fp->ul_CarrierFreq, pnf->rfs[0].band, fp->N_RB_DL);
  }

  if (req->subframe_config.duplex_mode.tl.tag == NFAPI_SUBFRAME_CONFIG_DUPLEX_MODE_TAG) {
    fp->frame_type = req->subframe_config.duplex_mode.value==0 ? TDD : FDD;
    num_tlv++;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() frame_type:%d\n", __FUNCTION__, fp->frame_type);
  }

  if (req->subframe_config.dl_cyclic_prefix_type.tl.tag == NFAPI_SUBFRAME_CONFIG_DL_CYCLIC_PREFIX_TYPE_TAG) {
    fp->Ncp = req->subframe_config.dl_cyclic_prefix_type.value;
    num_tlv++;
  }

  if (req->subframe_config.ul_cyclic_prefix_type.tl.tag == NFAPI_SUBFRAME_CONFIG_UL_CYCLIC_PREFIX_TYPE_TAG) {
    fp->Ncp_UL = req->subframe_config.ul_cyclic_prefix_type.value;
    num_tlv++;
  }

  fp->num_MBSFN_config = 0; // DJP - hard code alert

  if (req->sch_config.physical_cell_id.tl.tag == NFAPI_SCH_CONFIG_PHYSICAL_CELL_ID_TAG) {
    fp->Nid_cell = req->sch_config.physical_cell_id.value;
    fp->nushift = fp->Nid_cell%6;
    num_tlv++;
  }

  if (req->rf_config.tx_antenna_ports.tl.tag == NFAPI_RF_CONFIG_TX_ANTENNA_PORTS_TAG) {
    fp->nb_antennas_tx = req->rf_config.tx_antenna_ports.value;
    fp->nb_antenna_ports_eNB = 1;
    num_tlv++;
  }

  if (req->rf_config.rx_antenna_ports.tl.tag == NFAPI_RF_CONFIG_RX_ANTENNA_PORTS_TAG) {
    fp->nb_antennas_rx = req->rf_config.rx_antenna_ports.value;
    num_tlv++;
  }

  if (req->phich_config.phich_resource.tl.tag == NFAPI_PHICH_CONFIG_PHICH_RESOURCE_TAG) {
    fp->phich_config_common.phich_resource = req->phich_config.phich_resource.value;
    num_tlv++;
  }

  if (req->phich_config.phich_duration.tl.tag == NFAPI_PHICH_CONFIG_PHICH_DURATION_TAG) {
    fp->phich_config_common.phich_duration = req->phich_config.phich_duration.value;
    num_tlv++;
  }

  if (req->phich_config.phich_power_offset.tl.tag == NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG) {
    LOG_E(PHY, "%s() NFAPI_PHICH_CONFIG_PHICH_POWER_OFFSET_TAG tag:%d not supported\n", __FUNCTION__, req->phich_config.phich_power_offset.tl.tag);
    //fp->phich_config_common.phich_power_offset = req->phich_config.
    num_tlv++;
  }

  // UL RS Config
  if (req->uplink_reference_signal_config.cyclic_shift_1_for_drms.tl.tag == NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_CYCLIC_SHIFT_1_FOR_DRMS_TAG) {
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = req->uplink_reference_signal_config.cyclic_shift_1_for_drms.value;
    num_tlv++;
  }

  if (req->uplink_reference_signal_config.uplink_rs_hopping.tl.tag == NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_UPLINK_RS_HOPPING_TAG) {
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = req->uplink_reference_signal_config.uplink_rs_hopping.value;
    num_tlv++;
  }

  if (req->uplink_reference_signal_config.group_assignment.tl.tag == NFAPI_UPLINK_REFERENCE_SIGNAL_CONFIG_GROUP_ASSIGNMENT_TAG) {
    fp->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = req->uplink_reference_signal_config.group_assignment.value;
    num_tlv++;
  }

  if (req->pusch_config.hopping_mode.tl.tag == NFAPI_PUSCH_CONFIG_HOPPING_MODE_TAG) {
  }  // DJP - not being handled?

  if (req->pusch_config.hopping_offset.tl.tag == NFAPI_PUSCH_CONFIG_HOPPING_OFFSET_TAG) {
  }  // DJP - not being handled?

  if (req->pusch_config.number_of_subbands.tl.tag == NFAPI_PUSCH_CONFIG_NUMBER_OF_SUBBANDS_TAG) {
  }  // DJP - not being handled?

  if (req->prach_config.configuration_index.tl.tag == NFAPI_PRACH_CONFIG_CONFIGURATION_INDEX_TAG) {
    fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex=req->prach_config.configuration_index.value;
    num_tlv++;
  }

  if (req->prach_config.root_sequence_index.tl.tag == NFAPI_PRACH_CONFIG_ROOT_SEQUENCE_INDEX_TAG) {
    fp->prach_config_common.rootSequenceIndex=req->prach_config.root_sequence_index.value;
    num_tlv++;
  }

  if (req->prach_config.zero_correlation_zone_configuration.tl.tag == NFAPI_PRACH_CONFIG_ZERO_CORRELATION_ZONE_CONFIGURATION_TAG) {
    fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=req->prach_config.zero_correlation_zone_configuration.value;
    num_tlv++;
  }

  if (req->prach_config.high_speed_flag.tl.tag == NFAPI_PRACH_CONFIG_HIGH_SPEED_FLAG_TAG) {
    fp->prach_config_common.prach_ConfigInfo.highSpeedFlag=req->prach_config.high_speed_flag.value;
    num_tlv++;
  }

  if (req->prach_config.frequency_offset.tl.tag == NFAPI_PRACH_CONFIG_FREQUENCY_OFFSET_TAG) {
    fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset=req->prach_config.frequency_offset.value;
    num_tlv++;
  }

  if(NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] CONFIG_REQUEST[num_tlv:%d] TLVs processed:%d\n", req->num_tlv, num_tlv);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Simulating PHY CONFIG - DJP\n");
    PHY_Config_t phy_config;
    phy_config.Mod_id = 0;
    phy_config.CC_id=0;
    phy_config.cfg = req;
    phy_config_request(&phy_config);
    dump_frame_parms(fp);
  }
    phy_info->remote_port = req->nfapi_config.p7_vnf_port.value;
  struct sockaddr_in vnf_p7_sockaddr;
  memcpy(&vnf_p7_sockaddr.sin_addr.s_addr, &(req->nfapi_config.p7_vnf_address_ipv4.address[0]), 4);
  phy_info->remote_addr = inet_ntoa(vnf_p7_sockaddr.sin_addr);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] %d vnf p7 %s:%d timing %d %d %d\n", phy_info->id, phy_info->remote_addr, phy_info->remote_port,
         phy_info->timing_window, phy_info->timing_info_mode, phy_info->timing_info_period);
  nfapi_config_response_t nfapi_resp;
  memset(&nfapi_resp, 0, sizeof(nfapi_resp));
  nfapi_resp.header.message_id = NFAPI_CONFIG_RESPONSE;
  nfapi_resp.header.phy_id = phy_info->id;
  nfapi_resp.error_code = 0; // DJP - some value resp->error_code;
  nfapi_pnf_config_resp(config, &nfapi_resp);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_CONFIG_RESPONSE phy_id:%d\n", phy_info->id);

  if(NFAPI_MODE==NFAPI_UE_STUB_PNF)
    free(fp);

  return 0;
}

*/

int nr_config_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_nr_config_request_scf_t *req)
{
  (void)phy;
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_CONFIG_REQ phy_id:%d\n", req->header.phy_id);

  pnf_info *pnf = (pnf_info *)(config->user_data);
  phy_info *phy_info = pnf->phys;
  uint8_t num_tlv = 0;

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "\nTiming window tag: %d\n",NFAPI_NR_NFAPI_TIMING_WINDOW_TAG);
  if(req->nfapi_config.timing_window.tl.tag == NFAPI_NR_NFAPI_TIMING_WINDOW_TAG) {
    phy_info->timing_window = req->nfapi_config.timing_window.value;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Phy_info:Timing window:%u NFAPI_CONFIG:timing_window:%u\n", phy_info->timing_window, req->nfapi_config.timing_window.value);
    num_tlv++;
  }

  if(req->nfapi_config.timing_info_mode.tl.tag == NFAPI_NR_NFAPI_TIMING_INFO_MODE_TAG) {
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info mode:%d\n", req->nfapi_config.timing_info_mode.value);
    phy_info->timing_info_mode = req->nfapi_config.timing_info_mode.value;
    num_tlv++;
  } else {
    phy_info->timing_info_mode = 0;
    NFAPI_TRACE(NFAPI_TRACE_WARN, "NO timing info mode provided\n");
  }
  //TODO: Read the P7 message offset values
  if(req->nfapi_config.timing_info_period.tl.tag == NFAPI_NR_NFAPI_TIMING_INFO_PERIOD_TAG) {
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "timing info period provided value:%d\n", req->nfapi_config.timing_info_period.value);
    phy_info->timing_info_period = req->nfapi_config.timing_info_period.value;
    num_tlv++;
  } else {
    phy_info->timing_info_period = 0;
  }

  if(req->carrier_config.dl_bandwidth.tl.tag == NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG) {
    phy_info->dl_channel_bw_support = req->carrier_config.dl_bandwidth.value; //rf_config.dl_channel_bandwidth.value;
    num_tlv++;
  } else {            
  }

  if(req->carrier_config.uplink_bandwidth.tl.tag == NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG) {
    phy_info->ul_channel_bw_support = req->carrier_config.uplink_bandwidth.value; //req->rf_config.ul_channel_bandwidth.value;
    num_tlv++;
  }

  if (req->cell_config.phy_cell_id.tl.tag == NFAPI_NR_CONFIG_PHY_CELL_ID_TAG) {
    num_tlv++;
  }

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] CONFIG_REQUEST[num_tlv:%d] TLVs processed:%d\n", req->num_tlv, num_tlv); // make this an assert
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Simulating PHY CONFIG - DJP\n");

  phy_info->remote_port = req->nfapi_config.p7_vnf_port.value;
  struct sockaddr_in vnf_p7_sockaddr;
  memcpy(&vnf_p7_sockaddr.sin_addr.s_addr, &(req->nfapi_config.p7_vnf_address_ipv4.address[0]), 4);
  phy_info->remote_addr = inet_ntoa(vnf_p7_sockaddr.sin_addr);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] %d vnf p7 %s:%d timing %d %d %d\n", phy_info->id, phy_info->remote_addr, phy_info->remote_port,
         phy_info->timing_window, phy_info->timing_info_mode, phy_info->timing_info_period);
  nfapi_nr_config_response_scf_t nfapi_resp;
  memset(&nfapi_resp, 0, sizeof(nfapi_resp));
  nfapi_resp.header.message_id = NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE;
  nfapi_resp.header.phy_id = phy_info->id;
  nfapi_resp.error_code = 0; // DJP - some value resp->error_code;
  nfapi_nr_pnf_config_resp(config, &nfapi_resp);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent NFAPI_PNF_CONFIG_RESPONSE phy_id:%d\n", phy_info->id);

  return 0;
}

/*
nfapi_p7_message_header_t *pnf_phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t *msg_size) {
  if(message_id == P7_VENDOR_EXT_REQ) {
    (*msg_size) = sizeof(vendor_ext_p7_req);
    return (nfapi_p7_message_header_t *)malloc(sizeof(vendor_ext_p7_req));
  }

  return 0;
}

void pnf_phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t *header) {
  free(header);
}
*/

/*
int pnf_phy_ul_dci_req(gNB_L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_nr_ul_dci_request_t *req) {
  
  //   LOG_D(PHY,"[PNF] HI_DCI0_REQUEST SFN/SF:%05d dci:%d hi:%d\n", NFAPI_SFNSF2DEC(req->sfn_sf), req->hi_dci0_request_body.number_of_dci, req->hi_dci0_request_body.number_of_hi);

  struct PHY_VARS_gNB_s *gNB = RC.gNB[0];
  if (proc ==NULL) 
    proc = &gNB->proc.L1_proc;

  for (int i=0; i<req->numPdus; i++) {
    if (req->ul_dci_pdu_list[i].PDUType == 0) {
      nfapi_nr_ul_dci_request_pdus_t *ul_dci_req_pdu = &req->ul_dci_pdu_list[i]; 
      handle_nfapi_nr_ul_dci_pdu(gNB, req->SFN, req->Slot, ul_dci_req_pdu); 
    } 
    else {
      LOG_E(PHY,"[PNF] UL_DCI_REQ sfn_slot:%d PDU[%d] - unknown pdu type:%d\n", NFAPI_SFNSLOT2DEC(req->SFN, req->Slot), i, req->ul_dci_pdu_list[i].PDUType);
    }
  }

  return 0;
}
*/


int pnf_phy_hi_dci0_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_hi_dci0_request_t *req)
{

    (void)proc;
    (void)pnf_p7;
    (void)req;

    return 0;
}
/*
int pnf_phy_hi_dci0_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_hi_dci0_request_t *req) {
  if (req->hi_dci0_request_body.number_of_dci == 0 && req->hi_dci0_request_body.number_of_hi == 0)
    LOG_D(PHY,"[PNF] HI_DCI0_REQUEST SFN/SF:%05d dci:%d hi:%d\n", NFAPI_SFNSF2DEC(req->sfn_sf), req->hi_dci0_request_body.number_of_dci, req->hi_dci0_request_body.number_of_hi);

  //phy_info* phy = (phy_info*)(pnf_p7->user_data);
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  if (proc ==NULL) 
    proc = &eNB->proc.L1_proc;

  for (int i=0; i<req->hi_dci0_request_body.number_of_dci + req->hi_dci0_request_body.number_of_hi; i++) {
    //LOG_D(PHY,"[PNF] HI_DCI0_REQ sfn_sf:%d PDU[%d]\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);
    if (req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE) {
      //LOG_D(PHY,"[PNF] HI_DCI0_REQ sfn_sf:%d PDU[%d] - NFAPI_HI_DCI0_DCI_PDU_TYPE\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);
      nfapi_hi_dci0_request_pdu_t *hi_dci0_req_pdu = &req->hi_dci0_request_body.hi_dci0_pdu_list[i];
      handle_nfapi_hi_dci0_dci_pdu(eNB,NFAPI_SFNSF2SFN(req->sfn_sf),NFAPI_SFNSF2SF(req->sfn_sf),proc,hi_dci0_req_pdu);
      eNB->pdcch_vars[NFAPI_SFNSF2SF(req->sfn_sf)&1].num_dci++;
    } else if (req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type == NFAPI_HI_DCI0_HI_PDU_TYPE) {
      LOG_D(PHY,"[PNF] HI_DCI0_REQ sfn_sf:%d PDU[%d] - NFAPI_HI_DCI0_HI_PDU_TYPE\n", NFAPI_SFNSF2DEC(req->sfn_sf), i);
      nfapi_hi_dci0_request_pdu_t *hi_dci0_req_pdu = &req->hi_dci0_request_body.hi_dci0_pdu_list[i];
      handle_nfapi_hi_dci0_hi_pdu(eNB, NFAPI_SFNSF2SFN(req->sfn_sf),NFAPI_SFNSF2SF(req->sfn_sf), proc, hi_dci0_req_pdu);
    } else {
      LOG_E(PHY,"[PNF] HI_DCI0_REQ sfn_sf:%d PDU[%d] - unknown pdu type:%d\n", NFAPI_SFNSF2DEC(req->sfn_sf), i, req->hi_dci0_request_body.hi_dci0_pdu_list[i].pdu_type);
    }
  }

  return 0;
}
*/

/*
int pnf_phy_dl_tti_req(gNB_L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_nr_dl_tti_request_t *req) {
  if (RC.ru == 0) {
    return -1;
  }

  if (RC.gNB == 0) {
    return -2;
  }

  if (RC.gNB[0] == 0) {
    return -3;
  }

  if (sync_var != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() Main system not up - is this a dummy subframe?\n", __FUNCTION__);
    return -4;
  }

  int sfn = req->SFN;
  int slot =  req->Slot;
  struct PHY_VARS_gNB_s *gNB = RC.gNB[0];
  if (proc==NULL)
     proc = &gNB->proc.L1_proc;
  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdu_list = req->dl_tti_request_body.dl_tti_pdu_list;

    //if (req->dl_tti_request_body.nPDUs)
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() TX:%d/%d RX:%d/%d; sfn:%d, slot:%d, nGroup:%u, nPDUs: %u, nUE: %u, PduIdx: %u,\n",
    //             __FUNCTION__, proc->frame_tx, proc->slot_tx, proc->frame_rx, proc->slot_rx, // TODO: change subframes to slot
    //             req->SFN,
    //             req->Slot,
    //             req->dl_tti_request_body.nGroup,
    //             req->dl_tti_request_body.nPDUs,
    //             req->dl_tti_request_body.nUe,
    //             req->dl_tti_request_body.PduIdx);

  for (int i=0; i<req->dl_tti_request_body.nPDUs; i++) {
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() sfn/sf:%d PDU[%d] size:%d pdcch_vars->num_dci:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size,pdcch_vars->num_dci);

    if (dl_tti_pdu_list[i].PDUType == NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE) {
      nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdu=&dl_tti_pdu_list[i];
      handle_nfapi_nr_pdcch_pdu(gNB, sfn, slot, &dl_tti_pdu->pdcch_pdu);
    } 
    else if (dl_tti_pdu_list[i].PDUType == NFAPI_NR_DL_TTI_SSB_PDU_TYPE) {
      //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() PDU:%d BCH: pdu_index:%u pdu_length:%d sdu_length:%d BCH_SDU:%x,%x,%x\n", __FUNCTION__, i, pdu_index, bch_pdu->bch_pdu_rel8.length, tx_request_pdu[sfn][sf][pdu_index]->segments[0].segment_length, sdu[0], sdu[1], sdu[2]);
      handle_nr_nfapi_ssb_pdu(gNB, sfn, slot, &dl_tti_pdu_list[i]);
      gNB->pbch_configured=1;
    } 
    else if (dl_tti_pdu_list[i].PDUType == NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE) {
      nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu = &dl_tti_pdu_list[i].pdsch_pdu;
      nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15_pdu = &pdsch_pdu->pdsch_pdu_rel15;     
      nfapi_nr_pdu_t *tx_data = tx_data_request[sfn][slot][rel15_pdu->pduIndex];

      if (tx_data != NULL) {
        int UE_id = find_nr_dlsch(rel15_pdu->rnti,gNB,SEARCH_EXIST_OR_FREE);
        AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
        AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
        NR_gNB_DLSCH_t *dlsch0 = gNB->dlsch[UE_id][0];
        int harq_pid = dlsch0->harq_ids[sfn%2][slot];

        if(harq_pid >= dlsch0->Mdlharq) {
          LOG_E(PHY,"pnf_phy_dl_config_req illegal harq_pid %d\n", harq_pid);
          return(-1);
        }

        uint8_t *dlsch_sdu = nr_tx_pdus[UE_id][harq_pid];
        memcpy(dlsch_sdu, tx_data->TLVs[0].value.direct,tx_data->PDU_length);
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() DLSCH:pdu_index:%d handle_nfapi_dlsch_pdu(eNB, proc_rxtx, dlsch_pdu, transport_blocks:%d sdu:%p) eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols:%d\n", __FUNCTION__, rel8_pdu->pdu_index, rel8_pdu->transport_blocks, dlsch_sdu, eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols);
        handle_nr_nfapi_pdsch_pdu(gNB, sfn, slot,pdsch_pdu, dlsch_sdu);
      } 
      else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() DLSCH NULL TX PDU SFN/SF:%d PDU_INDEX:%d\n", __FUNCTION__, NFAPI_SFNSLOT2DEC(sfn,slot), rel15_pdu->pduIndex);     
      }
    }
    else if (dl_tti_pdu_list[i].PDUType == NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE) {
      nfapi_nr_dl_tti_csi_rs_pdu *csi_rs_pdu = &dl_tti_pdu_list[i].csi_rs_pdu;
      handle_nfapi_nr_csirs_pdu(gNB, sfn, slot, csi_rs_pdu);
    }
    else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() UNKNOWN:%d\n", __FUNCTION__, dl_tti_pdu_list[i].PDUType);
    }
  }

  if(req->vendor_extension)
    free(req->vendor_extension);

  return 0;
}
*/

/*
int pnf_phy_dl_config_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_dl_config_request_t *req) {

  if (RC.eNB == 0) {
    return -2;
  }

  if (RC.eNB[0][0] == 0) {
    return -3;
  }

  if (sync_var != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() Main system not up - is this a dummy subframe?\n", __FUNCTION__);
    return -4;
  }

  int sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
  int sf = NFAPI_SFNSF2SF(req->sfn_sf);
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  if (proc==NULL)
     proc = &eNB->proc.L1_proc;
  nfapi_dl_config_request_pdu_t *dl_config_pdu_list = req->dl_config_request_body.dl_config_pdu_list;
  LTE_eNB_PDCCH *pdcch_vars = &eNB->pdcch_vars[sf&1];
  pdcch_vars->num_pdcch_symbols = req->dl_config_request_body.number_pdcch_ofdm_symbols;
  pdcch_vars->num_dci = 0;

  if (req->dl_config_request_body.number_dci ||
      req->dl_config_request_body.number_pdu ||
      req->dl_config_request_body.number_pdsch_rnti)
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() TX:%d/%d RX:%d/%d sfn_sf:%d pdcch:%u dl_cfg[dci:%u pdus:%d pdsch_rnti:%d] pcfich:%u\n",
                __FUNCTION__, proc->frame_tx, proc->subframe_tx, proc->frame_rx, proc->subframe_rx,
                NFAPI_SFNSF2DEC(req->sfn_sf),
                req->dl_config_request_body.number_pdcch_ofdm_symbols,
                req->dl_config_request_body.number_dci,
                req->dl_config_request_body.number_pdu,
                req->dl_config_request_body.number_pdsch_rnti,
                req->dl_config_request_body.transmission_power_pcfich);

  for (int i=0; i<req->dl_config_request_body.number_pdu; i++) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() sfn/sf:%d PDU[%d] size:%d pdcch_vars->num_dci:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), i, dl_config_pdu_list[i].pdu_size,pdcch_vars->num_dci);

    if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE) {
      handle_nfapi_dci_dl_pdu(eNB,NFAPI_SFNSF2SFN(req->sfn_sf),NFAPI_SFNSF2SF(req->sfn_sf),proc,&dl_config_pdu_list[i]);
      pdcch_vars->num_dci++; // Is actually number of DCI PDUs
      NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() pdcch_vars->num_dci:%d\n", __FUNCTION__, pdcch_vars->num_dci);
    } else if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_BCH_PDU_TYPE) {
      nfapi_dl_config_bch_pdu *bch_pdu = &dl_config_pdu_list[i].bch_pdu;
      uint16_t pdu_index = bch_pdu->bch_pdu_rel8.pdu_index;

      if (tx_request_pdu[sfn][sf][pdu_index] != NULL) {
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() PDU:%d BCH: pdu_index:%u pdu_length:%d sdu_length:%d BCH_SDU:%x,%x,%x\n", __FUNCTION__, i, pdu_index, bch_pdu->bch_pdu_rel8.length, tx_request_pdu[sfn][sf][pdu_index]->segments[0].segment_length, sdu[0], sdu[1], sdu[2]);
        handle_nfapi_bch_pdu(eNB, proc, &dl_config_pdu_list[i], tx_request_pdu[sfn][sf][pdu_index]->segments[0].segment_data);
        eNB->pbch_configured=1;
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() BCH NULL TX PDU SFN/SF:%d PDU_INDEX:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), pdu_index);
      }
    } else if (dl_config_pdu_list[i].pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE) {
      nfapi_dl_config_dlsch_pdu *dlsch_pdu = &dl_config_pdu_list[i].dlsch_pdu;
      nfapi_dl_config_dlsch_pdu_rel8_t *rel8_pdu = &dlsch_pdu->dlsch_pdu_rel8;
      nfapi_tx_request_pdu_t *tx_pdu = tx_request_pdu[sfn][sf][rel8_pdu->pdu_index];

      if (tx_pdu != NULL) {
        int UE_id = find_dlsch(rel8_pdu->rnti,eNB,SEARCH_EXIST_OR_FREE);
        AssertFatal(UE_id!=-1,"no free or exiting dlsch_context\n");
        AssertFatal(UE_id<NUMBER_OF_UE_MAX,"returned UE_id %d >= %d(NUMBER_OF_UE_MAX)\n",UE_id,NUMBER_OF_UE_MAX);
        LTE_eNB_DLSCH_t *dlsch0 = eNB->dlsch[UE_id][0];
        //LTE_eNB_DLSCH_t *dlsch1 = eNB->dlsch[UE_id][1];
        int harq_pid = dlsch0->harq_ids[sfn%2][sf];

        if(harq_pid >= dlsch0->Mdlharq) {
          LOG_E(PHY,"pnf_phy_dl_config_req illegal harq_pid %d\n", harq_pid);
          return(-1);
        }

        uint8_t *dlsch_sdu = tx_pdus[UE_id][harq_pid];
        memcpy(dlsch_sdu, tx_pdu->segments[0].segment_data, tx_pdu->segments[0].segment_length);
        //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() DLSCH:pdu_index:%d handle_nfapi_dlsch_pdu(eNB, proc_rxtx, dlsch_pdu, transport_blocks:%d sdu:%p) eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols:%d\n", __FUNCTION__, rel8_pdu->pdu_index, rel8_pdu->transport_blocks, dlsch_sdu, eNB->pdcch_vars[proc->subframe_tx & 1].num_pdcch_symbols);
        handle_nfapi_dlsch_pdu( eNB, sfn,sf, proc, &dl_config_pdu_list[i], rel8_pdu->transport_blocks-1, dlsch_sdu);
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() DLSCH NULL TX PDU SFN/SF:%d PDU_INDEX:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), rel8_pdu->pdu_index);
      }
    } else {
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() UNKNOWN:%d\n", __FUNCTION__, dl_config_pdu_list[i].pdu_type);
    }
  }

  if(req->vendor_extension)
    free(req->vendor_extension);

  return 0;
}
*/

int pnf_phy_tx_data_req(nfapi_pnf_p7_config_t *pnf_p7, nfapi_nr_tx_data_request_t *req) {
  uint16_t sfn = req->SFN;
  uint16_t slot = req->Slot;

  if (req->Number_of_PDUs == 0)
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "%s() SFN/SLOT:%d%d PDUs:%d", __FUNCTION__, sfn, slot, req->Number_of_PDUs);

  //if (req->pdu_list[0].TLVs->tag ==  NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST) {
    for (int i=0; i<req->Number_of_PDUs; i++) {
      // LOG_D(PHY,"%s() SFN/SF:%d%d number_of_pdus:%d [PDU:%d] pdu_length:%d pdu_index:%d num_segments:%d\n",
      //       __FUNCTION__,
      //       sfn, sf,
      //       req->tx_request_body.number_of_pdus,
      //       i,
      //       req->tx_request_body.tx_pdu_list[i].pdu_length,
      //       req->tx_request_body.tx_pdu_list[i].pdu_index,
      //       req->tx_request_body.tx_pdu_list[i].num_segments
      //      );
      // tx_request_pdu[sfn][sf][i] = &req->tx_request_body.tx_pdu_list[i];
      tx_data_request[sfn][slot][i] = &req->pdu_list[i];
    }
  //}

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



int pnf_phy_ul_tti_req(gNB_L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_nr_ul_tti_request_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG,"[PNF] UL_TTI_REQ recvd, writing into structs, SFN/slot:%d.%d pdu:%d \n",
                req->SFN,req->Slot,
                req->n_pdus
               );

/*
  if (RC.ru == 0) {
    return -1;
  }

  if (RC.gNB == 0) {
    return -2;
  }

  if (RC.gNB[0] == 0) {
    return -3;
  }

  if (sync_var != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() Main system not up - is this a dummy slot?\n", __FUNCTION__);
    return -4;
  }

  uint16_t curr_sfn = req->SFN;
  uint16_t curr_slot = req->Slot;
  struct PHY_VARS_gNB_s *gNB = RC.gNB[0];

  if (proc==NULL)
     proc = &gNB->proc.L1_proc;
*/

  nfapi_nr_ul_tti_request_number_of_pdus_t *ul_tti_pdu_list = req->pdus_list;

  for (int i=0; i< req->n_pdus; i++) {
    switch (ul_tti_pdu_list[i].pdu_type) {
      case NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE:
        //LOG_D(PHY,"frame %d, slot %d, Got NFAPI_NR_UL_TTI_PUSCH_PDU_TYPE for %d.%d\n", frame, slot, UL_tti_req->SFN, UL_tti_req->Slot);
        //nr_fill_ulsch(gNB,curr_sfn, curr_slot, &ul_tti_pdu_list[i].pusch_pdu);
        break;
      case NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE:
        //LOG_D(PHY,"frame %d, slot %d, Got NFAPI_NR_UL_TTI_PUCCH_PDU_TYPE for %d.%d\n", frame, slot, UL_tti_req->SFN, UL_tti_req->Slot);
        //nr_fill_pucch(gNB,curr_sfn, curr_slot, &ul_tti_pdu_list[i].pucch_pdu);
        break;
      case NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE:
        //LOG_D(PHY,"frame %d, slot %d, Got NFAPI_NR_UL_TTI_PRACH_PDU_TYPE for %d.%d\n", frame, slot, UL_tti_req->SFN, UL_tti_req->Slot);
        //nr_fill_prach(gNB, curr_sfn, curr_slot, &ul_tti_pdu_list[i].prach_pdu);

        //if (gNB->RU_list[0]->if_south == LOCAL_RF)
          //nr_fill_prach_ru(gNB->RU_list[0], curr_sfn, curr_slot, &ul_tti_pdu_list[i].prach_pdu);

        break;
      default:
      NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() PDU:%i UNKNOWN type :%d\n", __FUNCTION__, i, ul_tti_pdu_list[i].pdu_type);
      break;

    }
    // //LOG_D(PHY, "%s() sfn/sf:%d PDU[%d] size:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(req->sfn_sf), i, ul_config_pdu_list[i].pdu_size);
    // if (
    //   ul_tti_pdu_list[i].pdu_type == NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE ||
    //   ul_tti_pdu_list[i].pdu_type == NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE ||
    //   ul_tti_pdu_list[i].pdu_type == NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE ||
    //   ul_tti_pdu_list[i].pdu_type == NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE
    // ) {
    //   //LOG_D(PHY, "%s() handle_nfapi_ul_pdu() for PDU:%d\n", __FUNCTION__, i);
    //   // handle_nfapi_ul_pdu(eNB,proc,&ul_config_pdu_list[i],curr_sfn,curr_sf,req->ul_config_request_body.srs_present);
      
    //   // TODO: dont have an NR function for this, also srs_present flag not there
      
    // } else {
    //   NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s() PDU:%i UNKNOWN type :%d\n", __FUNCTION__, i, ul_tti_pdu_list[i].pdu_type);
    // }
  }

  return 0;
}

int pnf_phy_ul_config_req(L1_rxtx_proc_t *proc, nfapi_pnf_p7_config_t *pnf_p7, nfapi_ul_config_request_t *req)
{

    (void)proc;
    (void)pnf_p7;

/*
//The following exists in nr oai code
  if (RC.eNB == 0) {
    return -2;
  }

  if (RC.eNB[0][0] == 0) {
    return -3;
  }

  if (sync_var != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "%s() Main system not up - is this a dummy subframe?\n", __FUNCTION__);
    return -4;
  }

  uint16_t curr_sfn = NFAPI_SFNSF2SFN(req->sfn_sf);
  uint16_t curr_sf = NFAPI_SFNSF2SF(req->sfn_sf);
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  if (proc==NULL)
     proc = &eNB->proc.L1_proc;
*/

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
            //The following exists in nr OAI code.
      	    ////LOG_D(PHY, "%s() handle_nfapi_ul_pdu() for PDU:%d\n", __FUNCTION__, i);
      	    //handle_nfapi_ul_pdu(eNB,proc,&ul_config_pdu_list[i],curr_sfn,curr_sf,req->ul_config_request_body.srs_present);
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

/*
// No in Lte code
int pnf_phy_vendor_ext(nfapi_pnf_p7_config_t *config, nfapi_p7_message_header_t *msg) {
  if(msg->message_id == P7_VENDOR_EXT_REQ) {
    //vendor_ext_p7_req* req = (vendor_ext_p7_req*)msg;
    //NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] vendor request (1:%d 2:%d)\n", req->dummy1, req->dummy2);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_WARN, "[PNF] unknown vendor ext\n");
  }

  return 0;
}

int pnf_phy_pack_p7_vendor_extension(nfapi_p7_message_header_t *header, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *codex) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P7_VENDOR_EXT_IND) {
    vendor_ext_p7_ind *ind = (vendor_ext_p7_ind *)(header);

    if(!push16(ind->error_code, ppWritePackedMsg, end))
      return 0;

    return 1;
  }

  return -1;
}

int pnf_phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t *header, uint8_t **ppReadPackedMessage, uint8_t *end, nfapi_p7_codec_config_t *codec) {
  if(header->message_id == P7_VENDOR_EXT_REQ) {
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
    vendor_ext_p7_req *req = (vendor_ext_p7_req *)(header);

    if(!(pull16(ppReadPackedMessage, &req->dummy1, end) &&
         pull16(ppReadPackedMessage, &req->dummy2, end)))
      return 0;

    return 1;
  }

  return -1;
}

int pnf_phy_unpack_vendor_extension_tlv(nfapi_tl_t *tl, uint8_t **ppReadPackedMessage, uint8_t *end, void **ve, nfapi_p7_codec_config_t *config) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "pnf_phy_unpack_vendor_extension_tlv\n");
  switch(tl->tag) {
    case VENDOR_EXT_TLV_1_TAG:
      *ve = malloc(sizeof(vendor_ext_tlv_1));

      if(!pull32(ppReadPackedMessage, &((vendor_ext_tlv_1 *)(*ve))->dummy, end))
        return 0;

      return 1;
      break;
  }

  return -1;
}

int pnf_phy_pack_vendor_extention_tlv(void *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *config) {
  //NFAPI_TRACE(NFAPI_TRACE_DEBUG, "%s\n", __FUNCTION__);
  (void)ve;
  (void)ppWritePackedMsg;
  return -1;
}

int pnf_sim_unpack_vendor_extension_tlv(nfapi_tl_t *tl, uint8_t **ppReadPackedMessage, uint8_t *end, void **ve, nfapi_p4_p5_codec_config_t *config) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "pnf_sim_unpack_vendor_extension_tlv\n");
  switch(tl->tag) {
    case VENDOR_EXT_TLV_2_TAG:
      *ve = malloc(sizeof(vendor_ext_tlv_2));

      if(!pull32(ppReadPackedMessage, &((vendor_ext_tlv_2 *)(*ve))->dummy, end))
        return 0;

      return 1;
      break;
  }

  return -1;
}

int pnf_sim_pack_vendor_extention_tlv(void *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  //NFAPI_TRACE(NFAPI_TRACE_DEBUG, "%s\n", __FUNCTION__);
  (void)ve;
  (void)ppWritePackedMsg;

  return -1;
}
*/

nfapi_dl_config_request_t dummy_dl_config_req;
nfapi_tx_request_t dummy_tx_req;
nfapi_pnf_p7_subframe_buffer_t dummy_subframe;

int start_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy, nfapi_start_request_t *req)
{
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_START_REQ phy_id:%d\n", req->header.phy_id);
    pnf_info *pnf = (pnf_info *)(config->user_data);
    phy_info *phy_info = pnf->phys;
    nfapi_pnf_p7_config_t *p7_config = nfapi_pnf_p7_config_create();
    p7_config->phy_id = phy->phy_id;
    p7_config->remote_p7_port = phy_info->remote_port;
    p7_config->remote_p7_addr = phy_info->remote_addr;
    p7_config->local_p7_port = 32123; // DJP - good grief cannot seem to get the right answer phy_info->local_port;
    //p7_config->local_p7_port = phy_info->udp.rx_port;
    //DJP p7_config->local_p7_addr = (char*)phy_info->local_addr.c_str();
    p7_config->local_p7_addr = phy_info->local_addr;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] P7 remote:%s:%d local:%s:%d\n", p7_config->remote_p7_addr, p7_config->remote_p7_port,
           p7_config->local_p7_addr, p7_config->local_p7_port);
    p7_config->user_data = phy_info;
    p7_config->malloc = &pnf_allocate;
    p7_config->free = &pnf_deallocate;
    p7_config->codec_config.allocate = &pnf_allocate;
    p7_config->codec_config.deallocate = &pnf_deallocate;
    p7_config->dl_config_req = NULL;
    p7_config->hi_dci0_req = NULL;
    p7_config->tx_req = NULL;
    p7_config->ul_config_req = NULL;
    phy->user_data = p7_config;
    p7_config->subframe_buffer_size = phy_info->timing_window;
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "subframe_buffer_size configured using phy_info->timing_window:%d\n", phy_info->timing_window);

    if (phy_info->timing_info_mode & 0x1)
    {
        p7_config->timing_info_mode_periodic = 1;
        p7_config->timing_info_period = phy_info->timing_info_period;
    }

    if (phy_info->timing_info_mode & 0x2)
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
    dummy_subframe.tx_req = 0;//&dummy_tx_req;
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

    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] OAI eNB/RU configured\n");
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] About to call init_eNB_afterRU()\n");

    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sending PNF_START_RESP\n");
    nfapi_send_pnf_start_resp(config, p7_config->phy_id);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sending first P7 subframe ind\n");
    nfapi_pnf_p7_subframe_ind(p7_config, p7_config->phy_id, 0); // DJP - SFN_SF set to zero - correct???
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent first P7 subframe ind\n");

    return 0;
}

int nr_start_request(nfapi_pnf_config_t *config, nfapi_pnf_phy_config_t *phy,  nfapi_nr_start_request_scf_t *req) {
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Received NFAPI_START_REQ phy_id:%d\n", req->header.phy_id);
  pnf_info *pnf = (pnf_info *)(config->user_data);
  phy_info *phy_info = pnf->phys;
  nfapi_pnf_p7_config_t *p7_config = nfapi_pnf_p7_config_create();
  p7_config->phy_id = phy->phy_id;
  p7_config->remote_p7_port = phy_info->remote_port;
  p7_config->remote_p7_addr = phy_info->remote_addr;
  // TODO: remove this hardcoded port
  p7_config->local_p7_port = 50610; // DJP - good grief cannot seem to get the right answer phy_info->local_port;
  //DJP p7_config->local_p7_addr = (char*)phy_info->local_addr.c_str();
  p7_config->local_p7_addr = phy_info->local_addr;
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] P7 remote:%s:%d local:%s:%d\n", p7_config->remote_p7_addr, p7_config->remote_p7_port, p7_config->local_p7_addr, p7_config->local_p7_port);
  p7_config->user_data = phy_info;
  //p7_config->user_data = phy_info;
  p7_config->malloc = &pnf_allocate;
  p7_config->free = &pnf_deallocate;
  p7_config->codec_config.allocate = &pnf_allocate;
  p7_config->codec_config.deallocate = &pnf_deallocate;

  //NR
  p7_config->dl_tti_req_fn  = NULL;
  p7_config->ul_tti_req_fn  = NULL;
  p7_config->ul_dci_req_fn  = NULL;
  p7_config->tx_data_req_fn = NULL;

  // LTE
  p7_config->dl_config_req = NULL;
  p7_config->hi_dci0_req = NULL;
  p7_config->tx_req = NULL;
  p7_config->ul_config_req = NULL;
  phy->user_data = p7_config;
  p7_config->subframe_buffer_size = phy_info->timing_window;
  p7_config->slot_buffer_size = phy_info->timing_window; // TODO: check if correct for NR
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "subframe_buffer_size configured using phy_info->timing_window:%d\n", phy_info->timing_window);

  if (phy_info->timing_info_mode & 0x1)
  {
      p7_config->timing_info_mode_periodic = 1;
      p7_config->timing_info_period = phy_info->timing_info_period;
  }

  if (phy_info->timing_info_mode & 0x2)
  {
      p7_config->timing_info_mode_aperiodic = 1;
  }
  p7_config->lbt_dl_config_req = &pnf_phy_lbt_dl_config_req;
  memset(&dummy_dl_config_req, 0, sizeof(dummy_dl_config_req));
  dummy_dl_config_req.dl_config_request_body.tl.tag=NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
  dummy_dl_config_req.dl_config_request_body.number_pdcch_ofdm_symbols=1;
  dummy_dl_config_req.dl_config_request_body.number_dci=0;
  dummy_dl_config_req.dl_config_request_body.number_pdu=0;
  dummy_dl_config_req.dl_config_request_body.number_pdsch_rnti=0;
  dummy_dl_config_req.dl_config_request_body.transmission_power_pcfich=6000;
  dummy_dl_config_req.dl_config_request_body.dl_config_pdu_list=0;
  memset(&dummy_tx_req, 0, sizeof(dummy_tx_req));
  dummy_tx_req.tx_request_body.number_of_pdus=0;
  dummy_tx_req.tx_request_body.tl.tag=NFAPI_TX_REQUEST_BODY_TAG;
  dummy_subframe.dl_config_req = &dummy_dl_config_req;
  dummy_subframe.tx_req = 0;//&dummy_tx_req;
  dummy_subframe.ul_config_req=0;
  dummy_subframe.hi_dci0_req=0;
  dummy_subframe.lbt_dl_config_req=0;
  p7_config->dummy_subframe = dummy_subframe;
  /*
  p7_config->vendor_ext = &pnf_phy_vendor_ext;
  p7_config->allocate_p7_vendor_ext = &pnf_phy_allocate_p7_vendor_ext;
  p7_config->deallocate_p7_vendor_ext = &pnf_phy_deallocate_p7_vendor_ext;
  p7_config->codec_config.unpack_p7_vendor_extension = &pnf_phy_unpack_p7_vendor_extension;
  p7_config->codec_config.pack_p7_vendor_extension = &pnf_phy_pack_p7_vendor_extension;
  p7_config->codec_config.unpack_vendor_extension_tlv = &pnf_phy_unpack_vendor_extension_tlv;
  p7_config->codec_config.pack_vendor_extension_tlv = &pnf_phy_pack_vendor_extention_tlv;
  */
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Creating P7 thread\n");
  pthread_t p7_thread;
  pthread_create(&p7_thread, NULL, &pnf_nr_p7_thread_start, p7_config);
  //((pnf_phy_user_data_t*)(phy_info->fapi->user_data))->p7_config = p7_config;
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Calling l1_north_init_eNB() %s\n", __FUNCTION__);
  //l1_north_init_gNB();

  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] DJP - HACK - Set p7_config global ready for subframe ind\n");
  p7_nr_config_g = p7_config;

  /*
  // Need to wait for main thread to create RU structures
  while(config_sync_var<0) {
    usleep(5000000);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] waiting for OAI to be configured (eNB/RU)\n");
  }

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] OAI eNB/RU configured\n");
  //NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] About to call phy_init_RU() for RC.ru[0]:%p\n", RC.ru[0]);
  //phy_init_RU(RC.ru[0]);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] About to call init_eNB_afterRU()\n");

  if (NFAPI_MODE!=NFAPI_UE_STUB_PNF) {
    init_eNB_afterRU();
  }

  // Signal to main thread that it can carry on - otherwise RU will startup too quickly and it is not initialised
  {
    pthread_mutex_lock(&nfapi_sync_mutex);
    nfapi_sync_var=0;
    pthread_cond_broadcast(&nfapi_sync_cond);
    pthread_mutex_unlock(&nfapi_sync_mutex);
  }

  while(sync_var<0) {
    usleep(5000000);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] waiting for OAI to be started\n");
  }
  */

  // Signal to main thread that it can carry on - otherwise RU will startup too quickly and it is not initialised

  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sending PNF_START_RESP\n");
  nfapi_nr_send_pnf_start_resp(config, p7_config->phy_id);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sending first P7 slot indication\n");
  nfapi_pnf_p7_slot_ind(p7_config, p7_config->phy_id, 0, 0);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent first P7 slot ind\n");

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

/*
int vendor_ext(nfapi_pnf_config_t *config, nfapi_p4_p5_message_header_t *msg) {
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] P5 %s %p\n", __FUNCTION__, msg);

  switch(msg->message_id) {
    case P5_VENDOR_EXT_REQ: {
      vendor_ext_p5_req *req = (vendor_ext_p5_req *)msg;
      NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] P5 Vendor Ext Req (%d %d)\n", req->dummy1, req->dummy2);
      // send back the P5_VENDOR_EXT_RSP
      vendor_ext_p5_rsp rsp;
      memset(&rsp, 0, sizeof(rsp));
      rsp.header.message_id = P5_VENDOR_EXT_RSP;
      rsp.error_code = NFAPI_MSG_OK;
      nfapi_pnf_vendor_extension(config, &rsp.header, sizeof(vendor_ext_p5_rsp));
    }
    break;
  }

  return 0;
}

nfapi_p4_p5_message_header_t *pnf_sim_allocate_p4_p5_vendor_ext(uint16_t message_id, uint16_t *msg_size) {
  if(message_id == P5_VENDOR_EXT_REQ) {
    (*msg_size) = sizeof(vendor_ext_p5_req);
    return (nfapi_p4_p5_message_header_t *)malloc(sizeof(vendor_ext_p5_req));
  }

  return 0;
}

void pnf_sim_deallocate_p4_p5_vendor_ext(nfapi_p4_p5_message_header_t *header) {
  free(header);
}

int pnf_sim_pack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t *header, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p4_p5_codec_config_t *config) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P5_VENDOR_EXT_RSP) {
    vendor_ext_p5_rsp *rsp = (vendor_ext_p5_rsp *)(header);
    return (!push16(rsp->error_code, ppWritePackedMsg, end));
  }

  return 0;
}

int pnf_sim_unpack_p4_p5_vendor_extension(nfapi_p4_p5_message_header_t *header, uint8_t **ppReadPackedMessage, uint8_t *end, nfapi_p4_p5_codec_config_t *codec) {
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if(header->message_id == P5_VENDOR_EXT_REQ) {
    vendor_ext_p5_req *req = (vendor_ext_p5_req *)(header);
    return (!(pull16(ppReadPackedMessage, &req->dummy1, end) &&
              pull16(ppReadPackedMessage, &req->dummy2, end)));
    //NFAPI_TRACE(NFAPI_TRACE_INFO, "%s (%d %d)\n", __FUNCTION__, req->dummy1, req->dummy2);
  }

  return 0;
}
*/

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

/*
void *pnf_start_thread(void *ptr) {
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] IN PNF NFAPI start thread %s\n", __FUNCTION__);
  nfapi_pnf_config_t *config = (nfapi_pnf_config_t *)ptr;
  struct sched_param sp;
  sp.sched_priority = 20;
  pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp);
  nfapi_pnf_start(config);
  return (void *)0;
}
*/

void *pnf_nr_start_thread(void *ptr) {
  NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] IN PNF NFAPI start thread %s\n", __FUNCTION__);
  nfapi_pnf_config_t *config = (nfapi_pnf_config_t *)ptr;
  struct sched_param sp;
  sp.sched_priority = 20;
  pthread_setschedparam(pthread_self(),SCHED_FIFO,&sp);
  nfapi_nr_pnf_start(config);
  return (void *)0;
}

void configure_nr_nfapi_pnf(const char *vnf_ip_addr, int vnf_p5_port, const char *pnf_ip_addr, int pnf_p7_port, int vnf_p7_port) {
  nfapi_pnf_config_t *config = nfapi_pnf_config_create();
  config->vnf_ip_addr = vnf_ip_addr;
  config->vnf_p5_port = vnf_p5_port;
  pnf_nr.phys[0].udp.enabled = 1;
  pnf_nr.phys[0].udp.rx_port = pnf_p7_port;
  pnf_nr.phys[0].udp.tx_port = vnf_p7_port;
  strcpy(pnf_nr.phys[0].udp.tx_addr, vnf_ip_addr);
  strcpy(pnf_nr.phys[0].local_addr, pnf_ip_addr);
  NFAPI_TRACE(NFAPI_TRACE_DEBUG,
         "%s() VNF:%s:%d PNF_PHY[addr:%s UDP:tx_addr:%s:%d rx:%d]\n",
         __FUNCTION__,config->vnf_ip_addr, config->vnf_p5_port,
         pnf_nr.phys[0].local_addr,
         pnf_nr.phys[0].udp.tx_addr, pnf_nr.phys[0].udp.tx_port,
         pnf_nr.phys[0].udp.rx_port);
  config->cell_search_req = &cell_search_request;

  //config->pnf_nr_param_req = &pnf_nr_param_request;
  config->pnf_nr_param_req = &pnf_nr_param_request;
  config->pnf_nr_config_req = &pnf_nr_config_request;
  config->pnf_nr_start_req = &pnf_nr_start_request;
  config->pnf_stop_req = &pnf_stop_request;
  config->nr_param_req = &nr_param_request;
  config->nr_config_req = &nr_config_request;
  config->nr_start_req = &nr_start_request;
  config->measurement_req = &measurement_request;
  config->rssi_req = &rssi_request;
  config->broadcast_detect_req = &broadcast_detect_request;
  config->system_information_schedule_req = &system_information_schedule_request;
  config->system_information_req = &system_information_request;
  config->nmm_stop_req = &nmm_stop_request;
  //config->vendor_ext = &vendor_ext;
  config->user_data = &pnf_nr;
  // To allow custom vendor extentions to be added to nfapi
  //config->codec_config.unpack_vendor_extension_tlv = &pnf_sim_unpack_vendor_extension_tlv;
  //config->codec_config.pack_vendor_extension_tlv = &pnf_sim_pack_vendor_extention_tlv;
  //config->allocate_p4_p5_vendor_ext = &pnf_sim_allocate_p4_p5_vendor_ext;
  //config->deallocate_p4_p5_vendor_ext = &pnf_sim_deallocate_p4_p5_vendor_ext;
  //config->codec_config.unpack_p4_p5_vendor_extension = &pnf_sim_unpack_p4_p5_vendor_extension;
  //config->codec_config.pack_p4_p5_vendor_extension = &pnf_sim_pack_p4_p5_vendor_extension;
  //NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Creating PNF NFAPI start thread %s\n", __FUNCTION__);
  pthread_create(&pnf_nr_start_pthread, NULL, &pnf_nr_start_thread, config);
  pthread_setname_np(pnf_nr_start_pthread, "NFAPI_PNF");

}

void configure_nfapi_pnf(const char *vnf_ip_addr, int vnf_p5_port, const char *pnf_ip_addr, int pnf_p7_port,
                         int vnf_p7_port)
{
    nfapi_pnf_config_t *config = nfapi_pnf_config_create();
    config->vnf_ip_addr = vnf_ip_addr;
    config->vnf_p5_port = vnf_p5_port;
    pnf.phys[0].udp.enabled = 1;
    pnf.phys[0].udp.rx_port = pnf_p7_port;
    pnf.phys[0].udp.tx_port = vnf_p7_port;
    strcpy(pnf.phys[0].udp.tx_addr, vnf_ip_addr);
    strcpy(pnf.phys[0].local_addr, pnf_ip_addr);
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "%s() VNF:%s:%d PNF_PHY[addr:%s UDP:tx_addr:%s:%d rx:%d]\n",
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
    // The followins are removed in LTE
    /*
    config->codec_config.unpack_vendor_extension_tlv = &pnf_sim_unpack_vendor_extension_tlv;
    config->codec_config.pack_vendor_extension_tlv = &pnf_sim_pack_vendor_extention_tlv;
    config->allocate_p4_p5_vendor_ext = &pnf_sim_allocate_p4_p5_vendor_ext;
    config->deallocate_p4_p5_vendor_ext = &pnf_sim_deallocate_p4_p5_vendor_ext;
    config->codec_config.unpack_p4_p5_vendor_extension = &pnf_sim_unpack_p4_p5_vendor_extension;
    config->codec_config.pack_p4_p5_vendor_extension = &pnf_sim_pack_p4_p5_vendor_extension;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] Creating PNF NFAPI start thread %s\n", __FUNCTION__);
    pthread_create(&pnf_start_pthread, NULL, &pnf_start_thread, config);
    pthread_setname_np(pnf_start_pthread, "NFAPI_PNF");
    */
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
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] %ld.%ld (sfn:%u sf:%u) SFN/SF(TX):%u\n",
                        ts.tv_sec, ts.tv_nsec, sfn, sf, NFAPI_SFNSF2DEC(sfn_sf_tx));
        }

        int subframe_ret = nfapi_pnf_p7_subframe_ind(p7_config_g, p7_config_g->phy_id, sfn_sf_tx);

        if (subframe_ret)
        {
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] (frame:%u subframe:%u) SFN/SF(TX):%u - PROBLEM with pnf_p7_subframe_ind()\n",
                        sfn, sf, sfn_sf_tx);
        }
        else
        {

        }
    }
    else
    {
    }
}

void oai_slot_ind(uint16_t sfn, uint16_t slot)
{
    //TODO FIXME - HACK - DJP - using a global to bodge it in
    if (p7_nr_config_g != NULL)
    {
        uint16_t sfn_slot_tx = sfn<<6 | slot; 
        if ((sfn % 100 == 0) && slot == 0)
        {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] %ld.%ld (sfn:%u slot:%u) SFN/SLOT(TX):%u\n", 
                        ts.tv_sec, ts.tv_nsec, sfn, slot, NFAPI_SFNSLOT2DEC(sfn, slot));
        }

        nfapi_nr_slot_indication_scf_t ind;
        ind.sfn = sfn;
        ind.slot = slot;
        ind.header.phy_id = p7_nr_config_g->phy_id;
        ind.header.message_id = NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION;
        int slot_ret = nfapi_pnf_p7_nr_slot_ind(p7_nr_config_g, &ind);

        if (slot_ret < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_INFO, "[PNF] (frame:%u slot:%u) SFN/SLOT(TX):%u - PROBLEM with pnf_p7_slot_ind()\n", 
                        sfn, slot, sfn_slot_tx);
        }
        else
        {
            NFAPI_TRACE(NFAPI_TRACE_DEBUG, "[PNF] Sent slot indication from PNF to VNF in oai_slot_ind %4d.%2d\n", 
                        sfn, slot);
        }
    }
    else
    {
    }
}

// Queues of message_buffer_t pointers, one queue per UE
static queue_t msgs_from_ue[MAX_UES];
static queue_t msgs_from_nr_ue[MAX_UES];

void oai_subframe_init()
{
    for (int i = 0; i < num_ues; i++)
    {
        init_queue(&msgs_from_ue[i]);
    }
}

void oai_slot_init()
{
    for (int i = 0; i < num_ues; i++)
    {
        init_queue(&msgs_from_nr_ue[i]);
    }
}

static uint16_t get_message_id(message_buffer_t *msg)
{
    uint16_t phy_id = ~0;
    uint16_t message_id = ~0;
    uint8_t *in = msg->data;
    uint8_t *end = msg->data + msg->length;
    assert(end <= msg->data + sizeof(msg->data));
    pull16(&in, &phy_id, end);
    pull16(&in, &message_id, end);
    return message_id;
}

static bool get_sfnsf(message_buffer_t *msg, uint16_t *sfnsf)
{
    uint8_t *in = msg->data;
    uint8_t *end = msg->data + msg->length;
    assert(end <= msg->data + sizeof(msg->data));

    uint16_t phy_id;
    uint16_t message_id;
    if (!pull16(&in, &phy_id, end) ||
        !pull16(&in, &message_id, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve message_id");
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
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return false;
    }

    in = msg->data + sizeof(nfapi_p7_message_header_t);
    if (!pull16(&in, sfnsf, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve sfn_sf");
        return false;
    }
    return true;
}

static bool get_sfnslot(message_buffer_t *msg, uint16_t *sfnslot)
{
    uint8_t *in = msg->data;
    uint8_t *end = msg->data + msg->length;
    assert(end <= msg->data + sizeof(msg->data));

    uint16_t phy_id;
    uint16_t message_id;
    if (!pull16(&in, &phy_id, end) ||
        !pull16(&in, &message_id, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve message_id");
        return false;
    }

    switch (message_id)
    {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
        break;
    default:
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message_id is unknown %u", message_id);
        return false;
    }

    in = msg->data + sizeof(nfapi_p7_message_header_t);
    uint16_t sfn, slot;
    if (!pull16(&in, &sfn, end) ||
        !pull16(&in, &slot, end))
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "could not retrieve sfn and slot");
        return false;
    }
    *sfnslot = NFAPI_SFNSLOT2HEX(sfn, slot);
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
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Mismatch phy_id %u,%u message_id %u,%u m_seg_sequence %u,%u sfn_sf %u,%u (%s)",
                    agg_header->phy_id, ind_header->phy_id,
                    agg_header->message_id, ind_header->message_id,
                    agg_header->m_segment_sequence, ind_header->m_segment_sequence,
                    agg_sfn_sf, ind_sfn_sf, label);
    }
}

static void warn_if_different_slots(const char *label,
                              const nfapi_p7_message_header_t *agg_header,
                              const nfapi_p7_message_header_t *ind_header,
                              uint16_t agg_sfn,
                              uint16_t ind_sfn,
                              uint16_t agg_slot,
                              uint16_t ind_slot)
{
    if (agg_header->phy_id != ind_header->phy_id ||
        agg_header->message_id != ind_header->message_id ||
        agg_header->m_segment_sequence != ind_header->m_segment_sequence ||
        agg_sfn != ind_sfn ||
        agg_slot != ind_slot)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Mismatch phy_id %u,%u message_id %u,%u m_seg_sequence %u,%u sfn %u,%u slot %u,%u (%s)",
                    agg_header->phy_id, ind_header->phy_id,
                    agg_header->message_id, ind_header->message_id,
                    agg_header->m_segment_sequence, ind_header->m_segment_sequence,
                    agg_sfn, ind_sfn,
                    agg_slot, ind_slot, label);
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
        assert(msg->length <= sizeof(msg->data));

        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "rach unpack failed");
            continue;
        }
        ind.sfn_sf = sfn_sf_add(ind.sfn_sf, 5);
        oai_nfapi_rach_ind(&ind);
    }
}

/*
//3.4.11 rach_indication
//table 3-74
typedef struct
{
  uint8_t  preamble_index;
  uint16_t timing_advance;
  uint32_t preamble_pwr;

} nfapi_nr_prach_indication_preamble_t;

typedef struct{
  uint16_t phy_cell_id;
  uint8_t  symbol_index;
  uint8_t  slot_index;
  uint8_t  freq_index;
  uint8_t  avg_rssi;
  uint8_t  avg_snr;
  uint8_t  num_preamble;
  nfapi_nr_prach_indication_preamble_t* preamble_list;

}nfapi_nr_prach_indication_pdu_t;

typedef struct
{
  uint16_t sfn;
  uint16_t slot;
  uint8_t number_of_pdus;
  nfapi_nr_prach_indication_pdu_t* pdu_list;

} nfapi_nr_rach_indication_t;
*/

static void oai_slot_aggregate_rach_ind(slot_msgs_t *msgs)
{
    /* Currently we select the first RACH and ignore others.
       We could have various policies to select one RACH over
       the others (e.g., strongest signal) */
    assert(msgs->num_msgs > 0);
    for (int i = 0; i < msgs->num_msgs; ++i)
    {
        message_buffer_t *msg = msgs->msgs[i];
        nfapi_nr_rach_indication_t ind;
        assert(msg->length <= sizeof(msg->data));

        if (nfapi_nr_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR,  "nr rach unpack failed");
            continue;
        }
        oai_nfapi_nr_rach_indication(&ind);
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
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "crc indication unpack failed, msg[%zu]", n);
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
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
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
typedef struct
{
  uint32_t handle;
  uint16_t rnti;
  uint8_t  harq_id;
  uint8_t  tb_crc_status;
  uint16_t num_cb;//If CBG is not used this parameter can be set to zero. Otherwise the number of CBs in the TB. Value: 0->65535
  //! fixme
  uint8_t* cb_crc_status;//cb_crc_status[ceil(NumCb/8)];
  uint8_t  ul_cqi;
  uint16_t timing_advance;
  uint16_t rssi;

} nfapi_nr_crc_t;

typedef struct
{
  nfapi_p7_message_header_t header;
  uint16_t sfn;
  uint16_t slot;
  uint16_t number_crcs;
  nfapi_nr_crc_t* crc_list;

} nfapi_nr_crc_indication_t;
*/

static void oai_slot_aggregate_crc_ind(slot_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_nr_crc_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.crc_list = calloc(NFAPI_NR_CRC_IND_MAX_PDU, sizeof(nfapi_nr_crc_t));
    assert(agg.crc_list);

    size_t pduIndex = 0;
    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_nr_crc_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_nr_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "nr_crc indication unpack failed, msg[%zu]", n);
            free(agg.crc_list);
            return;
        }

        // Header, sfn, and slot assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different_slots(__func__, &agg.header, &ind.header, agg.sfn, ind.sfn, agg.slot, ind.slot);
        }
        agg.header = ind.header;
        agg.sfn = ind.sfn;
        agg.slot = ind.slot;
        agg.number_crcs += ind.number_crcs;

        for (size_t i = 0; i < ind.number_crcs; ++i)
        {
            if (pduIndex == NFAPI_NR_CRC_IND_MAX_PDU)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
                break;
            }
            agg.crc_list[pduIndex] = ind.crc_list[i];
            pduIndex++;
        }
        free(ind.crc_list);
        if (pduIndex == NFAPI_NR_CRC_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_nr_crc_indication(&agg);
    free(agg.crc_list);
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
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "Printing RX_IND fields");
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.message_id: %u", p->header.message_id);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.phy_id: %u", p->header.phy_id);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.message_id: %u", p->header.message_id);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.m_segment_sequence: %u", p->header.m_segment_sequence);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.checksum: %u", p->header.checksum);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "header.transmit_timestamp: %u", p->header.transmit_timestamp);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "sfn_sf: %u", p->sfn_sf);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "rx_indication_body.tl.tag: 0x%x", p->rx_indication_body.tl.tag);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "rx_indication_body.tl.length: %u", p->rx_indication_body.tl.length);
//   NFAPI_TRACE(NFAPI_TRACE_INFO, "rx_indication_body.number_of_pdus: %u", p->rx_indication_body.number_of_pdus);

//   nfapi_rx_indication_pdu_t *pdu = p->rx_indication_body.rx_pdu_list;
//   for (int i = 0; i < p->rx_indication_body.number_of_pdus; i++)
//   {
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_ue_information.tl.tag: 0x%x", i, pdu->rx_ue_information.tl.tag);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_ue_information.tl.length: %u", i, pdu->rx_ue_information.tl.length);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_ue_information.handle: %u", i, pdu->rx_ue_information.handle);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_ue_information.rnti: %u", i, pdu->rx_ue_information.rnti);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.tl.tag: 0x%x", i, pdu->rx_indication_rel8.tl.tag);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.tl.length: %u", i, pdu->rx_indication_rel8.tl.length);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.length: %u", i, pdu->rx_indication_rel8.length);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.offset: %u", i, pdu->rx_indication_rel8.offset);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.ul_cqi: %u", i, pdu->rx_indication_rel8.ul_cqi);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel8.timing_advance: %u", i, pdu->rx_indication_rel8.timing_advance);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel9.tl.tag: 0x%x", i, pdu->rx_indication_rel9.tl.tag);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel9.tl.length: %u", i, pdu->rx_indication_rel9.tl.length);
//     NFAPI_TRACE(NFAPI_TRACE_INFO, "pdu %d nfapi_rx_indication_rel9.timing_advance_r9: %u", i, pdu->rx_indication_rel9.timing_advance_r9);
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
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Something went very wrong");
            abort();
        }
        NFAPI_TRACE(NFAPI_TRACE_INFO, "We are aggregating %zu rx_ind's for SFN.SF %u.%u "
                    "trying to aggregate",
                    msgs->num_msgs, sfn_sf_val >> 4,
                    sfn_sf_val & 15);
    }

    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_rx_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "rx indication unpack failed, msg[%zu]", n);
            free(agg.rx_indication_body.rx_pdu_list);
            return;
        }

        if (ind.rx_indication_body.number_of_pdus == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "empty rx message");
            abort();
        }

        int rnti = ind.rx_indication_body.rx_pdu_list[0].rx_ue_information.rnti;
        bool found = false;
        for (int i = 0; i < agg.rx_indication_body.number_of_pdus; ++i)
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
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "two rx for single UE rnti: %x", rnti);
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
        NFAPI_TRACE(NFAPI_TRACE_INFO, "agg.tl.tag: %x agg.tl.length: %d ind.tl.tag: %x ind.tl.length: %u",
                    agg.rx_indication_body.tl.tag,agg.rx_indication_body.tl.length,
                    ind.rx_indication_body.tl.tag, ind.rx_indication_body.tl.length);
        agg.rx_indication_body.number_of_pdus += ind.rx_indication_body.number_of_pdus;

        for (size_t i = 0; i < ind.rx_indication_body.number_of_pdus; ++i)
        {
            if (pduIndex == NFAPI_RX_IND_MAX_PDU)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
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
typedef struct 
{
  uint32_t handle;
  uint16_t rnti;
  uint8_t  harq_id;
  uint16_t pdu_length;
  uint8_t  ul_cqi;
  uint16_t timing_advance;//Timing advance 𝑇𝐴 measured for the UE [TS 38.213, Section 4.2] NTA_new = NTA_old + (TA − 31) ⋅ 16 ⋅ 64⁄2μ Value: 0 → 63 0xffff should be set if this field is invalid
  uint16_t rssi;
  //variable ! fixme
  uint8_t *pdu; //MAC PDU

} nfapi_nr_rx_data_pdu_t;

typedef struct
{
  nfapi_p7_message_header_t header;
  uint16_t sfn;
  uint16_t slot;
  uint16_t number_of_pdus;
  nfapi_nr_rx_data_pdu_t *pdu_list; //changed from pointer to struct - gokul

} nfapi_nr_rx_data_indication_t;
*/

static void oai_slot_aggregate_rx_data_ind(slot_msgs_t *msgs)
{

    assert(msgs->num_msgs > 0);
    nfapi_nr_rx_data_indication_t agg;
    memset(&agg, 0, sizeof(agg));    //To Do Items : Need to check whether MAX_PDU is okay ?
    agg.pdu_list = calloc(NFAPI_NR_RX_DATA_IND_MAX_PDU, sizeof(nfapi_nr_rx_data_pdu_t));
    assert(agg.pdu_list);

    size_t pduIndex = 0;

    if (msgs->num_msgs > 1)
    {
        uint16_t sfn_slot_val;
        message_buffer_t *first_msg = msgs->msgs[0];
        if (!get_sfnslot(first_msg, &sfn_slot_val))
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Something went very wrong");
            abort();
        }
        NFAPI_TRACE(NFAPI_TRACE_INFO, "We are aggregating %zu rx_data_ind's for SFN.SLOT %u.%u "
                   "trying to aggregate",
                   msgs->num_msgs, sfn_slot_val >> 6,
                   sfn_slot_val & 0x3F);
    }

    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_nr_rx_data_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_nr_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "rx indication unpack failed, msg[%zu]", n);
            free(agg.pdu_list);
            return;
        }

        if (ind.number_of_pdus == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "empty rx message");
            abort();
        }

        int rnti = ind.pdu_list[0].rnti;
        bool found = false;
        for (int i = 0; i < agg.number_of_pdus; ++i)
        {
            int rnti_i = agg.pdu_list[i].rnti;
            if (rnti == rnti_i)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "two rx for single UE rnti: %x", rnti);
        }

        // Header, sfn, and slot assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different_slots(__func__, &agg.header, &ind.header, agg.sfn, ind.sfn, agg.slot, ind.slot);
        }
        agg.header = ind.header;
        agg.sfn = ind.sfn;
        agg.slot = ind.slot;
        agg.number_of_pdus += ind.number_of_pdus;

        for (size_t i = 0; i < ind.number_of_pdus; ++i)
        {
            if (pduIndex == NFAPI_NR_RX_DATA_IND_MAX_PDU)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
                break;
            }
            agg.pdu_list[pduIndex] = ind.pdu_list[i];
            pduIndex++;
        }
        free(ind.pdu_list);
        if (pduIndex == NFAPI_NR_RX_DATA_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_nr_rx_data_indication(&agg);
    free(agg.pdu_list);
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
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "cqi indication unpack failed, msg[%zu]", n);
            free(agg.cqi_indication_body.cqi_pdu_list);
            free(agg.cqi_indication_body.cqi_raw_pdu_list);
            return;
        }

        if (ind.cqi_indication_body.number_of_cqis == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "empty cqi message");
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
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "two cqis for single UE rnti: %x", rnti);
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
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
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
typedef struct
{
  uint16_t pdu_type;  // 0 for PDU on PUSCH, 1 for PUCCH format 0 or 1, 2 for PUCCH format 2 to 4
  uint16_t pdu_size;
  union
  {
    nfapi_nr_uci_pusch_pdu_t pusch_pdu;
        uint8_t  pduBitmap;
        uint32_t handle;
        uint16_t rnti;
        uint8_t  ul_cqi;
        uint16_t timing_advance;
        uint16_t rssi;
        nfapi_nr_harq_pdu_2_3_4_t harq;//table 3-70
            uint8_t  harq_crc;
            uint16_t harq_bit_len;
            //! fixme
            uint8_t*  harq_payload;//harq_payload[ceil(harq_bit_len)];
        nfapi_nr_csi_part1_pdu_t csi_part1;//71
            uint8_t  csi_part1_crc;
            uint16_t csi_part1_bit_len;
            //! fixme
            uint8_t*  csi_part1_payload;//uint8_t[ceil(csiPart1BitLen/8)]
        nfapi_nr_csi_part2_pdu_t csi_part2;//72
            uint8_t  csi_part2_crc;
            uint16_t csi_part2_bit_len;
            //! fixme
            uint8_t*  csi_part2_payload;//uint8_t[ceil(csiPart2BitLen/8)]
    nfapi_nr_uci_pucch_pdu_format_0_1_t pucch_pdu_format_0_1;
        uint8_t  pduBitmap;
        uint32_t handle;
        uint16_t rnti;
        uint8_t  pucch_format;//PUCCH format Value: 0 -> 1 0: PUCCH Format0 1: PUCCH Format1
        uint8_t  ul_cqi;
        uint16_t timing_advance;
        uint16_t rssi;
        nfapi_nr_sr_pdu_0_1_t *sr;//67
            uint8_t sr_indication;
            uint8_t sr_confidence_level;
        nfapi_nr_harq_pdu_0_1_t *harq;//68
            uint8_t num_harq;
            uint8_t harq_confidence_level;
            nfapi_nr_harq_t* harq_list;
                uint8_t  harq_value;//Indicates result on HARQ data. Value: 0 = pass 1 = fail 2 = not present
    nfapi_nr_uci_pucch_pdu_format_2_3_4_t pucch_pdu_format_2_3_4;
        uint8_t  pduBitmap;
        uint32_t handle;
        uint16_t rnti;
        uint8_t  pucch_format;//PUCCH format Value: 0 -> 2 0: PUCCH Format2 1: PUCCH Format3 2: PUCCH Format4
        uint8_t  ul_cqi;
        uint16_t timing_advance;
        uint16_t rssi;
        nfapi_nr_sr_pdu_2_3_4_t sr;//69
            uint16_t sr_bit_len;
            //! fixme
            uint8_t* sr_payload;//sr_payload[ceil(sr_bit_len/8)];
        nfapi_nr_harq_pdu_2_3_4_t harq;//70
            uint8_t  harq_crc;
            uint16_t harq_bit_len;
            //! fixme
            uint8_t*  harq_payload;//harq_payload[ceil(harq_bit_len)];
        nfapi_nr_csi_part1_pdu_t csi_part1;//71
            uint8_t  csi_part1_crc;
            uint16_t csi_part1_bit_len;
            //! fixme
            uint8_t*  csi_part1_payload;//uint8_t[ceil(csiPart1BitLen/8)]
        nfapi_nr_csi_part2_pdu_t csi_part2;//72
            uint8_t  csi_part2_crc;
            uint16_t csi_part2_bit_len;
            //! fixme
            uint8_t*  csi_part2_payload;//uint8_t[ceil(csiPart2BitLen/8)]
  };
} nfapi_nr_uci_t;

typedef struct
{
  nfapi_p7_message_header_t header;
  uint16_t sfn;
  uint16_t slot;
  uint16_t num_ucis;
  nfapi_nr_uci_t *uci_list;

} nfapi_nr_uci_indication_t;
*/

static int get_nr_uci_rnti(nfapi_nr_uci_t *pdu, uint16_t pdu_type)
{
    if (pdu_type == NFAPI_NR_UCI_PUSCH_PDU_TYPE)
        return pdu->pusch_pdu.rnti;
    else if (pdu_type == NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE)
        return pdu->pucch_pdu_format_0_1.rnti;
    else if (pdu_type == NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE)
        return pdu->pucch_pdu_format_2_3_4.rnti;
    return -1;
}

static void oai_slot_aggregate_uci_ind(slot_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_nr_uci_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.uci_list = calloc(NFAPI_NR_UCI_IND_MAX_PDU, sizeof(nfapi_nr_uci_t));
    assert(agg.uci_list);
    size_t pduIndex = 0;

    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_nr_uci_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_nr_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "uci indication unpack failed, msg[%zu]", n);
            free(agg.uci_list);
            return;
        }

        if (ind.num_ucis == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "empty uci message");
            abort();
        }

        uint16_t pdu_type = ind.uci_list[0].pdu_type;
        int rnti = get_nr_uci_rnti(&ind.uci_list[0], pdu_type);
        if (rnti == -1)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Invalid rnti value in uci indication");
            abort();
        }

        bool found = false;
        for (int i = 0; i < agg.num_ucis; ++i)
        {
            int rnti_i = get_nr_uci_rnti(&agg.uci_list[i], pdu_type);

            if (rnti == rnti_i)
            {
                found = true;
                break;
            }
        }
        if (found)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "two uci for single UE rnti: %x", rnti);
        }

        // Header, sfn, and slot assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different_slots(__func__, &agg.header, &ind.header, agg.sfn, ind.sfn, agg.slot, ind.slot);
        }
        agg.header = ind.header;
        agg.sfn = ind.sfn;
        agg.slot = ind.slot;
        agg.num_ucis += ind.num_ucis;

        for (size_t i = 0; i < ind.num_ucis; ++i)
        {
            if (pduIndex == NFAPI_NR_UCI_IND_MAX_PDU)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
                break;
            }

            agg.uci_list[pduIndex] = ind.uci_list[i];
            pduIndex++;
        }
        free(ind.uci_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_NR_UCI_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_nr_uci_indication(&agg);
    free(agg.uci_list); // Should actually be nfapi_vnf_p7_release_msg
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
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "harq indication unpack failed, msg[%zu]", n);
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
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
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
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "sr indication unpack failed, msg[%zu]", n);
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
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
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

/*
typedef struct
{
  uint32_t handle;
  uint16_t rnti;
  uint16_t timing_advance;
  uint8_t  num_symbols;
  uint8_t  wide_band_snr;
  uint8_t  num_reported_symbols;
  nfapi_nr_srs_indication_reported_symbol_t* reported_symbol_list;

}nfapi_nr_srs_indication_pdu_t;

typedef struct
{
  nfapi_p7_message_header_t header;
  uint16_t sfn;
  uint16_t slot;
  uint8_t number_of_pdus;
  nfapi_nr_srs_indication_pdu_t* pdu_list;

} nfapi_nr_srs_indication_t;
*/

static void oai_slot_aggregate_srs_ind(slot_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    nfapi_nr_srs_indication_t agg;
    memset(&agg, 0, sizeof(agg));
    agg.pdu_list = calloc(NFAPI_NR_SRS_IND_MAX_PDU, sizeof(nfapi_nr_srs_indication_t));
    assert(agg.pdu_list);

    size_t pduIndex = 0;
    for (size_t n = 0; n < msgs->num_msgs; ++n)
    {
        nfapi_nr_srs_indication_t ind;
        message_buffer_t *msg = msgs->msgs[n];
        assert(msg->length <= sizeof(msg->data));
        if (nfapi_nr_p7_message_unpack(msg->data, msg->length, &ind, sizeof(ind), NULL) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "srs indication unpack failed, msg[%zu]", n);
            free(agg.pdu_list);
            return;
        }

        // Header, sfn, and slot assumed to be same across msgs
        if (n != 0)
        {
            warn_if_different_slots(__func__, &agg.header, &ind.header, agg.sfn, ind.sfn, agg.slot, ind.slot);
        }
        agg.header = ind.header;
        agg.sfn = ind.sfn;
        agg.slot = ind.slot;
        agg.number_of_pdus += ind.number_of_pdus;

        for (size_t i = 0; i < ind.number_of_pdus; ++i)
        {
            if (pduIndex == NFAPI_NR_SRS_IND_MAX_PDU)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many PDUs to aggregate");
                break;
            }
            agg.pdu_list[pduIndex] = ind.pdu_list[i];
            pduIndex++;
        }
        free(ind.pdu_list); // Should actually be nfapi_vnf_p7_release_msg
        if (pduIndex == NFAPI_NR_SRS_IND_MAX_PDU)
        {
            break;
        }
    }

    oai_nfapi_nr_srs_indication(&agg);
    free(agg.pdu_list); // Should actually be nfapi_vnf_p7_release_msg
}

static void oai_subframe_aggregate_message_id(uint16_t msg_id, subframe_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    assert(msgs->msgs[0] != NULL);
    assert(msgs->msgs[0]->length <= sizeof(msgs->msgs[0]->data));
    uint16_t sfn_sf = nfapi_get_sfnsf(msgs->msgs[0]->data, msgs->msgs[0]->length);
    NFAPI_TRACE(NFAPI_TRACE_INFO, "(Proxy eNB) Aggregating collection of %s uplink messages prior to sending to eNB. Frame: %d, Subframe: %d",
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
                        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many msgs");
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

int get_slot_delta(uint16_t a, uint16_t b)
{
    int sfn_a = a >> 6;
    int slot_a = a & 0x3F;
    int sfn_b = b >> 6;
    int slot_b = b & 0x3F;

    return (sfn_a * 20 + slot_a) - (sfn_b * 20 + slot_b);
}

uint16_t sfn_sf_add(uint16_t a, uint16_t add_val)
{
    uint16_t sfn_a = a >> 4;
    uint16_t sf_a = a & 15;
    uint16_t temp = sfn_a * 10 + sf_a + add_val;
    uint16_t sf = temp % 10;
    uint16_t sfn = (temp / 10) % 1024;
    uint16_t sfn_sf = (sfn << 4) | sf;

    return sfn_sf;
}

void sfn_slot_add(uint16_t *sfn, uint16_t *slot, uint16_t add_val)
{
    *sfn = (*sfn + (*slot + add_val) / 20) % 1024;
    *slot = (*slot + add_val) % 20;
}

static void oai_slot_aggregate_message_id(uint16_t msg_id, slot_msgs_t *msgs)
{
    assert(msgs->num_msgs > 0);
    assert(msgs->msgs[0] != NULL);
    assert(msgs->msgs[0]->length <= sizeof(msgs->msgs[0]->data));
    uint16_t sfn_slot = nfapi_get_sfnslot(msgs->msgs[0]->data, msgs->msgs[0]->length);

    NFAPI_TRACE(NFAPI_TRACE_INFO, "(Proxy gNB) Aggregating collection of %s uplink messages prior to sending to gNB. Frame: %d, Slot: %d",
                nfapi_nr_get_message_id(msgs->msgs[0]->data, msgs->msgs[0]->length), NFAPI_SFNSLOT2SFN(sfn_slot), NFAPI_SFNSLOT2SLOT(sfn_slot));
    // Aggregate these messages and send the resulting message to the gNB
    switch (msg_id)
    {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
        oai_slot_aggregate_rach_ind(msgs);//It is using subframe. Need to check further.
        break;
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
        oai_slot_aggregate_crc_ind(msgs);
        break;
    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
        oai_slot_aggregate_rx_data_ind(msgs);
        break;
    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
        oai_slot_aggregate_uci_ind(msgs);
        break;
    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
        oai_slot_aggregate_srs_ind(msgs);
        break;
    default:
        return;
    }
}

/*
    slot_msgs is a pointer to an array of num_ues
*/
static void oai_slot_aggregate_messages(slot_msgs_t *slot_msgs)
{
    for (;;)
    {
        slot_msgs_t msgs_by_id;
        memset(&msgs_by_id, 0, sizeof(msgs_by_id));
        uint16_t found_msg_id = 0;
        for (int i = 0; i < num_ues; i++)
        {
            for (int j = 0; j < slot_msgs[i].num_msgs; j++)
            {
                message_buffer_t *msg = slot_msgs[i].msgs[j];
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
                    slot_msgs[i].msgs[j] = NULL;
                    if (msgs_by_id.num_msgs == MAX_SLOT_MSGS)
                    {
                        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many msgs");
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

        //To Do Items to implement oai_slot_aggregate_message_id
        oai_slot_aggregate_message_id(found_msg_id, &msgs_by_id);

        for (int k = 0; k < msgs_by_id.num_msgs; k++)
        {
            message_buffer_t *msg = msgs_by_id.msgs[k];
            assert(msg->magic == MESSAGE_BUFFER_MAGIC);
            msg->magic = 0;
            free(msg);
        }
    }
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
//                 NFAPI_TRACE(NFAPI_TRACE_ERROR, "Something went horribly wrong in dequer");
//                 abort();
//             }
//             for (int k = j + 1; k < subframe_msgs[i].num_msgs; ++k)
//             {
//                 message_buffer_t *msg_k = subframe_msgs[i].msgs[k];
//                 uint16_t sfn_sf_k;
//                 if (!get_sfnsf(msg_k, &sfn_sf_k))
//                 {
//                     NFAPI_TRACE(NFAPI_TRACE_ERROR, "Something went horribly wrong in dequer");
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
//             NFAPI_TRACE(NFAPI_TRACE_ERROR, "UE's have mismatched sfn_sfs in their message buffers "
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
            if (sf_delta > 200)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "sfn_sf not correct %u.%u - %u.%u = %d message id: %d", sfn_sf_tx >> 4,
                            sfn_sf_tx & 0XF, msg_sfn_sf >> 4, msg_sfn_sf & 0XF, sf_delta, get_message_id(msg));
                msg->magic = 0;
                free(msg);
                continue;
            }
            subframe_msgs_t *p = &subframe_msgs[i];
            if (p->num_msgs == MAX_SUBFRAME_MSGS)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many msgs from ue");
                msg->magic = 0;
                free(msg);
                continue;
            }
            p->msgs[p->num_msgs++] = msg;
            NFAPI_TRACE(NFAPI_TRACE_INFO, "p->num_msgs = %zu", p->num_msgs);
        }
    }

    return are_queues_empty;
}

bool dequeue_ue_slot_msgs(slot_msgs_t *slot_msgs, uint16_t sfn_slot_tx)
{
    // Dequeue for all UE responses, and discard any with the wrong sfn/sf value.
    // There might be multiple messages from a given UE with the same sfn/sf value.
    bool are_queues_empty = true;
    uint16_t master_sfn_slot = 0xFFFF;
    for (int i = 0; i < num_ues; i++)
    {
        for (;;)
        {
            message_buffer_t *msg = get_queue(&msgs_from_nr_ue[i]);
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
            uint16_t msg_sfn_slot;
            if (!get_sfnslot(msg, &msg_sfn_slot))
            {
                msg->magic = 0;
                free(msg);
                continue;
            }
            if (master_sfn_slot == 0xFFFF)
            {
                master_sfn_slot  = msg_sfn_slot;
            }
            if (master_sfn_slot != msg_sfn_slot)
            {
                if (!requeue(&msgs_from_nr_ue[i], msg))
                {
                    msg->magic = 0;
                    free(msg);
                }
                are_queues_empty = false;
                break;
            }
            int slot_delta = get_slot_delta(sfn_slot_tx, msg_sfn_slot);
            if (slot_delta > 400)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "sfn_slot not correct %u.%u - %u.%u = %d message id: %d", sfn_slot_tx >> 6,
                            sfn_slot_tx & 0X3F, msg_sfn_slot >> 6, msg_sfn_slot & 0X3F, slot_delta, get_message_id(msg));
                msg->magic = 0;
                free(msg);
                continue;
            }
            slot_msgs_t *p = &slot_msgs[i];
            if (p->num_msgs == MAX_SLOT_MSGS)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Too many msgs from ue");
                msg->magic = 0;
                free(msg);
                continue;
            }
            p->msgs[p->num_msgs++] = msg;
            NFAPI_TRACE(NFAPI_TRACE_INFO, "p->num_msgs = %zu", p->num_msgs);
        }
    }

    return are_queues_empty;
}

void add_sleep_time(uint64_t start, uint64_t poll, uint64_t send, uint64_t agg)
{
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "polling took %" PRIu64 "usec, subframe ind took %" PRIu64 "usec, aggreg took %" PRIu64 "usec",
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
        NFAPI_TRACE(NFAPI_TRACE_INFO, "Subframe loop took too long by %" PRIu64 "usec",
                    elapsed_usec - 1000);
    }
}

void add_nr_sleep_time(uint64_t start, uint64_t poll, uint64_t send, uint64_t agg)
{
    NFAPI_TRACE(NFAPI_TRACE_DEBUG, "polling took %" PRIu64 "usec, subframe ind took %" PRIu64 "usec, aggreg took %" PRIu64 "usec",
                poll - start,
                send - poll,
                agg - send);

    /* See how long this iteration of this loop took, and sleep for the
        balance of the 1ms subframe */
    uint64_t elapsed_usec = agg - start;
    if (elapsed_usec <= 500)
    {
        usleep(500 - elapsed_usec);
    }
    else
    {
        NFAPI_TRACE(NFAPI_TRACE_INFO, "Slot loop took too long by %" PRIu64 "usec",
                    elapsed_usec - 500);
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

static uint16_t sfn_slot_counter(uint16_t *sfn, uint16_t *slot)
{
    if (++*slot == 20)
    {
        *slot = 0;
        if (++*sfn == 1024)
        {
            *sfn = 0;
        }
    }
    return (*sfn << 6) | *slot;
}

#define LTE_PROXY_DONE   1
#define NR_PROXY_DONE    2
#define BOTH_LTE_NR_DONE 3
#define errExit(msg)     do { NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s", msg); \
                              exit(EXIT_FAILURE); \
                         } while (0)

uint16_t sf_slot_tick;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_sf_slot = PTHREAD_COND_INITIALIZER;


void *oai_subframe_task(void *context)
{
    pnf_set_thread_priority(79);
    uint16_t sfn = 0;
    uint16_t sf = 0;
    bool are_queues_empty = true;
    softmodem_mode_t softmodem_mode = (softmodem_mode_t) context;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Subframe Task thread");
    while (true)
    {

        uint16_t sfn_sf_tx = sfn_sf_counter(&sfn, &sf);

        uint64_t iteration_start = clock_usec();

        transfer_downstream_sfn_sf_to_proxy(sfn_sf_tx); // send to oai UE
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Frame %u Subframe %u sent to OAI ue", sfn_sf_tx >> 4,
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

        if (softmodem_mode == SOFTMODEM_NSA)
        {
            if (pthread_mutex_lock(&lock) != 0)
                errExit("failed to lock mutex");

            sf_slot_tick |= LTE_PROXY_DONE;
            if (sf_slot_tick == BOTH_LTE_NR_DONE)
            {
                if (pthread_cond_broadcast(&cond_sf_slot) != 0)
                    errExit("failed to broadcast on the condition");
            }
            else
            {
                while ( sf_slot_tick != BOTH_LTE_NR_DONE)
                {
                    if (pthread_cond_wait(&cond_sf_slot, &lock) != 0)
                        errExit("failed to wait on the condition");
                }
                sf_slot_tick = 0;
            }

            if (pthread_mutex_unlock(&lock) != 0)
                errExit("failed to unlock mutex");
        }

        uint64_t aggregation_done = clock_usec();

        if (are_queues_empty)
        {
            add_sleep_time(iteration_start, poll_end, subframe_sent, aggregation_done);
        }
    }
}

void *oai_slot_task(void *context)
{
    pnf_set_thread_priority(79);
    uint16_t sfn = 0;
    uint16_t slot = 0;
    bool are_queues_empty = true;
    softmodem_mode_t softmodem_mode = (softmodem_mode_t) context;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "Slot Task thread");
    uint16_t slot_tick = 0;

    while (true)
    {
        uint16_t sfn_slot_tx = sfn_slot_counter(&sfn, &slot);//Need to update it.

        uint64_t iteration_start = clock_usec();
        /* Previously we would sleep to ensure that the 500us contraint (per slot)
           was met, although the sleep time was 312us, which is pretty arbitrary.
           As we push higher throughputs, we see that the PNF (proxy) is
           ahead of the VNF (OAI gNB). By arbitrarily changing the sleep to be executed
           before every slot is sent to the OAI UE, and increasing the sleep time to 1000us,
           the slot indications are not arriving at the OAI NRUE too quickly as they were
           before. Furthermore, this ensures that even when TX_DATA_REQs take too long to
           arrive to the OAI UE, that the slot will arrive AFTER the TX_DATA_REQ. The relationship
           between slot indications and TX_DATA_REQs are critical to the ACK/nACK procedure.
           This is a temporary fix until will can concretely sync the PNF and VNF. */
        usleep(1000);
        transfer_downstream_sfn_slot_to_proxy(sfn_slot_tx); // send to oai UE
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Frame %u Slot %u sent to OAI ue", NFAPI_SFNSLOT2SFN(sfn_slot_tx),
                   NFAPI_SFNSLOT2SLOT(sfn_slot_tx));

        uint64_t poll_end = clock_usec();
        oai_slot_ind(sfn, slot);
        uint64_t slot_sent = clock_usec();

        /*
            Dequeue, collect and aggregate the messages with the same message ID.
        */
        slot_msgs_t slot_msgs[MAX_UES];
        memset(slot_msgs, 0, sizeof(slot_msgs));
        are_queues_empty = dequeue_ue_slot_msgs(slot_msgs, sfn_slot_tx);

        oai_slot_aggregate_messages(slot_msgs);

        if (++slot_tick == NR_PROXY_DONE && softmodem_mode == SOFTMODEM_NSA)
        {
            if (pthread_mutex_lock(&lock) != 0)
                errExit("failed to lock mutex");

            sf_slot_tick |= NR_PROXY_DONE;
            if (sf_slot_tick == BOTH_LTE_NR_DONE)
            {
                if (pthread_cond_broadcast(&cond_sf_slot) != 0)
                    errExit("failed to broadcast on the condition");
            }
            else{
                while ( sf_slot_tick != BOTH_LTE_NR_DONE)
                {
                    if (pthread_cond_wait(&cond_sf_slot, &lock) != 0)
                        errExit("failed to wait on the condition");
                }
                sf_slot_tick = 0;
            }

            if (pthread_mutex_unlock(&lock)!= 0)
                errExit("failed to unlock mutex");

            slot_tick = 0;
        }

        uint64_t aggregation_done = clock_usec();

        if (are_queues_empty)
        {
            add_nr_sleep_time(iteration_start, poll_end, slot_sent, aggregation_done);
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
    NFAPI_TRACE(NFAPI_TRACE_INFO, "(eNB) Adding %s uplink message to queue prior to sending to eNB. Frame: %d, Subframe: %d",
                nfapi_get_message_id(msg, len), NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));

    int i = (int)nem_id - MIN_UE_NEM_ID;
    if (i < 0 || i >= num_ues)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nem_id out of range: %d", nem_id);
        return;
    }

    message_buffer_t *p = malloc(sizeof(message_buffer_t));
    assert(p != NULL);
    p->magic = MESSAGE_BUFFER_MAGIC;
    p->length = len;
    assert(len < sizeof(p->data));
    memcpy(p->data, msg, len);

    if (!put_queue(&msgs_from_ue[i], p))
    {
        p->magic = 0;
        free(p);
    }
}

void oai_slot_handle_msg_from_ue(const void *msg, size_t len, uint16_t nem_id)
{
    if (len == 4) // Dummy packet ignore
    {
        return;
    }
    uint16_t sfn_slot = nfapi_get_sfnslot(msg, len);
    NFAPI_TRACE(NFAPI_TRACE_INFO, "(Proxy) Adding %s uplink message to queue prior to sending to gNB. Frame: %d, Slot: %d",
                nfapi_nr_get_message_id(msg, len), NFAPI_SFNSLOT2SFN(sfn_slot), NFAPI_SFNSLOT2SLOT(sfn_slot));

    int i = (int)nem_id - MIN_UE_NEM_ID;
    if (i < 0 || i >= num_ues)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nem_id out of range: %d", nem_id);
        return;
    }

    message_buffer_t *p = malloc(sizeof(message_buffer_t));
    assert(p != NULL);
    p->magic = MESSAGE_BUFFER_MAGIC;
    p->length = len;
    assert(len < sizeof(p->data));
    memcpy(p->data, msg, len);

    if (!put_queue(&msgs_from_nr_ue[i], p))
    {
        p->magic = 0;
        free(p);
    }
}

/*
void handle_nr_slot_ind(uint16_t sfn, uint16_t slot) {

    //send VNF slot indication, which is aligned with TX thread, so that it can call the scheduler
    nfapi_nr_slot_indication_scf_t *ind;
    ind = (nfapi_nr_slot_indication_scf_t *) malloc(sizeof(nfapi_nr_slot_indication_scf_t));
    uint8_t slot_ahead = 6;
    uint32_t sfn_slot_tx = sfnslot_add_slot(sfn, slot, slot_ahead);
    uint16_t sfn_tx = NFAPI_SFNSLOT2SFN(sfn_slot_tx);
    uint8_t slot_tx = NFAPI_SFNSLOT2SLOT(sfn_slot_tx);

    ind->sfn = sfn_tx;
    ind->slot = slot_tx;
    oai_nfapi_nr_slot_indication(ind); 

    //copy data from appropriate p7 slot buffers into channel structures for PHY processing
    nfapi_pnf_p7_slot_ind(p7_config_g, p7_config_g->phy_id, sfn, slot); 

    return;
}
*/

int oai_nfapi_rach_ind(nfapi_rach_indication_t *rach_ind)
{

    rach_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!

    NFAPI_TRACE(NFAPI_TRACE_INFO, "Sent the rach to eNB sf: %u sfn : %u num of preambles: %u",
               rach_ind->sfn_sf & 0xF, rach_ind->sfn_sf >> 4, rach_ind->rach_indication_body.number_of_preambles);

    return nfapi_pnf_p7_rach_ind(p7_config_g, rach_ind);
}

int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind)
{
    harq_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    harq_ind->header.message_id = NFAPI_HARQ_INDICATION;
    NFAPI_TRACE(NFAPI_TRACE_INFO, "sfn_sf:%d number_of_harqs:%d\n", NFAPI_SFNSF2DEC(harq_ind->sfn_sf),
               harq_ind->harq_indication_body.number_of_harqs);
    int retval = nfapi_pnf_p7_harq_ind(p7_config_g, harq_ind);

    if (retval != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "sfn_sf:%d number_of_harqs:%d nfapi_pnf_p7_harq_ind()=%d\n",
                    NFAPI_SFNSF2DEC(harq_ind->sfn_sf),
                    harq_ind->harq_indication_body.number_of_harqs, retval);
    }

    return retval;
}

int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind) // msg 3
{

    crc_ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    crc_ind->header.message_id = NFAPI_CRC_INDICATION;

    return nfapi_pnf_p7_crc_ind(p7_config_g, crc_ind);
}

int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *ind) // maybe msg 3
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
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "two cqis for single UE");
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
    char buffer[NFAPI_RX_IND_DATA_MAX];
    int encoded_size = nfapi_p7_message_pack(ind, buffer, sizeof(buffer), NULL);
    if (encoded_size <= 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to pack aggregated RX_IND for SFN.SF %u.%u",
                    ind->sfn_sf >> 4, ind->sfn_sf & 15);
        return -1;
    }

    nfapi_rx_indication_t test_ind;
    memset(&test_ind, 0, sizeof(test_ind));

    if (nfapi_p7_message_unpack(buffer, encoded_size, &test_ind, sizeof(test_ind), NULL) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to unpack aggregated RX_IND for SFN.SF %u.%u",
                    ind->sfn_sf >> 4, ind->sfn_sf & 15);
        return -1;
    }

    if (nfapi_pnf_p7_rx_ind(p7_config_g, ind) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Failed to send the RX_IND for SFN.SF %u.%u",
                    ind->sfn_sf >> 4, ind->sfn_sf & 15);
        return -1;
    }

    uint16_t num_rx = ind->rx_indication_body.number_of_pdus;
    for (int i = 0; i < num_rx; ++i)
    {
        for (int j = i + 1; j < num_rx; ++j)
        {
            int rnti_i = ind->rx_indication_body.rx_pdu_list[i].rx_ue_information.rnti;
            int rnti_j = ind->rx_indication_body.rx_pdu_list[j].rx_ue_information.rnti;
            if (rnti_i == rnti_j)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "two rx for a single UE rnti: %x", rnti_i);
            }
        }
        NFAPI_TRACE(NFAPI_TRACE_INFO, "rel8 tag[%d]: %x length %u", i, ind->rx_indication_body.rx_pdu_list[i].rx_indication_rel8.tl.tag,
                    ind->rx_indication_body.rx_pdu_list[i].rx_indication_rel8.length);
    }
    return 0;
}

int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind)
{

    ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
    int retval = nfapi_pnf_p7_sr_ind(p7_config_g, ind);
    return retval;
}

//NR UPLINK INDICATION

int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) {
  ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION;
  return nfapi_pnf_p7_nr_rx_data_ind(p7_nr_config_g, ind);
}

int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) {
  ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION;
  return nfapi_pnf_p7_nr_crc_ind(p7_nr_config_g, ind);
}

int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) {
  ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION;
  return nfapi_pnf_p7_nr_srs_ind(p7_nr_config_g, ind);
}

int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) {
  ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION;
  return nfapi_pnf_p7_nr_uci_ind(p7_nr_config_g, ind);
}

int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) {
  ind->header.phy_id = 1; // DJP HACK TODO FIXME - need to pass this around!!!!
  ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION;
  return nfapi_pnf_p7_nr_rach_ind(p7_nr_config_g, ind);
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
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "TODO implement me");
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
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "TODO implement me");
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
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "TODO implement me");
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
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "TODO implement me");
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
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "TODO implement me");
}

#ifdef __cplusplus
}
#endif

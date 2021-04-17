/*
                                nfapi_nr_interface.h
                             -------------------
  AUTHOR  : Chenyu Zhang, Florian Kaltenberger
  COMPANY : BUPT, EURECOM
  EMAIL   : octopus@bupt.edu.cn, florian.kaltenberger@eurecom.fr
*/

#ifndef _NFAPI_NR_INTERFACE_SCF_H_
#define _NFAPI_NR_INTERFACE_SCF_H_

#include "stddef.h"
#include "nfapi_interface.h"
#include "nfapi_nr_interface.h"

#define NFAPI_NR_MAX_NB_CCE_AGGREGATION_LEVELS 5
#define NFAPI_NR_MAX_NB_TCI_STATES_PDCCH 64
#define NFAPI_NR_MAX_NB_CORESETS 12
#define NFAPI_NR_MAX_NB_SEARCH_SPACES 40

#define NFAPI_MAX_NUM_UL_UE_PER_GROUP 6
#define NFAPI_MAX_NUM_UL_PDU 8
#define NFAPI_MAX_NUM_UCI_INDICATION 8
#define NFAPI_MAX_NUM_GROUPS 8
#define NFAPI_MAX_NUM_CB 8

// Extension to the generic structures for single tlv values


typedef enum {
  NFAPI_NR_DMRS_TYPE1=0,
  NFAPI_NR_DMRS_TYPE2
} nfapi_nr_dmrs_type_e;


typedef struct {
  /// Value: 0 -> 1, 0: Payload is carried directly in the value field, 1: Pointer to payload is in the value field 
  uint16_t tag; 
  /// Length of the actual payload in bytes, without the padding bytes Value: 0 → 65535
  uint16_t length;
  union { 
    uint32_t *ptr;
    uint32_t direct[16384];
  } value;
} nfapi_nr_tx_data_request_tlv_t;


// 2019.8
// SCF222_5G-FAPI_PHY_SPI_Specificayion.pdf Section 3.2

//PHY API message types

typedef enum {
  NFAPI_NR_PHY_MSG_TYPE_PARAM_REQUEST=  0x00,
  NFAPI_NR_PHY_MSG_TYPE_PARAM_RESPONSE= 0x01,
  NFAPI_NR_PHY_MSG_TYPE_CONFIG_REQUEST= 0x02,
  NFAPI_NR_PHY_MSG_TYPE_CONFIG_RESPONSE=0X03,
  NFAPI_NR_PHY_MSG_TYPE_START_REQUEST=  0X04,
  NFAPI_NR_PHY_MSG_TYPE_STOP_REQUEST=   0X05,
  NFAPI_NR_PHY_MSG_TYPE_STOP_INDICATION=0X06,
  NFAPI_NR_PHY_MSG_TYPE_ERROR_INDICATION=0X07,
  NFAPI_NR_PHY_MSG_TYPE_START_RESPONSE=0X010D,
  NFAPI_NR_PHY_MSG_TYPE_STOP_RESPONSE=0X010F,
  //RESERVED 0X08 ~ 0X7F
  NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST= 0X80,
  NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST= 0X81,
  NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION=0X82,
  NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST= 0X83,
  NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST=0X84, // CHANGED TO 0X84
  NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION=0X85,
  NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION= 0X86,
  NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION= 0X87,
  NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION= 0X88,
  NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION= 0X89,
  //RESERVED 0X8a ~ 0xff
  NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_REQUEST = 0x0100,
	NFAPI_NR_PHY_MSG_TYPE_PNF_PARAM_RESPONSE = 0x0101,
	NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_REQUEST= 0x0102,
	NFAPI_NR_PHY_MSG_TYPE_PNF_CONFIG_RESPONSE= 0x0103,
	NFAPI_NR_PHY_MSG_TYPE_PNF_START_REQUEST= 0x0104,
	NFAPI_NR_PHY_MSG_TYPE_PNF_START_RESPONSE= 0x0105,
	NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_REQUEST= 0x0106,
	NFAPI_NR_PHY_MSG_TYPE_PNF_STOP_RESPONSE= 0x0107,

  NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC = 0x0180,
	NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC,
	NFAPI_NR_PHY_MSG_TYPE_TIMING_INFO
} nfapi_nr_phy_msg_type_e;

// SCF222_5G-FAPI_PHY_SPI_Specificayion.pdf Section 3.3

//3.3.1 PARAM



//same with nfapi_param_request_t


/*typedef struct {
  nfapi_nr_param_errors_e error_code;
  //Number of TLVs contained in the message body.
  uint8_t number_of_tlvs;
  nfapi_nr_param_tlv_t TLV;
} nfapi_nr_param_response_t;*/


//PARAM and CONFIG TLVs are used in the PARAM and CONFIG message exchanges, respectively


//nfapi_nr_param_tlv_format_t cell param ~ measurement_param:

//table 3-9

#define  NFAPI_NR_PARAM_TLV_RELEASE_CAPABILITY_TAG 0x0001
#define  NFAPI_NR_PARAM_TLV_PHY_STATE_TAG         0x0002
#define  NFAPI_NR_PARAM_TLV_SKIP_BLANK_DL_CONFIG_TAG 0x0003
#define  NFAPI_NR_PARAM_TLV_SKIP_BLANK_UL_CONFIG_TAG 0x0004
#define  NFAPI_NR_PARAM_TLV_NUM_CONFIG_TLVS_TO_REPORT_TAG 0x0005
#define  NFAPI_NR_PARAM_TLV_CYCLIC_PREFIX_TAG 0x0006
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_DL_TAG 0x0007
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_DL_TAG 0x0008
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_SUBCARRIER_SPACINGS_UL_TAG 0x0009
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_BANDWIDTH_UL_TAG 0x000A
#define  NFAPI_NR_PARAM_TLV_CCE_MAPPING_TYPE_TAG 0x000B
#define  NFAPI_NR_PARAM_TLV_CORESET_OUTSIDE_FIRST_3_OFDM_SYMS_OF_SLOT_TAG 0x000C
#define  NFAPI_NR_PARAM_TLV_PRECODER_GRANULARITY_CORESET_TAG 0x000D
#define  NFAPI_NR_PARAM_TLV_PDCCH_MU_MIMO_TAG 0x000E
#define  NFAPI_NR_PARAM_TLV_PDCCH_PRECODER_CYCLING_TAG 0x000F
#define  NFAPI_NR_PARAM_TLV_MAX_PDCCHS_PER_SLOT_TAG 0x0010
#define  NFAPI_NR_PARAM_TLV_PUCCH_FORMATS_TAG 0x0011
#define  NFAPI_NR_PARAM_TLV_MAX_PUCCHS_PER_SLOT_TAG 0x0012
#define  NFAPI_NR_PARAM_TLV_PDSCH_MAPPING_TYPE_TAG 0x0013
#define  NFAPI_NR_PARAM_TLV_PDSCH_ALLOCATION_TYPES_TAG 0x0014
#define  NFAPI_NR_PARAM_TLV_PDSCH_VRB_TO_PRB_MAPPING_TAG 0x0015
#define  NFAPI_NR_PARAM_TLV_PDSCH_CBG_TAG 0x0016
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_CONFIG_TYPES_TAG 0x0017
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_MAX_LENGTH_TAG 0x0018
#define  NFAPI_NR_PARAM_TLV_PDSCH_DMRS_ADDITIONAL_POS_TAG 0x0019
#define  NFAPI_NR_PARAM_TLV_MAX_PDSCH_S_YBS_PER_SLOT_TAG 0x001A
#define  NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_PDSCH_TAG 0x001B
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_MAX_MODULATION_ORDER_DL_TAG 0x001C
#define  NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_DL_TAG 0x001D
#define  NFAPI_NR_PARAM_TLV_PDSCH_DATA_IN_DMRS_SYMBOLS_TAG 0x001E
#define  NFAPI_NR_PARAM_TLV_PREMPTION_SUPPORT_TAG 0x001F
#define  NFAPI_NR_PARAM_TLV_PDSCH_NON_SLOT_SUPPORT_TAG 0x0020
#define  NFAPI_NR_PARAM_TLV_UCI_MUX_ULSCH_IN_PUSCH_TAG 0x0021
#define  NFAPI_NR_PARAM_TLV_UCI_ONLY_PUSCH_TAG 0x0022
#define  NFAPI_NR_PARAM_TLV_PUSCH_FREQUENCY_HOPPING_TAG 0x0023
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_CONFIG_TYPES_TAG 0x0024
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_MAX_LEN_TAG 0x0025
#define  NFAPI_NR_PARAM_TLV_PUSCH_DMRS_ADDITIONAL_POS_TAG 0x0026
#define  NFAPI_NR_PARAM_TLV_PUSCH_CBG_TAG 0x0027
#define  NFAPI_NR_PARAM_TLV_PUSCH_MAPPING_TYPE_TAG 0x0028
#define  NFAPI_NR_PARAM_TLV_PUSCH_ALLOCATION_TYPES_TAG 0x0029
#define  NFAPI_NR_PARAM_TLV_PUSCH_VRB_TO_PRB_MAPPING_TAG 0x002A
#define  NFAPI_NR_PARAM_TLV_PUSCH_MAX_PTRS_PORTS_TAG 0x002B
#define  NFAPI_NR_PARAM_TLV_MAX_PDUSCHS_TBS_PER_SLOT_TAG 0x002C
#define  NFAPI_NR_PARAM_TLV_MAX_NUMBER_MIMO_LAYERS_NON_CB_PUSCH_TAG 0x002D
#define  NFAPI_NR_PARAM_TLV_SUPPORTED_MODULATION_ORDER_UL_TAG 0x002E
#define  NFAPI_NR_PARAM_TLV_MAX_MU_MIMO_USERS_UL_TAG 0x002F
#define  NFAPI_NR_PARAM_TLV_DFTS_OFDM_SUPPORT_TAG 0x0030
#define  NFAPI_NR_PARAM_TLV_PUSCH_AGGREGATION_FACTOR_TAG 0x0031
#define  NFAPI_NR_PARAM_TLV_PRACH_LONG_FORMATS_TAG 0x0032
#define  NFAPI_NR_PARAM_TLV_PRACH_SHORT_FORMATS_TAG 0x0033
#define  NFAPI_NR_PARAM_TLV_PRACH_RESTRICTED_SETS_TAG 0x0034
#define  NFAPI_NR_PARAM_TLV_MAX_PRACH_FD_OCCASIONS_IN_A_SLOT_TAG 0x0035
#define  NFAPI_NR_PARAM_TLV_RSSI_MEASUREMENT_SUPPORT_TAG 0x0036

typedef struct 
{
  nfapi_uint16_tlv_t release_capability; //TAG 0x0001
  nfapi_uint16_tlv_t phy_state;
  nfapi_uint8_tlv_t  skip_blank_dl_config;
  nfapi_uint8_tlv_t  skip_blank_ul_config;
  nfapi_uint16_tlv_t num_config_tlvs_to_report;
  nfapi_uint8_tlv_t* config_tlvs_to_report_list;
} nfapi_nr_cell_param_t;

//table 3-10 Carrier parameters
typedef struct 
{
  nfapi_uint8_tlv_t  cyclic_prefix;//TAG 0x0006
  nfapi_uint16_tlv_t  supported_subcarrier_spacings_dl;
  nfapi_uint16_tlv_t supported_bandwidth_dl;
  nfapi_uint8_tlv_t  supported_subcarrier_spacings_ul;
  nfapi_uint16_tlv_t supported_bandwidth_ul;
  
} nfapi_nr_carrier_param_t;

//table 3-11 PDCCH parameters
typedef struct 
{
  nfapi_uint8_tlv_t  cce_mapping_type;
  nfapi_uint8_tlv_t  coreset_outside_first_3_of_ofdm_syms_of_slot;
  nfapi_uint8_tlv_t  coreset_precoder_granularity_coreset;
  nfapi_uint8_tlv_t  pdcch_mu_mimo;
  nfapi_uint8_tlv_t  pdcch_precoder_cycling;
  nfapi_uint8_tlv_t  max_pdcch_per_slot;//TAG 0x0010

} nfapi_nr_pdcch_param_t;

//table 3-12 PUCCH parameters
typedef struct 
{
  nfapi_uint8_tlv_t pucch_formats;
  nfapi_uint8_tlv_t max_pucchs_per_slot;

} nfapi_nr_pucch_param_t;

//table 3-13 PDSCH parameters
typedef struct 
{
  nfapi_uint8_tlv_t pdsch_mapping_type;
  nfapi_uint8_tlv_t pdsch_allocation_types;
  nfapi_uint8_tlv_t pdsch_vrb_to_prb_mapping;
  nfapi_uint8_tlv_t pdsch_cbg;
  nfapi_uint8_tlv_t pdsch_dmrs_config_types;
  nfapi_uint8_tlv_t pdsch_dmrs_max_length;
  nfapi_uint8_tlv_t pdsch_dmrs_additional_pos;
  nfapi_uint8_tlv_t max_pdsch_tbs_per_slot;
  nfapi_uint8_tlv_t max_number_mimo_layers_pdsch;
  nfapi_uint8_tlv_t supported_max_modulation_order_dl;
  nfapi_uint8_tlv_t max_mu_mimo_users_dl;
  nfapi_uint8_tlv_t pdsch_data_in_dmrs_symbols;
  nfapi_uint8_tlv_t premption_support;//TAG 0x001F
  nfapi_uint8_tlv_t pdsch_non_slot_support;

} nfapi_nr_pdsch_param_t;

//table 3-14
typedef struct 
{
  nfapi_uint8_tlv_t uci_mux_ulsch_in_pusch;
  nfapi_uint8_tlv_t uci_only_pusch;
  nfapi_uint8_tlv_t pusch_frequency_hopping;
  nfapi_uint8_tlv_t pusch_dmrs_config_types;
  nfapi_uint8_tlv_t pusch_dmrs_max_len;
  nfapi_uint8_tlv_t pusch_dmrs_additional_pos;
  nfapi_uint8_tlv_t pusch_cbg;
  nfapi_uint8_tlv_t pusch_mapping_type;
  nfapi_uint8_tlv_t pusch_allocation_types;
  nfapi_uint8_tlv_t pusch_vrb_to_prb_mapping;
  nfapi_uint8_tlv_t pusch_max_ptrs_ports;
  nfapi_uint8_tlv_t max_pduschs_tbs_per_slot;
  nfapi_uint8_tlv_t max_number_mimo_layers_non_cb_pusch;
  nfapi_uint8_tlv_t supported_modulation_order_ul;
  nfapi_uint8_tlv_t max_mu_mimo_users_ul;
  nfapi_uint8_tlv_t dfts_ofdm_support;
  nfapi_uint8_tlv_t pusch_aggregation_factor;//TAG 0x0031

} nfapi_nr_pusch_param_t;

//table 3-15
typedef struct 
{
  nfapi_uint8_tlv_t prach_long_formats;
  nfapi_uint8_tlv_t prach_short_formats;
  nfapi_uint8_tlv_t prach_restricted_sets;
  nfapi_uint8_tlv_t max_prach_fd_occasions_in_a_slot;
} nfapi_nr_prach_param_t;

//table 3-16
typedef struct 
{
  nfapi_uint8_tlv_t rssi_measurement_support;
} nfapi_nr_measurement_param_t;

//-------------------------------------------//
//3.3.2 CONFIG

/*typedef struct {
	nfapi_nr_config_errors_e error_code;
  uint8_t number_of_invalid_tlvs_that_can_only_be_configured_in_idle;
  uint8_t unmber_of_missing_tlvs;
  //? ↓
  nfapi_nr_config_tlv_t* tlv_invalid_list;
  nfapi_nr_config_tlv_t* tlv_invalid_idle_list;
  nfapi_nr_config_tlv_t* tlv_invalid_running_list;
  nfapi_nr_config_tlv_t* tlv_missing_list;

} nfapi_nr_config_response_t;*/

//nfapi_nr_config_tlv_format_t carrier config ~ precoding config:

#define NFAPI_NR_CONFIG_DL_BANDWIDTH_TAG 0x1001
#define NFAPI_NR_CONFIG_DL_FREQUENCY_TAG 0x1002
#define NFAPI_NR_CONFIG_DL_K0_TAG 0x1003
#define NFAPI_NR_CONFIG_DL_GRID_SIZE_TAG 0x1004
#define NFAPI_NR_CONFIG_NUM_TX_ANT_TAG 0x1005
#define NFAPI_NR_CONFIG_UPLINK_BANDWIDTH_TAG 0x1006
#define NFAPI_NR_CONFIG_UPLINK_FREQUENCY_TAG 0x1007
#define NFAPI_NR_CONFIG_UL_K0_TAG 0x1008
#define NFAPI_NR_CONFIG_UL_GRID_SIZE_TAG 0x1009
#define NFAPI_NR_CONFIG_NUM_RX_ANT_TAG 0x100A
#define NFAPI_NR_CONFIG_FREQUENCY_SHIFT_7P5KHZ_TAG 0x100B

#define NFAPI_NR_CONFIG_PHY_CELL_ID_TAG 0x100C
#define NFAPI_NR_CONFIG_FRAME_DUPLEX_TYPE_TAG 0x100D

#define NFAPI_NR_CONFIG_SS_PBCH_POWER_TAG 0x100E
#define NFAPI_NR_CONFIG_BCH_PAYLOAD_TAG 0x100F
#define NFAPI_NR_CONFIG_SCS_COMMON_TAG 0x1010

#define NFAPI_NR_CONFIG_PRACH_SEQUENCE_LENGTH_TAG 0x1011
#define NFAPI_NR_CONFIG_PRACH_SUB_C_SPACING_TAG 0x1012
#define NFAPI_NR_CONFIG_RESTRICTED_SET_CONFIG_TAG 0x1013
#define NFAPI_NR_CONFIG_NUM_PRACH_FD_OCCASIONS_TAG 0x1014
#define NFAPI_NR_CONFIG_PRACH_CONFIG_INDEX_TAG 0x1029
#define NFAPI_NR_CONFIG_PRACH_ROOT_SEQUENCE_INDEX_TAG 0x1015
#define NFAPI_NR_CONFIG_NUM_ROOT_SEQUENCES_TAG 0x1016
#define NFAPI_NR_CONFIG_K1_TAG 0x1017
#define NFAPI_NR_CONFIG_PRACH_ZERO_CORR_CONF_TAG 0x1018
#define NFAPI_NR_CONFIG_NUM_UNUSED_ROOT_SEQUENCES_TAG 0x1019
#define NFAPI_NR_CONFIG_UNUSED_ROOT_SEQUENCES_TAG 0x101A
#define NFAPI_NR_CONFIG_SSB_PER_RACH_TAG 0x101B
#define NFAPI_NR_CONFIG_PRACH_MULTIPLE_CARRIERS_IN_A_BAND_TAG 0x101C

#define NFAPI_NR_CONFIG_SSB_OFFSET_POINT_A_TAG 0x101D
#define NFAPI_NR_CONFIG_BETA_PSS_TAG 0x101E
#define NFAPI_NR_CONFIG_SSB_PERIOD_TAG 0x101F
#define NFAPI_NR_CONFIG_SSB_SUBCARRIER_OFFSET_TAG 0x1020
#define NFAPI_NR_CONFIG_MIB_TAG 0x1021
#define NFAPI_NR_CONFIG_SSB_MASK_TAG 0x1022
#define NFAPI_NR_CONFIG_BEAM_ID_TAG 0x1023
#define NFAPI_NR_CONFIG_SS_PBCH_MULTIPLE_CARRIERS_IN_A_BAND_TAG 0x1024
#define NFAPI_NR_CONFIG_MULTIPLE_CELLS_SS_PBCH_IN_A_CARRIER_TAG 0x1025
#define NFAPI_NR_CONFIG_TDD_PERIOD_TAG 0x1026
#define NFAPI_NR_CONFIG_SLOT_CONFIG_TAG 0x1027

#define NFAPI_NR_CONFIG_RSSI_MEASUREMENT_TAG 0x1028

//table 3-21
typedef struct 
{
  nfapi_uint16_tlv_t dl_bandwidth;//Carrier bandwidth for DL in MHz [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  nfapi_uint32_tlv_t dl_frequency; //Absolute frequency of DL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  nfapi_uint16_tlv_t dl_k0[5];//𝑘_{0}^{𝜇} for each of the numerologies [38.211, sec 5.3.1] Value: 0 ->23699
  nfapi_uint16_tlv_t dl_grid_size[5];//Grid size 𝑁_{𝑔𝑟𝑖𝑑}^{𝑠𝑖𝑧𝑒,𝜇} for each of the numerologies [38.211, sec 4.4.2] Value: 0->275 0 = this numerology not used
  nfapi_uint16_tlv_t num_tx_ant;//Number of Tx antennas
  nfapi_uint16_tlv_t uplink_bandwidth;//Carrier bandwidth for UL in MHz. [38.104, sec 5.3.2] Values: 5, 10, 15, 20, 25, 30, 40,50, 60, 70, 80,90,100,200,400
  nfapi_uint32_tlv_t uplink_frequency;//Absolute frequency of UL point A in KHz [38.104, sec5.2 and 38.211 sec 4.4.4.2] Value: 450000 -> 52600000
  nfapi_uint16_tlv_t ul_k0[5];//𝑘0 𝜇 for each of the numerologies [38.211, sec 5.3.1] Value: : 0 ->23699
  nfapi_uint16_tlv_t ul_grid_size[5];//Grid size 𝑁𝑔𝑟𝑖𝑑 𝑠𝑖𝑧𝑒,𝜇 for each of the numerologies [38.211, sec 4.4.2]. Value: 0->275 0 = this numerology not used
  nfapi_uint16_tlv_t num_rx_ant;//
  nfapi_uint8_tlv_t  frequency_shift_7p5khz;//Indicates presence of 7.5KHz frequency shift. Value: 0 = false 1 = true

} nfapi_nr_carrier_config_t; 

//table 3-22
typedef struct 
{
  nfapi_uint16_tlv_t phy_cell_id;//Physical Cell ID, 𝑁_{𝐼𝐷}^{𝑐𝑒𝑙𝑙} [38.211, sec 7.4.2.1] Value: 0 ->1007
  nfapi_uint8_tlv_t frame_duplex_type;//Frame duplex type Value: 0 = FDD 1 = TDD

} nfapi_nr_cell_config_t;

//table 3-23
typedef struct 
{
  nfapi_uint32_tlv_t ss_pbch_power;//SSB Block Power Value: TBD (-60..50 dBm)
  nfapi_uint8_tlv_t  bch_payload;//Defines option selected for generation of BCH payload, see Table 3-13 (v0.0.011 Value: 0: MAC generates the full PBCH payload 1: PHY generates the timing PBCH bits 2: PHY generates the full PBCH payload
  nfapi_uint8_tlv_t  scs_common;//subcarrierSpacing for common, used for initial access and broadcast message. [38.211 sec 4.2] Value:0->3

} nfapi_nr_ssb_config_t;

//table 3-24
/*typedef struct {
  uint8_t unused_root_sequences;//Unused root sequence or sequences per FD occasion. Required for noise estimation.
} nfapi_nr_num_unused_root_sequences_t;*/

typedef struct 
{
  nfapi_uint16_tlv_t prach_root_sequence_index;//Starting logical root sequence index, 𝑖, equivalent to higher layer parameter prach-RootSequenceIndex [38.211, sec 6.3.3.1] Value: 0 -> 837
  nfapi_uint8_tlv_t  num_root_sequences;//Number of root sequences for a particular FD occasion that are required to generate the necessary number of preambles
  nfapi_uint16_tlv_t k1;//Frequency offset (from UL bandwidth part) for each FD. [38.211, sec 6.3.3.2] Value: from 0 to 272
  nfapi_uint8_tlv_t  prach_zero_corr_conf;//PRACH Zero CorrelationZone Config which is used to dervive 𝑁𝑐𝑠 [38.211, sec 6.3.3.1] Value: from 0 to 15
  nfapi_uint8_tlv_t  num_unused_root_sequences;//Number of unused sequences available for noise estimation per FD occasion. At least one unused root sequence is required per FD occasion.
  nfapi_uint8_tlv_t* unused_root_sequences_list;//Unused root sequence or sequences per FD occasion. Required for noise estimation.

} nfapi_nr_num_prach_fd_occasions_t;

typedef struct 
{
  nfapi_uint8_tlv_t prach_sequence_length;//RACH sequence length. Only short sequence length is supported for FR2. [38.211, sec 6.3.3.1] Value: 0 = Long sequence 1 = Short sequence
  nfapi_uint8_tlv_t prach_sub_c_spacing;//Subcarrier spacing of PRACH. [38.211 sec 4.2] Value:0->4
  nfapi_uint8_tlv_t restricted_set_config;//PRACH restricted set config Value: 0: unrestricted 1: restricted set type A 2: restricted set type B
  nfapi_uint8_tlv_t num_prach_fd_occasions;//Corresponds to the parameter 𝑀 in [38.211, sec 6.3.3.2] which equals the higher layer parameter msg1FDM Value: 1,2,4,8
  nfapi_uint8_tlv_t prach_ConfigurationIndex;//PRACH configuration index. Value:0->255
  nfapi_nr_num_prach_fd_occasions_t* num_prach_fd_occasions_list;

  nfapi_uint8_tlv_t ssb_per_rach;//SSB-per-RACH-occasion Value: 0: 1/8 1:1/4, 2:1/2 3:1 4:2 5:4, 6:8 7:16
  nfapi_uint8_tlv_t prach_multiple_carriers_in_a_band;//0 = disabled 1 = enabled

} nfapi_nr_prach_config_t;

//table 3-25
typedef struct 
{
  nfapi_uint32_tlv_t ssb_mask;//Bitmap for actually transmitted SSB. MSB->LSB of first 32 bit number corresponds to SSB 0 to SSB 31 MSB->LSB of second 32 bit number corresponds to SSB 32 to SSB 63 Value for each bit: 0: not transmitted 1: transmitted

} nfapi_nr_ssb_mask_size_2_t;

typedef struct 
{
  nfapi_uint8_tlv_t beam_id[64];//BeamID for each SSB in SsbMask. For example, if SSB mask bit 26 is set to 1, then BeamId[26] will be used to indicate beam ID of SSB 26. Value: from 0 to 63

} nfapi_nr_ssb_mask_size_64_t;

typedef struct 
{
  nfapi_uint16_tlv_t ssb_offset_point_a;//Offset of lowest subcarrier of lowest resource block used for SS/PBCH block. Given in PRB [38.211, section 4.4.4.2] Value: 0->2199
  nfapi_uint8_tlv_t  beta_pss;//PSS EPRE to SSS EPRE in a SS/PBCH block [38.213, sec 4.1] Values: 0 = 0dB
  nfapi_uint8_tlv_t  ssb_period;//SSB periodicity in msec Value: 0: ms5 1: ms10 2: ms20 3: ms40 4: ms80 5: ms160
  nfapi_uint8_tlv_t  ssb_subcarrier_offset;//ssbSubcarrierOffset or 𝑘𝑆𝑆𝐵 (38.211, section 7.4.3.1) Value: 0->31
  nfapi_uint32_tlv_t MIB;//MIB payload, where the 24 MSB are used and represent the MIB in [38.331 MIB IE] and represent 0 1 2 3 1 , , , ,..., A− a a a a a [38.212, sec 7.1.1]
  nfapi_nr_ssb_mask_size_2_t ssb_mask_list[2];
  nfapi_nr_ssb_mask_size_64_t* ssb_beam_id_list;//64
  nfapi_uint8_tlv_t  ss_pbch_multiple_carriers_in_a_band;//0 = disabled 1 = enabled
  nfapi_uint8_tlv_t  multiple_cells_ss_pbch_in_a_carrier;//Indicates that multiple cells will be supported in a single carrier 0 = disabled 1 = enabled

} nfapi_nr_ssb_table_t;

//table 3-26

//? 
typedef struct 
{
  nfapi_uint8_tlv_t slot_config;//For each symbol in each slot a uint8_t value is provided indicating: 0: DL slot 1: UL slot 2: Guard slot

} nfapi_nr_max_num_of_symbol_per_slot_t;

typedef struct 
{
  nfapi_nr_max_num_of_symbol_per_slot_t* max_num_of_symbol_per_slot_list;

} nfapi_nr_max_tdd_periodicity_t;

typedef struct 
{
  nfapi_uint8_tlv_t tdd_period;//DL UL Transmission Periodicity. Value:0: ms0p5 1: ms0p625 2: ms1 3: ms1p25 4: ms2 5: ms2p5 6: ms5 7: ms10 8: ms3 9: ms4
  nfapi_nr_max_tdd_periodicity_t* max_tdd_periodicity_list;

} nfapi_nr_tdd_table_t;

//table 3-27
typedef struct 
{
  nfapi_uint8_tlv_t rssi_measurement;//RSSI measurement unit. See Table 3-16 for RSSI definition. Value: 0: Do not report RSSI 1: dBm 2: dBFS

} nfapi_nr_measurement_config_t;

// ERROR enums
typedef enum {    // Table 2-22
  NFAPI_NR_PARAM_MSG_OK = 0, 
	NFAPI_NR_PARAM_MSG_INVALID_STATE
} nfapi_nr_param_errors_e;

typedef enum {    // Table 2-25
  NFAPI_NR_CONFIG_MSG_OK = 0,
  NFAPI_NR_CONFIG_MSG_INVALID_STATE, //The CONFIG.request was received when the PHY was not in the IDLE state or the CONFIGURED state.
  NFAPI_NR_CONFIG_MSG_INVALID_CONFIG  //The configuration provided has missing mandatory TLVs, or TLVs that are invalid or unsupported in this state.
} nfapi_nr_config_errors_e;

typedef enum {    // Table 2-27
  NFAPI_NR_START_MSG_OK = 0,       
	NFAPI_NR_START_MSG_INVALID_STATE
} nfapi_nr_start_errors_e;

//PNF P5 NR 
typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_param_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_pnf_param_general_t pnf_param_general;
	nfapi_pnf_phy_t pnf_phy;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_param_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint8_t num_tlvs;
	nfapi_pnf_phy_rf_config_t pnf_phy_rf_config;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_config_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_config_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_start_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_start_response_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_stop_request_t;

typedef struct {
	nfapi_p4_p5_message_header_t header;
	uint32_t error_code;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_pnf_stop_response_t;


/* PARAM.REQUEST */
typedef struct {
  nfapi_p4_p5_message_header_t  header;
	nfapi_vendor_extension_tlv_t  vendor_extension;
} nfapi_nr_param_request_scf_t;

/* PARAM.RESPONSE */
typedef struct {
  nfapi_p4_p5_message_header_t  header;
  uint8_t       error_code;
  
  uint8_t                       num_tlv;
  nfapi_vendor_extension_tlv_t  vendor_extension;

  nfapi_nr_cell_param_t         cell_param;
  nfapi_nr_carrier_param_t      carrier_param;
  nfapi_nr_pdcch_param_t        pdcch_param;
  nfapi_nr_pucch_param_t        pucch_param;
  nfapi_nr_pdsch_param_t        pdsch_param;
  nfapi_nr_pusch_param_t        pusch_param;
  nfapi_nr_prach_param_t        prach_param;
  nfapi_nr_measurement_param_t  measurement_param;
  nfapi_nr_nfapi_t              nfapi_config;
} nfapi_nr_param_response_scf_t;

//------------------------------//
//3.3.2 CONFIG

/* CONFIG.REQUEST */
typedef struct {
  nfapi_p4_p5_message_header_t  header;

  uint8_t                       num_tlv;
  nfapi_vendor_extension_tlv_t  vendor_extension;

  nfapi_nr_carrier_config_t     carrier_config;
  nfapi_nr_cell_config_t        cell_config;
  nfapi_nr_ssb_config_t         ssb_config;
  nfapi_nr_prach_config_t       prach_config;
  nfapi_nr_ssb_table_t          ssb_table;
  nfapi_nr_tdd_table_t          tdd_table;
  nfapi_nr_measurement_config_t measurement_config;
  nfapi_nr_nfapi_t              nfapi_config;
} nfapi_nr_config_request_scf_t;


/* CONFIG.RESPONSE */
typedef struct {
  nfapi_p4_p5_message_header_t  header;
  uint8_t error_code;
  //uint8_t num_invalid_tlvs;
  // TODO: add list of invalid/unsupported TLVs (see Table 3.18)
   nfapi_vendor_extension_tlv_t  vendor_extension;
} nfapi_nr_config_response_scf_t;

//------------------------------//
//3.3.3 START

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_start_request_scf_t;

typedef struct {
  nfapi_p4_p5_message_header_t header;
  nfapi_nr_start_errors_e error_code;
  nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_start_response_scf_t;

//3.3.4 STOP

typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_stop_request_t;


typedef struct {
	nfapi_p4_p5_message_header_t header;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_stop_indication_t;

typedef enum {
	NFAPI_NR_STOP_MSG_INVALID_STATE
} nfapi_nr_stop_errors_e;

//3.3.5 PHY Notifications
typedef enum {
  NFAPI_NR_PHY_API_MSG_OK              =0x0,
	NFAPI_NR_PHY_API_MSG_INVALID_STATE   =0x1,
  NFAPI_NR_PHY_API_MSG_INVALID_CONFIG  =0x2,
  NFAPI_NR_PHY_API_SFN_OUT_OF_SYNC     =0X3,
  NFAPI_NR_PHY_API_MSG_SLOR_ERR        =0X4,
  NFAPI_NR_PHY_API_MSG_BCH_MISSING     =0X5,
  NFAPI_NR_PHY_API_MSG_INVALID_SFN     =0X6,
  NFAPI_NR_PHY_API_MSG_UL_DCI_ERR      =0X7,
  NFAPI_NR_PHY_API_MSG_TX_ERR          =0X8
} nfapi_nr_phy_notifications_errors_e;

typedef struct {
	uint16_t sfn; //0~1023
  uint16_t slot;//0~319
  nfapi_nr_phy_msg_type_e msg_id;//Indicate which message received by the PHY has an error. Values taken from Table 3-4.
  nfapi_nr_phy_notifications_errors_e error_code;
} nfapi_nr_phy_notifications_error_indicate_t;

//-----------------------//
//3.3.6 Storing Precoding and Beamforming Tables

//table 3-32
//? 
typedef struct {
	uint16_t beam_idx;     //0~65535
} nfapi_nr_dig_beam_t;

typedef struct {
	uint16_t dig_beam_weight_Re;
  uint16_t dig_beam_weight_Im;
} nfapi_nr_txru_t;

typedef struct {
	uint16_t num_dig_beams; //0~65535
  uint16_t num_txrus;    //0~65535
  nfapi_nr_dig_beam_t* dig_beam_list;
  nfapi_nr_txru_t*  txru_list;
} nfapi_nr_dbt_pdu_t;


//table 3-33
//?
typedef struct {
  uint16_t num_ant_ports;
	int16_t precoder_weight_Re;
  int16_t precoder_weight_Im;
} nfapi_nr_num_ant_ports_t;

typedef struct {
  uint16_t numLayers;   //0~65535
	nfapi_nr_num_ant_ports_t* num_ant_ports_list;
} nfapi_nr_num_layers_t;

typedef struct {
	uint16_t pm_idx;       //0~65535
  nfapi_nr_num_layers_t* num_layers_list;   //0~65535
  //nfapi_nr_num_ant_ports_t* num_ant_ports_list;
} nfapi_nr_pm_pdu_t;


// Section 3.4

// Section 3.4.1 slot indication
#define NFAPI_NR_SLOT_INDICATION_PERIOD_NUMEROLOGY_0 1000 //us
#define NFAPI_NR_SLOT_INDICATION_PERIOD_NUMEROLOGY_1 50 //us
#define NFAPI_NR_SLOT_INDICATION_PERIOD_NUMEROLOGY_2 250 //us
#define NFAPI_NR_SLOT_INDICATION_PERIOD_NUMEROLOGY_3 125 //us

typedef struct {
	uint16_t sfn; //0->1023   
  uint16_t slot;//0->319
  
} nfapi_nr_slot_indication_scf_t;

// 3.4.2 

//for pdcch_pdu:

typedef struct
{
  uint16_t beam_idx;//Index of the digital beam weight vector pre-stored at cell configuration. The vector maps this input port to output TXRUs. Value: 0->65535

}nfapi_nr_dig_bf_interface_t;

typedef struct
{
  uint16_t pm_idx;//Index to precoding matrix (PM) pre-stored at cell configuration. Note: If precoding is not used this parameter should be set to 0. Value: 0->65535.
  nfapi_nr_dig_bf_interface_t* dig_bf_interface_list;

}nfapi_nr_tx_precoding_and_beamforming_number_of_prgs_t;

/*
//table 3-43
typedef struct 
{
  uint16_t num_prgs;//Number of PRGs spanning this allocation. Value : 1->275 
  uint16_t prg_size;//Size in RBs of a precoding resource block group (PRG) – to which same precoding and digital beamforming gets applied. Value: 1->275
  //watchout: dig_bf_interfaces here, in table 3-53 it's dig_bf_interface
  uint8_t  dig_bf_interfaces;//Number of STD ant ports (parallel streams) feeding into the digBF Value: 0->255
  nfapi_nr_tx_precoding_and_beamforming_number_of_prgs_t* prgs_list;//

}nfapi_nr_tx_precoding_and_beamforming_t;
*/

typedef struct {
  /// Number of PRGs spanning this allocation. Value : 1->275
  uint16_t numPRGs;
  /// Size in RBs of a precoding resource block group (PRG) – to which same precoding and digital beamforming gets applied. Value: 1->275
  uint16_t prgSize;
  /// Number of STD ant ports (parallel streams) feeding into the digBF Value: 0->255
  uint8_t digBFInterfaces;
  // Depends on numPRGs
  uint16_t PMIdx[275];
  // Depends on digBFInterfaces
  uint16_t beamIdx[256];
} nfapi_nr_tx_precoding_and_beamforming_t;

//table 3-37 

#define DCI_PAYLOAD_BYTE_LEN 8 // 12 ? TS38.212 sec 7.3.1
#define MAX_DCI_CORESET 8

typedef struct {
  // The RNTI used for identifying the UE when receiving the PDU Value: 1 -> 65535.
  uint16_t RNTI;
  // For a UE-specific search space it equals the higher-layer parameter PDCCH-DMRSScrambling-ID if configured,
  // otherwise it should be set to the phy cell ID. [TS38.211, sec 7.3.2.3] Value: 0->65535
  uint16_t ScramblingId;
  // For a UE-specific search space where PDCCH-DMRSScrambling- ID is configured This param equals the CRNTI.
  // Otherwise, it should be set to 0. [TS38.211, sec 7.3.2.3] Value: 0 -> 65535 
  uint16_t ScramblingRNTI;
  // CCE start Index used to send the DCI Value: 0->135
  uint8_t CceIndex;
  // Aggregation level used [TS38.211, sec 7.3.2.1] Value: 1,2,4,8,16
  uint8_t AggregationLevel;
  // Precoding and Beamforming structure See Table 3-43
  nfapi_nr_tx_precoding_and_beamforming_t precodingAndBeamforming;
  // PDCCH power value used for PDCCH Format 1_0 with CRC scrambled by SI-RNTI, PI-RNTI or RA-RNTI.
  // This is ratio of SSB/PBCH EPRE to PDCCH and PDCCH DMRS EPRE [TS38.213, sec 4.1]
  // Value :0->17 Report title: 5G FAPI: PHY API Specification Issue date: 29 June 2019 Version: 222.10.17 68 Field Type Description representing -8 to 8 dB in 1dB steps
  uint8_t beta_PDCCH_1_0;
  // PDCCH power value used for all other PDCCH Formats.
  // This is ratio of SSB/PBCH block EPRE to PDCCH and PDCCH DMRS EPRE [TS38.214, sec 4.1] Values: 0: -3dB,1: 0dB,2: 3dB,3: 6dB
  uint8_t powerControlOffsetSS;
  // The total DCI length (in bits) including padding bits [TS38.212 sec 7.3.1] Range 0->DCI_PAYLOAD_BYTE_LEN*8
  uint16_t PayloadSizeBits;
  // DCI payload, where the actual size is defined by PayloadSizeBits. The bit order is as following bit0-bit7 are mapped to first byte of MSB - LSB
  uint8_t Payload[DCI_PAYLOAD_BYTE_LEN];

} nfapi_nr_dl_dci_pdu_t;

typedef struct {
  /// Number of PRGs spanning this allocation. Value : 1->275
  uint16_t numPRGs;
  /// Size in RBs of a precoding resource block group (PRG) – to which same precoding and digital beamforming gets applied. Value: 1->275
  uint16_t prgSize;
  /// Number of STD ant ports (parallel streams) feeding into the digBF Value: 0->255
  uint8_t digBFInterfaces;
  uint16_t PMIdx[275];
  uint16_t *beamIdx[275];
} nr_beamforming_t;

typedef struct {
  ///Bandwidth part size [TS38.213 sec12]. Number of contiguous PRBs allocated to the BWP,Value: 1->275
  uint16_t BWPSize;
  ///bandwidth part start RB index from reference CRB, [TS38.213 sec 12], Value: 0->274
  uint16_t BWPStart;
  ///subcarrierSpacing [TS38.211 sec 4.2], Value:0->4
  uint8_t SubcarrierSpacing;
  ///Cyclic prefix type [TS38.211 sec 4.2], 0: Normal; 1: Extended
  uint8_t CyclicPrefix;
  ///Starting OFDM symbol for the CORESET, Value: 0->13
  uint8_t StartSymbolIndex;
///Contiguous time duration of the CORESET in number of symbols. Corresponds to L1 parameter 𝑁𝑠𝑦𝑚𝑏_𝐶𝑂𝑅𝐸𝑆𝐸𝑇 [TS38.211 sec 7.3.2.2] Value: 1,2,3
  uint8_t DurationSymbols; 
  ///Frequency domain resources. This is a bitmap defining non-overlapping groups of 6 PRBs in ascending order. [TS38.213 10.1]. Also, corresponds to L1 parameter CORE SET RB N [TS38.211 sec 7.3.2.2] Bitmap of uint8 array. 45 bits.
  uint8_t FreqDomainResource[6];
  ///CORESET-CCE-to-REG-mapping-type [TS38.211 sec 7.3.2.2] 0: non-interleaved 1: interleaved
  uint8_t CceRegMappingType;
  ///The number of REGs in a bundle. Must be 6 for cceRegMappingType = nonInterleaved. For cceRegMappingType = interleaved, must belong to {2,6} if duration = 1,2 and must belong to {3,6} if duration = 3. Corresponds to parameter L. [TS38.211 sec 7.3.2.2] Value: 2,3,6
  uint8_t RegBundleSize;
  ///The interleaver size. For interleaved mapping belongs to {2,3,6} and for non-interleaved mapping is NA. Corresponds to parameter R. [TS38.211 sec 7.3.2.2] Value: 2,3,6 CoreSetType
  uint8_t InterleaverSize; 
  ///[TS38.211 sec 7.3.2.2 and sec 7.4.1.3.2] 0: CORESET is configured by the PBCH or SIB1 (subcarrier 0 of CRB0 for DMRS mapping) 1: otherwise (subcarrier 0 of CORESET)
  uint8_t CoreSetType;
  ///[TS38.211 sec 7.3.2.2] Not applicable for non-interleaved mapping. For interleaved mapping and a PDCCH transmitted in a CORESET configured by the PBCH or SIB1 this should be set to phy cell ID. Value: 10 bits Otherwise, for interleaved mapping this is set to 0-> max num of PRBs. Value 0-> 275
  uint16_t ShiftIndex;
  ///Granularity of precoding [TS38.211 sec 7.3.2.2] Field Type Description 0: sameAsRegBundle 1: allContiguousRBs
  uint8_t precoderGranularity;
  ///Number of DCIs in this CORESET.Value: 0->MaxDciPerSlot
  uint16_t numDlDci;
  ///DL DCI PDU
  nfapi_nr_dl_dci_pdu_t dci_pdu[MAX_DCI_CORESET];
}  nfapi_nr_dl_tti_pdcch_pdu_rel15_t;

typedef struct {
  uint16_t pduBitmap;
  uint16_t rnti;
  uint16_t pduIndex;
  // BWP  [TS38.213 sec 12]
  /// Bandwidth part size [TS38.213 sec12]. Number of contiguous PRBs allocated to the BWP, Value: 1->275
  uint16_t BWPSize;
  /// bandwidth part start RB index from reference CRB [TS38.213 sec 12],Value: 0->274
  uint16_t BWPStart;
  /// subcarrierSpacing [TS38.211 sec 4.2], Value:0->4
  uint8_t SubcarrierSpacing;
  /// Cyclic prefix type [TS38.211 sec 4.2], 0: Normal; 1: Extended
  uint8_t CyclicPrefix;
  // Codeword information
  /// Number of code words for this RNTI (UE), Value: 1 -> 2
  uint8_t NrOfCodewords;
  /// Target coding rate [TS38.212 sec 5.4.2.1 and 38.214 sec 5.1.3.1]. This is the number of information bits per 1024 coded bits expressed in 0.1 bit units
  uint16_t targetCodeRate[2]; 
  /// QAM modulation [TS38.212 sec 5.4.2.1 and 38.214 sec 5.1.3.1], Value: 2,4,6,8
  uint8_t qamModOrder[2];
  ///  MCS index [TS38.214, sec 5.1.3.1], should match value sent in DCI Value : 0->31
  uint8_t mcsIndex[2];
  /// MCS-Table-PDSCH [TS38.214, sec 5.1.3.1] 0: notqam256, 1: qam256, 2: qam64LowSE
  uint8_t mcsTable[2];   
  /// Redundancy version index [TS38.212, Table 5.4.2.1-2 and 38.214, Table 5.1.2.1-2], should match value sent in DCI Value : 0->3
  uint8_t rvIndex[2];
  /// Transmit block size (in bytes) [TS38.214 sec 5.1.3.2], Value: 0->65535
  uint32_t TBSize[2];
  /// dataScramblingIdentityPdsch [TS38.211, sec 7.3.1.1], It equals the higher-layer parameter Datascrambling-Identity if configured and the RNTI equals the C-RNTI, otherwise L2 needs to set it to physical cell id. Value: 0->65535
  uint16_t dataScramblingId;
  /// Number of layers [TS38.211, sec 7.3.1.3]. Value : 1->8
  uint8_t nrOfLayers;
  /// PDSCH transmission schemes [TS38.214, sec5.1.1] 0: Up to 8 transmission layers
  uint8_t transmissionScheme;
  /// Reference point for PDSCH DMRS "k" - used for tone mapping [TS38.211, sec 7.4.1.1.2] Resource block bundles [TS38.211, sec 7.3.1.6] Value: 0 -> 1 If 0, the 0 reference point for PDSCH DMRS is at Point A [TS38.211 sec 4.4.4.2]. Resource block bundles generated per sub-bullets 2 and 3 in [TS38.211, sec 7.3.1.6]. For sub-bullet 2, the start of bandwidth part must be set to the start of actual bandwidth part +NstartCORESET and the bandwidth of the bandwidth part must be set to the bandwidth of the initial bandwidth part. If 1, the DMRS reference point is at the lowest VRB/PRB of the allocation. Resource block bundles generated per sub-bullets 1 [TS38.211, sec 7.3.1.6]
  uint8_t refPoint;
  // DMRS  [TS38.211 sec 7.4.1.1]
  /// DMRS symbol positions [TS38.211, sec 7.4.1.1.2 and Tables 7.4.1.1.2-3 and 7.4.1.1.2-4] Bitmap occupying the 14 LSBs with: bit 0: first symbol and for each bit 0: no DMRS 1: DMRS
  uint16_t dlDmrsSymbPos;  
  /// DL DMRS config type [TS38.211, sec 7.4.1.1.2] 0: type 1,  1: type 2
  uint8_t dmrsConfigType;
  /// DL-DMRS-Scrambling-ID [TS38.211, sec 7.4.1.1.2 ] If provided by the higher-layer and the PDSCH is scheduled by PDCCH with CRC scrambled by CRNTI or CS-RNTI, otherwise, L2 should set this to physical cell id. Value: 0->65535
  uint16_t dlDmrsScramblingId;
  /// DMRS sequence initialization [TS38.211, sec 7.4.1.1.2]. Should match what is sent in DCI 1_1, otherwise set to 0. Value : 0->1
  uint8_t SCID;
  /// Number of DM-RS CDM groups without data [TS38.212 sec 7.3.1.2.2] [TS38.214 Table 4.1-1] it determines the ratio of PDSCH EPRE to DM-RS EPRE. Value: 1->3
  uint8_t numDmrsCdmGrpsNoData;
  /// DMRS ports. [TS38.212 7.3.1.2.2] provides description between DCI 1-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
  uint16_t dmrsPorts;
  // Pdsch Allocation in frequency domain [TS38.214, sec 5.1.2.2]
  /// Resource Allocation Type [TS38.214, sec 5.1.2.2] 0: Type 0, 1: Type 1
  uint8_t resourceAlloc;
  /// For resource alloc type 0. TS 38.212 V15.0.x, 7.3.1.2.2 bitmap of RBs, 273 rounded up to multiple of 32. This bitmap is in units of VRBs. LSB of byte 0 of the bitmap represents the first RB of the bwp 
  uint8_t rbBitmap[36];
  /// For resource allocation type 1. [TS38.214, sec 5.1.2.2.2] The starting resource block within the BWP for this PDSCH. Value: 0->274
  uint16_t rbStart;
  /// For resource allocation type 1. [TS38.214, sec 5.1.2.2.2] The number of resource block within for this PDSCH. Value: 1->275
  uint16_t rbSize;
  /// VRB-to-PRB-mapping [TS38.211, sec 7.3.1.6] 0: non-interleaved 1: interleaved with RB size 2 2: Interleaved with RB size 4
  uint8_t VRBtoPRBMapping;
  // Resource Allocation in time domain [TS38.214, sec 5.1.2.1]
  /// Start symbol index of PDSCH mapping from the start of the slot, S. [TS38.214, Table 5.1.2.1-1] Value: 0->13
  uint8_t StartSymbolIndex;
  /// PDSCH duration in symbols, L [TS38.214, Table 5.1.2.1-1] Value: 1->14
  uint8_t NrOfSymbols;
  // PTRS [TS38.214, sec 5.1.6.3]
  /// PT-RS antenna ports [TS38.214, sec 5.1.6.3] [TS38.211, table 7.4.1.2.2-1] Bitmap occupying the 6 LSBs with: bit 0: antenna port 1000 bit 5: antenna port 1005 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t PTRSPortIndex ;
  /// PT-RS time density [TS38.214, table 5.1.6.3-1] 0: 1 1: 2 2: 4
  uint8_t PTRSTimeDensity;
  /// PT-RS frequency density [TS38.214, table 5.1.6.3-2] 0: 2 1: 4
  uint8_t PTRSFreqDensity;
  /// PT-RS resource element offset [TS38.211, table 7.4.1.2.2-1] Value: 0->3
  uint8_t PTRSReOffset;
  ///  PT-RS-to-PDSCH EPRE ratio [TS38.214, table 4.1-2] Value :0->3
  uint8_t nEpreRatioOfPDSCHToPTRS;
  // Beamforming
  nfapi_nr_tx_precoding_and_beamforming_t precodingAndBeamforming;
}nfapi_nr_dl_tti_pdsch_pdu_rel15_t;


//for pdsch_pdu:
/*
typedef struct
{
  uint16_t target_code_rate;//
  uint8_t  qam_mod_order;//
  uint8_t  mcs_index;//
  uint8_t  mcs_table;//
  uint8_t  rv_index;//
  uint32_t tb_size;//
} nfapi_nr_code_word_t;

//table 3-38
typedef struct
{
  uint16_t pdu_bit_map;//Bitmap indicating presence of optional PDUs Bit 0: pdschPtrs - Indicates PTRS included (FR2) Bit 1:cbgRetxCtrl (Present when CBG based retransmit is used) All other bits reserved
  uint16_t rnti;//The RNTI used for identifying the UE when receiving the PDU Value: 1 -> 65535
  uint16_t pdu_index;//
  uint16_t bwp_size;//Bandwidth part size [TS38.213 sec12]. Number of contiguous PRBs allocated to the BWP Value: 1->275
  uint16_t bwp_start;//bandwidth part start RB index from reference CRB [TS38.213 sec 12] Value: 0->274
  uint8_t  subcarrier_spacing;//Value:0->4
  uint8_t  cyclic_prefix;//0: Normal; 1: Extended
  uint8_t  nr_of_code_words;//Number of code words for this RNTI (UE) Value: 1 -> 2 
  nfapi_nr_code_word_t* code_word_list;
  uint16_t data_scrambling_id;//
  uint8_t  nr_of_layers;//
  uint8_t  transmission_scheme;//PDSCH transmission schemes [TS38.214, sec 5.1.1] 0: Up to 8 transmission layers
  uint8_t  ref_point;//Reference point for PDSCH DMRS "k" - used for tone mapping
  //DMRS
  uint16_t dl_dmrs_symb_pos;//Bitmap occupying the 14 LSBs with: bit 0: first symbol and for each bit 0: no DMRS 1: DMRS
  uint8_t  dmrs_config_type;//DL DMRS config type[TS38.211, sec 7.4.1.1.2] 0: type 1 1: type 2
  uint16_t dl_dmrs_scrambling_id;//DL-DMRS-Scrambling-ID [TS38.211, sec 7.4.1.1.2 ] If provided by the higher-layer and the PDSCH is scheduled by PDCCH with CRC scrambled by CRNTI or CS-RNTI, otherwise, L2 should set this to physical cell id. Value: 0->65535
  uint8_t  scid;// 0 1
  uint8_t  num_dmrs_cdm_grps_no_data;//Number of DM-RS CDM groups without data [TS38.212 sec 7.3.1.2.2] [TS38.214 Table 4.1-1] it determines the ratio of PDSCH EPRE to DM-RS EPRE. Value: 1->3
  uint16_t dmrs_ports;//DMRS ports. [TS38.212 7.3.1.2.2] provides description between DCI 1-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used

  //Pdsch Allocation in frequency domain [TS38.214, sec 5.1.2.2]
  uint8_t  resource_alloc;//Resource Allocation Type [TS38.214, sec 5.1.2.2] 0: Type 0 1: Type 1
  uint8_t  rb_bit_map[36];//For resource alloc type 0. TS 38.212 V15.0.x, 7.3.1.2.2 bitmap of RBs, 273 rounded up to multiple of 32. This bitmap is in units of VRBs. LSB of byte 0 of the bitmap represents the first RB of the bwp
  uint16_t rb_start;//For resource allocation type 1. [TS38.214, sec5.1.2.2.2] The starting resource block within the BWP for this PDSCH. Value: 0->274
  uint16_t rb_size;//For resource allocation type 1. [TS38.214, sec 5.1.2.2.2] The number of resource block within for this PDSCH. Value: 1->275
  uint8_t  vrb_to_prb_mapping;//VRB-to-PRB-mapping [TS38.211, sec 7.3.1.6] 0: non-interleaved 1: interleaved with RB size 2 2: Interleaved with RB size 4
  //Resource Allocation in time domain [TS38.214, sec 5.1.2.1]
  uint8_t  start_symbol_index;//Start symbol index of PDSCH mapping from the start of the slot, S. [TS38.214, Table 5.1.2.1-1] Value: 0->13
  uint8_t  nr_of_symbols;//PDSCH duration in symbols, L [TS38.214, Table 5.1.2.1-1] Value: 1->14
  //PTRS TS38.214 sec 5.1.6.3
  uint8_t  ptrs_port_index;//PT-RS antenna ports [TS38.214, sec 5.1.6.3] [TS38.211, table 7.4.1.2.2-1] Bitmap occupying the 6 LSBs with: bit 0:  ntenna port 1000 bit 5: antenna port 1005 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t  ptrs_time_density;//PT-RS time density [TS38.214, table 5.1.6.3-1] 0: 1 1: 2 2: 4
  uint8_t  ptrs_freq_density;//PT-RS frequency density [TS38.214, table[5.1.6.3-2] 0: 2 1: 4
  uint8_t  ptrs_re_offset;//PT-RS resource element offset [TS38.211, table [7.4.1.2.2-1]
  uint8_t  n_epre_ratio_of_pdsch_to_ptrs;//PT-RS-to-PDSCH EPRE ratio [TS38.214, table 4.1-2] Value :0->3
  nfapi_nr_tx_precoding_and_beamforming_t* precoding_and_beamforming_list;
  //TX power info
  uint8_t  power_control_offset;//Ratio of PDSCH EPRE to NZP CSI-RSEPRE [TS38.214, sec 5.2.2.3.1] Value :0->23 representing -8 to 15 dB in 1dB steps
  uint8_t  power_control_offset_ss;//Ratio of SSB/PBCH block EPRE to NZP CSI-RS EPRES [TS38.214, sec 5.2.2.3.1] Values: 0: -3dB, 1: 0dB, 2: 3dB, 3: 6dB
  //CBG fields
  uint8_t  is_last_cb_present;//Indicates whether last CB is present in the CBG retransmission 0: CBG retransmission does not include last CB 1: CBG retransmission includes last CB If CBG Re-Tx includes last CB, L1 will add the TB CRC to the last CB in the payload before it is read into the LDPC HW unit
  uint8_t  is_inline_tb_crc;//Indicates whether TB CRC is part of data payload or control message 0: TB CRC is part of data payload 1: TB CRC is part of control message
  uint32_t dl_tb_crc;//TB CRC: to be used in the last CB, applicable only if last CB is present

} nfapi_nr_dlsch_pdu_t;
*/

//for csi-rs_pdu:

//table 3-39
typedef struct
{
  uint16_t bwp_size;//
  uint16_t bwp_start;//
  uint8_t  subcarrier_spacing;//
  uint8_t  cyclic_prefix;//
  uint16_t start_rb;
  uint16_t nr_of_rbs;
  uint8_t  csi_type;//Value: 0:TRS 1:CSI-RS NZP 2:CSI-RS ZP
  uint8_t  row;//Row entry into the CSI Resource location table. [TS38.211, sec 7.4.1.5.3 and table 7.4.1.5.3-1] Value: 1-18
  uint16_t freq_domain;//Value: Up to the 12 LSBs, actual size is determined by the Row parameter
  uint8_t  symb_l0;//The time domain location l0 and firstOFDMSymbolInTimeDomain Value: 0->13
  uint8_t  symb_l1;//
  uint8_t  cdm_type;
  uint8_t  freq_density;//The density field, p and comb offset (for dot5).0: dot5 (even RB), 1: dot5 (odd RB), 2: one, 3: three
  uint16_t scramb_id;//ScramblingID of the CSI-RS [TS38.214, sec 5.2.2.3.1] Value: 0->1023
  //tx power info
  uint8_t  power_control_offset;//Ratio of PDSCH EPRE to NZP CSI-RSEPRE Value :0->23 representing -8 to 15 dB in 1dB steps
  uint8_t  power_control_offset_ss;//Ratio of SSB/PBCH block EPRE to NZP CSI-RS EPRES 0: -3dB, 1: 0dB, 2: 3dB, 3: 6dB

} nfapi_nr_dl_tti_csi_rs_pdu_rel15_t;

//for ssb_pdu: 

typedef struct
{
  uint32_t bch_payload;//BCH payload. The valid bits are indicated in the PARAM/CONFIG TLVs. If PARAM/CONFIG TLVs indicate MAC generates full bchPayload then the payload length is 31 bits with the 8 LSB bits being. Otherwise timing PBCH bits are generated by the PHY. And for bchPayload the 24 LSB are used.

} nfapi_nr_mac_generated_mib_pdu_t;

typedef struct
{
  uint8_t  dmrs_type_a_position;//The position of the first DM-RS for downlink and uplink.
  uint8_t  pdcch_config_sib1;//The parameter PDCCH-ConfigSIB1 that determines a common ControlResourceSet (CORESET) a common search space and necessary PDCCH parameters.
  uint8_t  cell_barred;//The flag to indicate whether the cell is barred
  uint8_t  intra_freq_reselection;//The flag to controls cell selection/reselection to intrafrequency cells when the highest ranked cell is barred, or treated as barred by the UE. value 0 1

} nfapi_nr_phy_generated_mib_pdu_t;


typedef struct
{
  nfapi_nr_mac_generated_mib_pdu_t* mac_generated_mib_pdu;
  nfapi_nr_phy_generated_mib_pdu_t* phy_generated_mib_pdu;

} nfapi_nr_bch_payload_t;

typedef struct {
  /// Physical Cell ID Value 0~>1007
  uint16_t PhysCellId;
  ///PSS EPRE to SSS EPRE in a SS/PBCH block 0 = 0dB 1 = 3dB
  uint8_t  BetaPss;
  ///SS/PBCH block index within a SSB burst set. Required for PBCH DMRS scrambling. Value: 0->63 (Lmax)
  uint8_t  SsbBlockIndex;
  /// ssbSubcarrierOffset or 𝑘𝑆𝑆𝐵 (TS38.211, section 7.4.3.1) Value: 0->31
  uint8_t  SsbSubcarrierOffset;
  ///Offset of lowest subcarrier of lowest resource block used for SS/PBCH block. Value: 0->2199
  uint16_t ssbOffsetPointA;
  /// A value indicating how the BCH payload is generated. This should match the PARAM/CONFIG TLVs. Value: 0: MAC generates the full PBCH payload, see Table 3-41, where bchPayload has 31 bits 1: PHY generates the timing PBCH bits, see Table 3-41, where the bchPayload has 24 bits 2: PHY generates the full PBCH payload
  uint8_t  bchPayloadFlag;
  uint32_t bchPayload;
  nfapi_nr_tx_precoding_and_beamforming_t precoding_and_beamforming;
} nfapi_nr_dl_tti_ssb_pdu_rel15_t;

typedef struct {
  nfapi_nr_dl_tti_ssb_pdu_rel15_t ssb_pdu_rel15;
} nfapi_nr_dl_tti_ssb_pdu;

typedef struct {
  nfapi_nr_dl_tti_csi_rs_pdu_rel15_t csi_rs_pdu_rel15;
} nfapi_nr_dl_tti_csi_rs_pdu;

typedef struct {
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t pdsch_pdu_rel15;
} nfapi_nr_dl_tti_pdsch_pdu;

typedef struct {
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t pdcch_pdu_rel15;
} nfapi_nr_dl_tti_pdcch_pdu;

typedef struct {
  uint16_t PDUType;
  uint16_t PDUSize;

  union {
  nfapi_nr_dl_tti_pdcch_pdu      pdcch_pdu;
  nfapi_nr_dl_tti_pdsch_pdu      pdsch_pdu;
  nfapi_nr_dl_tti_csi_rs_pdu     csi_rs_pdu;
  nfapi_nr_dl_tti_ssb_pdu        ssb_pdu;
  };
} nfapi_nr_dl_tti_request_pdu_t;

#define NFAPI_NR_MAX_DL_TTI_PDUS 32 

typedef struct {
  /// Number of PDUs that are included in this message. All PDUs in the message are numbered in order. Value 0 -> 255
  uint8_t nPDUs;
  /// Number of UEs in the Group included in this message. Value 0 -> 255
  uint8_t nGroup;
  /// List containing PDUs
  nfapi_nr_dl_tti_request_pdu_t dl_tti_pdu_list[NFAPI_NR_MAX_DL_TTI_PDUS];
  /// Number of UE in this group. For SU-MIMO, one group includes one UE only. For MU-MIMO, one group includes up to 12 UEs. Value 1 -> 12
  uint8_t nUe[256];
  /// This value is an index for number of PDU identified by nPDU in this message Value: 0 -> 255
  uint8_t PduIdx[256][12];
} nfapi_nr_dl_tti_request_body_t;


typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t t1;
	int32_t delta_sfn_slot;
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_dl_node_sync_t;

typedef struct {
	nfapi_p7_message_header_t header;
	uint32_t t1;
	uint32_t t2;
	uint32_t t3;	
	nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_ul_node_sync_t;


typedef struct {
  nfapi_p7_message_header_t header;
  /// System Frame Number (0-1023)
  uint16_t SFN;
  /// Slot number (0-19)
  uint16_t Slot;
  nfapi_nr_dl_tti_request_body_t dl_tti_request_body;
  nfapi_vendor_extension_tlv_t vendor_extension;
} nfapi_nr_dl_tti_request_t;

/*
typedef struct
{
  nfapi_nr_dl_tti_pdcch_pdu* pdcch_pdu;
  nfapi_nr_dl_tti_pdsch_pdu* pdsch_pdu;
  nfapi_nr_dl_tti_csi_rs_pdu_t* csi_rs_pdu;
  nfapi_nr_dl_tti_ssb_pdu_t* ssb_pdu;
} nfapi_nr_dl_pdu_configuration_t;
*/
 /* 
typedef struct 
{
  uint16_t pdu_type;//0: PDCCH PDU 1: PDSCH PDU 2: CSI-RS PDU 3: SSB PDU, 
  uint16_t pdu_size;//Size of the PDU control information (in bytes). This length value includes the 4 bytes required for the PDU type and PDU size parameters. Value 0 -> 65535
  nfapi_nr_dl_pdu_configuration_t* dl_pdu_configuration;

} nfapi_nr_dl_tti_request_number_of_pdus_t;
  

typedef struct 
{
  uint8_t  pdu_idx;//This value is an index for number of PDU identified by nPDU in this message Value: 0 -> 255

} nfapi_nr_dl_tti_request_number_of_ue_t;

typedef struct 
{
  uint8_t  n_ue;//Number of UE in this group For SU-MIMO, one group includes one UE only. For MU-MIMO, one group includes up to 12 UEs. Value 1 -> 12
  nfapi_nr_dl_tti_request_number_of_ue_t* ue_list;

} nfapi_nr_dl_tti_request_number_of_groups_t;

//3.4.2 dl_tti_request
typedef struct {
	uint16_t sfn; //0->1023   
  uint16_t slot;//0->319
  uint8_t n_pdus;//Number of PDUs that are included in this message. All PDUs in the message are numbered in order. Value 0 -> 255
  uint8_t n_group;//Number of UE Groups included in this message. Value 0 -> 255
  nfapi_nr_dl_tti_request_number_of_pdus_t* pdus_list;
  nfapi_nr_dl_tti_request_number_of_groups_t* groups_list;

} nfapi_nr_dl_tti_request_t;
*/

// Section 3.4.3 ul_tti_request
  
//for prach_pdu:
typedef struct
{
  nfapi_nr_dig_bf_interface_t* dig_bf_interface_list;

} nfapi_nr_ul_beamforming_number_of_prgs_t;

typedef struct
{
  uint16_t num_prgs;
  uint16_t prg_size;
  //watchout: dig_bf_interface here, in table 3-43 it's dig_bf_interfaces
  uint8_t  dig_bf_interface;
  nfapi_nr_ul_beamforming_number_of_prgs_t* prgs_list;//

} nfapi_nr_ul_beamforming_t;

typedef struct
{
  uint16_t phys_cell_id;
  uint8_t  num_prach_ocas;
  // SCF PRACH PDU format field does not consider A1/B1 etc. possibilities
  // We added 9 = A1/B1 10 = A2/B2 11 A3/B3
  uint8_t  prach_format;
  uint8_t  num_ra;
  uint8_t  prach_start_symbol;
  uint16_t num_cs;
  nfapi_nr_ul_beamforming_t beamforming;

} nfapi_nr_prach_pdu_t;

//for pusch_pdu:

  //for nfapi_nr_pusch_data_t;
typedef struct
{
  uint8_t  rv_index;
  uint8_t  harq_process_id;
  uint8_t  new_data_indicator;
  uint32_t tb_size;
  uint16_t num_cb;
  uint8_t cb_present_and_position[(NFAPI_MAX_NUM_CB+7) / 8];

} nfapi_nr_pusch_data_t;
  //for nfapi_nr_pusch_uci_t
typedef struct
{
  uint16_t harq_ack_bit_length;
  uint16_t csi_part1_bit_length;
  uint16_t csi_part2_bit_length;
  uint8_t  alpha_scaling;
  uint8_t  beta_offset_harq_ack;
  uint8_t  beta_offset_csi1;
  uint8_t  beta_offset_csi2;

} nfapi_nr_pusch_uci_t;

  //for nfapi_nr_pusch_ptrs_t
typedef struct
{
  uint16_t ptrs_port_index;//PT-RS antenna ports [TS38.214, sec6.2.3.1 and 38.212, section 7.3.1.1.2] Bitmap occupying the 12 LSBs with: bit 0: antenna port 0 bit 11: antenna port 11 and for each bit 0: PTRS port not used 1: PTRS port used
  uint8_t  ptrs_dmrs_port;//DMRS port corresponding to PTRS.
  uint8_t  ptrs_re_offset;//PT-RS resource element offset value taken from 0~11
} nfapi_nr_ptrs_ports_t;

typedef struct
{
  uint8_t  num_ptrs_ports;
  nfapi_nr_ptrs_ports_t* ptrs_ports_list;
  uint8_t  ptrs_time_density;
  uint8_t  ptrs_freq_density;
  uint8_t  ul_ptrs_power;

}nfapi_nr_pusch_ptrs_t;

  //for nfapi_nr_dfts_ofdm_t 
typedef struct
{
  uint8_t  low_papr_group_number;//Group number for Low PAPR sequence generation.
  uint16_t low_papr_sequence_number;//[TS38.211, sec 5.2.2] For DFT-S-OFDM.
  uint8_t  ul_ptrs_sample_density;//Number of PTRS groups [But I suppose this sentence is misplaced, so as the next one. --Chenyu]
  uint8_t  ul_ptrs_time_density_transform_precoding;//Number of samples per PTRS group

} nfapi_nr_dfts_ofdm_t;

#define PUSCH_PDU_BITMAP_PUSCH_DATA 0x1
#define PUSCH_PDU_BITMAP_PUSCH_UCI  0x2
#define PUSCH_PDU_BITMAP_PUSCH_PTRS 0x4
#define PUSCH_PDU_BITMAP_DFTS_OFDM  0x8

typedef struct
{
  uint16_t pdu_bit_map;//Bitmap indicating presence of optional PDUs (see above)
  uint16_t rnti;
  uint32_t handle;//An opaque handling returned in the RxData.indication and/or UCI.indication message
  //BWP
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint8_t  cyclic_prefix;
  //pusch information always include
  uint16_t target_code_rate;
  uint8_t  qam_mod_order;
  uint8_t  mcs_index;
  uint8_t  mcs_table;
  uint8_t  transform_precoding;
  uint16_t data_scrambling_id;
  uint8_t  nrOfLayers;
  //DMRS
  uint16_t  ul_dmrs_symb_pos;
  uint8_t  dmrs_config_type;
  uint16_t ul_dmrs_scrambling_id;
  uint16_t pusch_identity;
  uint8_t  scid;
  uint8_t  num_dmrs_cdm_grps_no_data;
  uint16_t dmrs_ports;//DMRS ports. [TS38.212 7.3.1.1.2] provides description between DCI 0-1 content and DMRS ports. Bitmap occupying the 11 LSBs with: bit 0: antenna port 1000 bit 11: antenna port 1011 and for each bit 0: DMRS port not used 1: DMRS port used
  //Pusch Allocation in frequency domain [TS38.214, sec 6.1.2.2]
  uint8_t  resource_alloc;
  uint8_t  rb_bitmap[36];//
  uint16_t rb_start;
  uint16_t rb_size;
  uint8_t  vrb_to_prb_mapping;
  uint8_t  frequency_hopping;
  uint16_t tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  uint8_t  uplink_frequency_shift_7p5khz;
  //Resource Allocation in time domain
  uint8_t  start_symbol_index;
  uint8_t  nr_of_symbols;
  //Optional Data only included if indicated in pduBitmap
  nfapi_nr_pusch_data_t pusch_data;
  nfapi_nr_pusch_uci_t  pusch_uci;
  nfapi_nr_pusch_ptrs_t pusch_ptrs;
  nfapi_nr_dfts_ofdm_t dfts_ofdm;
  //beamforming
  nfapi_nr_ul_beamforming_t beamforming;

} nfapi_nr_pusch_pdu_t;

//for pucch_pdu:
typedef struct
{
  uint16_t rnti;
  uint32_t handle;
  //BWP
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint8_t  cyclic_prefix;

  uint8_t  format_type;
  uint8_t  multi_slot_tx_indicator;
  uint8_t  pi_2bpsk;
  //pucch allocation in freq domain
  uint16_t prb_start;
  uint16_t prb_size;
  //pucch allocation in tome domain
  uint8_t  start_symbol_index;
  uint8_t  nr_of_symbols;
  //hopping info
  uint8_t  freq_hop_flag;
  uint16_t second_hop_prb;
  uint8_t  group_hop_flag;
  uint8_t  sequence_hop_flag;
  uint16_t hopping_id;
  uint16_t initial_cyclic_shift;

  uint16_t data_scrambling_id;
  uint8_t  time_domain_occ_idx;
  uint8_t  pre_dft_occ_idx;
  uint8_t  pre_dft_occ_len;
  //DMRS
  uint8_t  add_dmrs_flag;
  uint16_t dmrs_scrambling_id;
  uint8_t  dmrs_cyclic_shift;

  uint8_t  sr_flag;
  uint8_t  bit_len_harq;
  uint16_t bit_len_csi_part1;
  uint16_t bit_len_csi_part2;

  nfapi_nr_ul_beamforming_t beamforming;

} nfapi_nr_pucch_pdu_t;

//for srs_pdu:

typedef struct
{
  uint16_t rnti;//UE RNTI
  uint32_t handle;//An opaque handling returned in the SRS.indication
  //BWP
  uint16_t bwp_size;
  uint16_t bwp_start;
  uint8_t  subcarrier_spacing;
  uint8_t  cyclic_prefix;

  uint8_t  num_ant_ports;
  uint8_t  num_symbols;
  uint8_t  num_repetitions;
  uint8_t  time_start_position;//Starting position in the time domain l0; Note: the MAC undertakes the translation from startPosition to 𝑙0
  uint8_t  config_index;
  uint16_t sequence_id;
  uint8_t  bandwidth_index;
  uint8_t  comb_size;
  uint8_t  comb_offset;//Transmission comb offset 𝑘 ̄ TC [TS38.211, Sec 6.4.1.4.3] Value: 0 → 1 (combSize = 0) Value: 0 → 3 (combSize = 1)
  uint8_t  cyclic_shift;
  uint8_t  frequency_position;
  uint8_t  frequency_shift;
  uint8_t  frequency_hopping;
  uint8_t  group_or_sequence_hopping;//Group or sequence hopping configuration (RRC parameter groupOrSequenceHopping in SRS-Resource
  uint8_t  resource_type;//Type of SRS resource allocation
  uint16_t t_srs;//SRS-Periodicity in slots [TS38.211 Sec 6.4.1.4.4] Value: 1,2,3,4,5,8,10,16,20,32,40,64,80,160,320,640,1280,2560
  uint16_t t_offset;//Slot offset value [TS38.211, Sec 6.4.1.4.3] Value:0->2559

  nfapi_nr_ul_beamforming_t beamforming;

} nfapi_nr_srs_pdu_t;

//

typedef enum {
  NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE  = 0,
  NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE  = 1,
  NFAPI_NR_DL_TTI_CSI_RS_PDU_TYPE = 2,
  NFAPI_NR_DL_TTI_SSB_PDU_TYPE    = 3,
} nfapi_nr_dl_tti_pdu_type_e;

typedef enum {
  NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE=0,
  NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE,
  NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE,
  NFAPI_NR_UL_CONFIG_SRS_PDU_TYPE,
} nfapi_nr_ul_config_pdu_type_e;

typedef struct
{
  uint16_t pdu_type;//0: PRACH PDU, 1: PUSCH PDU, 2: PUCCH PDU, 3: SRS PDU
  uint16_t pdu_size;//Value: 0 -> 65535
  union
  {
    nfapi_nr_prach_pdu_t prach_pdu;
    nfapi_nr_pusch_pdu_t pusch_pdu;
    nfapi_nr_pucch_pdu_t pucch_pdu;
    nfapi_nr_srs_pdu_t srs_pdu;
  };

} nfapi_nr_ul_tti_request_number_of_pdus_t;

typedef struct 
{
  uint8_t  pdu_idx;//This value is an index for number of PDU identified by nPDU in this message Value: 0 -> 255

} nfapi_nr_ul_tti_request_number_of_ue_t;

typedef struct
{
  uint8_t  n_ue;//Number of UE in this group For SU-MIMO, one group includes one UE only. For MU-MIMO, one group includes up to 12 UEs. Value 1 -> 6
  nfapi_nr_ul_tti_request_number_of_ue_t ue_list[NFAPI_MAX_NUM_UL_UE_PER_GROUP];

} nfapi_nr_ul_tti_request_number_of_groups_t;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t SFN; //0->1023   
  uint16_t Slot;//0->319
  uint8_t n_pdus;//Number of PDUs that are included in this message. All PDUs in the message are numbered in order. Value 0 -> 255
  uint8_t  rach_present;//Indicates if a RACH PDU will be included in this message. 0: no RACH in this slot 1: RACH in this slot
  uint8_t  n_ulsch;//Number of ULSCH PDUs that are included in this message.
  uint8_t  n_ulcch;//Number of ULCCH PDUs
  uint8_t n_group;//Number of UE Groups included in this message. Value 0 -> 8
  nfapi_nr_ul_tti_request_number_of_pdus_t pdus_list[NFAPI_MAX_NUM_UL_PDU];
  nfapi_nr_ul_tti_request_number_of_groups_t groups_list[NFAPI_MAX_NUM_GROUPS];
} nfapi_nr_ul_tti_request_t;

//3.4.4 ul_dci_request

//table 3-54

/*
typedef struct 
{
  uint16_t pdu_type;//0: PDCCH PDU 
  uint16_t pdu_size;
  nfapi_nr_dl_pdu_configuration_t* phcch_pdu_configuration;

} nfapi_nr_ul_dci_request_number_of_pdus_t;

typedef struct
{
  uint16_t sfn;
  uint16_t slot;
  uint8_t  num_pdus;
  nfapi_nr_ul_dci_request_number_of_pdus_t* pdu_list;

} nfapi_nr_ul_dci_request_t;
*/

  // normally one PDU per coreset per BWP
#define NFAPI_NR_MAX_UL_DCI_PDUS 4

typedef struct {
  /// only possible value 0: PDCCH PDU
  uint16_t PDUType;
  ///Size of the PDU control information (in bytes). This length value includes the 4 bytes required for the PDU type and PDU size parameters. Value 0 -> 65535
  uint16_t PDUSize;
  nfapi_nr_dl_tti_pdcch_pdu pdcch_pdu;
} nfapi_nr_ul_dci_request_pdus_t;

typedef struct {
  nfapi_p7_message_header_t header;
  uint16_t SFN;
  uint16_t Slot;
  uint8_t  numPdus;
  nfapi_nr_ul_dci_request_pdus_t ul_dci_pdu_list[NFAPI_NR_MAX_UL_DCI_PDUS];
} nfapi_nr_ul_dci_request_t;

//3.4.5 slot_errors
typedef enum {
	NFAPI_NR_SLOT_UL_TTI_MSG_INVALID_STATE,
  NFAPI_NR_SLOT_UL_TTI_SFN_OUT_OF_SYNC,
  NFAPI_NR_SLOT_UL_TTI_MSG_BCH_MISSING,
  NFAPI_NR_SLOT_UL_TTI_MSG_SLOT_ERR

} nfapi_nr_slot_errors_ul_tti_e;

typedef enum {
	NFAPI_NR_SLOT_DL_TTI_MSG_INVALID_STATE,
  NFAPI_NR_SLOT_DL_TTI_MSG_SLOT_ERR

} nfapi_nr_slot_errors_dl_tti_e;


typedef enum {
	NFAPI_NR_SLOT_UL_DCI_MSG_INVALID_STATE,
  NFAPI_NR_SLOT_UL_DCI_MSG_INVALID_SFN,
  NFAPI_NR_SLOT_UL_DCI_MSG_UL_DCI_ERR

} nfapi_nr_slot_errors_ul_dci_e;

//3.4.6 tx_data_request

//table 3-58
#define NFAPI_NR_MAX_TX_REQUEST_TLV 2
typedef struct
{
  uint16_t PDU_length;
  uint16_t PDU_index;
  uint32_t num_TLV;
  nfapi_nr_tx_data_request_tlv_t TLVs[NFAPI_NR_MAX_TX_REQUEST_TLV]; 

} nfapi_nr_pdu_t;

#define NFAPI_NR_MAX_TX_REQUEST_PDUS 16
typedef struct
{
  nfapi_p7_message_header_t header;
  uint16_t SFN;
  uint16_t Slot;
  uint16_t Number_of_PDUs;
  nfapi_nr_pdu_t pdu_list[NFAPI_NR_MAX_TX_REQUEST_PDUS];

} nfapi_nr_tx_data_request_t;

typedef enum {
	NFAPI_NR_DL_DATA_MSG_INVALID_STATE,
  NFAPI_NR_DL_DATA_MSG_INVALID_SFN,
  NFAPI_NR_DL_DATA_MSG_TX_ERR

} nfapi_nr_dl_data_errors_e;

//section 3.4.7 rx_data_indication

//table 3-61

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
  uint16_t sfn;
  uint16_t slot;
  uint16_t number_of_pdus;
  nfapi_nr_rx_data_pdu_t* pdu_list;

} nfapi_nr_rx_data_indication_t;

//3.4.8 crc_indication
//table 3-62
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
  uint16_t sfn;
  uint16_t slot;
  uint16_t number_crcs;
  nfapi_nr_crc_t* crc_list;

} nfapi_nr_crc_indication_t;

//3.4.9 uci_indication

//table 3-67
typedef struct
{
  uint8_t sr_indication;
  uint8_t sr_confidence_level;

} nfapi_nr_sr_pdu_0_1_t;
//table 3-69
typedef struct
{
  uint16_t sr_bit_len;
  //! fixme
  uint8_t* sr_payload;//sr_payload[ceil(sr_bit_len/8)];

} nfapi_nr_sr_pdu_2_3_4_t;
//table 3-68
typedef struct
{
  uint8_t  harq_value;//Indicates result on HARQ data. Value: 0 = pass 1 = fail 2 = not present

} nfapi_nr_harq_t;

//table 3-70
typedef struct
{
  uint8_t  harq_crc;
  uint16_t harq_bit_len;
  //! fixme
  uint8_t*  harq_payload;//harq_payload[ceil(harq_bit_len)];
} nfapi_nr_harq_pdu_2_3_4_t;

typedef struct
{
  uint8_t num_harq;
  uint8_t harq_confidence_level;
  nfapi_nr_harq_t* harq_list;

} nfapi_nr_harq_pdu_0_1_t;

//table 3-71
typedef struct
{
  uint8_t  csi_part1_crc;
  uint16_t csi_part1_bit_len;
  //! fixme
  uint8_t*  csi_part1_payload;//uint8_t[ceil(csiPart1BitLen/8)]
  
} nfapi_nr_csi_part1_pdu_t;

//table 3-72
typedef struct
{
  uint8_t  csi_part2_crc;
  uint16_t csi_part2_bit_len;
  //! fixme
  uint8_t*  csi_part2_payload;//uint8_t[ceil(csiPart2BitLen/8)]
} nfapi_nr_csi_part2_pdu_t;

//table 3-63

  //for dci_pusch_pdu
typedef struct
{
  uint8_t  pduBitmap;
  uint32_t handle;
  uint16_t rnti;
  uint8_t  ul_cqi;
  uint16_t timing_advance;
  uint16_t rssi;
  nfapi_nr_harq_pdu_2_3_4_t harq;//table 3-70
  nfapi_nr_csi_part1_pdu_t csi_part1;//71
  nfapi_nr_csi_part2_pdu_t csi_part2;//72

}nfapi_nr_uci_pusch_pdu_t;

//for PUCCH PDU Format 0/1
typedef struct
{
  uint8_t  pduBitmap;
  uint32_t handle;
  uint16_t rnti;
  uint8_t  pucch_format;//PUCCH format Value: 0 -> 1 0: PUCCH Format0 1: PUCCH Format1
  uint8_t  ul_cqi;
  uint16_t timing_advance;
  uint16_t rssi;
  nfapi_nr_sr_pdu_0_1_t *sr;//67
  nfapi_nr_harq_pdu_0_1_t *harq;//68
  

}nfapi_nr_uci_pucch_pdu_format_0_1_t;

//PUCCH PDU Format 2/3/4
typedef struct
{
  uint8_t  pduBitmap;
  uint32_t handle;
  uint16_t rnti;
  uint8_t  pucch_format;//PUCCH format Value: 0 -> 2 0: PUCCH Format2 1: PUCCH Format3 2: PUCCH Format4
  uint8_t  ul_cqi;
  uint16_t timing_advance;
  uint16_t rssi;
  nfapi_nr_sr_pdu_2_3_4_t sr;//69
  nfapi_nr_harq_pdu_2_3_4_t harq;//70
  nfapi_nr_csi_part1_pdu_t csi_part1;//71
  nfapi_nr_csi_part2_pdu_t csi_part2;//72

}nfapi_nr_uci_pucch_pdu_format_2_3_4_t;

typedef enum {
  NFAPI_NR_UCI_PUSCH_PDU_TYPE  = 0,
  NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE  = 1,
  NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE = 2,
} nfapi_nr_uci_pdu_type_e;

typedef struct
{
  uint16_t pdu_type;  // 0 for PDU on PUSCH, 1 for PUCCH format 0 or 1, 2 for PUCCH format 2 to 4
  uint16_t pdu_size;
  union
  {
    nfapi_nr_uci_pusch_pdu_t pusch_pdu;
    nfapi_nr_uci_pucch_pdu_format_0_1_t pucch_pdu_format_0_1;
    nfapi_nr_uci_pucch_pdu_format_2_3_4_t pucch_pdu_format_2_3_4;
  };
} nfapi_nr_uci_t;

typedef struct
{
  uint16_t sfn;
  uint16_t slot;
  uint16_t num_ucis;
  nfapi_nr_uci_t *uci_list;

} nfapi_nr_uci_indication_t;

//3.4.10 srs_indication
//table 3-73

typedef struct
{
  uint8_t  rb_snr;
}nfapi_nr_srs_indication_reported_symbol_resource_block_t;

typedef struct
{
  uint16_t num_rbs;
  nfapi_nr_srs_indication_reported_symbol_resource_block_t* rb_list;
}nfapi_nr_srs_indication_reported_symbol_t;


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
  uint16_t sfn;
  uint16_t slot;
  uint8_t number_of_pdus;
  nfapi_nr_srs_indication_pdu_t* pdu_list;

} nfapi_nr_srs_indication_t;


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

#endif

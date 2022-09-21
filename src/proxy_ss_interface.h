#ifdef __cplusplus
extern "C" {
#endif

#ifndef _PROXY_SS_INTERFACE
#define _PROXY_SS_INTERFACE

#include "stdbool.h"

/* Default Values of RSRP, RSRQ and Cell Search Periodicity */
#define DEFAULT_CELL_RSRP 		40
#define DEFAULT_CELL_RSRQ 		30
#define DEFAULT_CELL_SRCH_PERIOD 	100

typedef enum proxy_ss_message_id {
  SS_ATTN_LIST     	= 1,
  SS_ATTN_LIST_CNF 	= 2,
  SS_CELL_CONFIG   	= 3,
  SS_CELL_CONFIG_CNF 	= 4,
  SS_CELL_RELEASE 	= 5,
  SS_CELL_RELEASE_CNF 	= 6,
  SS_VNG_CMD_REQ 	= 7,
  SS_VNG_CMD_RESP 	= 8,
  SS_INVALID_MSG 	= 0xFF
} proxy_ss_msgs_e;

typedef enum VngProxyCmd
{
    Vng_Invalid_Resp = 0,
    Vng_Config_Cmd = 1,
    Vng_Activate_Cmd = 2,
    Vng_Deactivate_Cmd = 3,
} VngProxyCmd_e;

typedef enum CarrierBandwidthEUTRA_dl_Bandwidth_e {
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n6 = 0,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n15 = 1,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n25 = 2,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n50 = 3,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n75 = 4,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_n100 = 5,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare10 = 6,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare9 = 7,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare8 = 8,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare7 = 9,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare6 = 10,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare5 = 11,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare4 = 12,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare3 = 13,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare2 = 14,
        CarrierBandwidthEUTRA_dl_Bandwidth_e_spare1 = 15,
} Dl_Bandwidth_e;


/*
 * Proxy SS Header: To be used for comunication between Proxy and SS-eNB.
 *
 * preamble: 0xFEEDC0DE for messages coming from SSeNB to Proxy
 *           0xF00DC0DE for messages coming from Proxy to SSeNB
 * msg_id  : To identify the message that is sent.
 * length  : Length of the message in bytes.
 */
typedef struct proxy_ss_header_s {
  uint32_t preamble;
  proxy_ss_msgs_e  msg_id;
  uint8_t  cell_id;
  uint16_t length;
} proxy_ss_header_t,
 *proxy_ss_header_p;

#define PROXY_CELL_CONFIG      ((1<<0))
#define PROXY_CELLATTN_LIST    ((1<<1))
#define PROXY_CELL_RELEASE     ((1<<2))
#define PROXY_VNG_CMD          ((1<<3))

typedef struct proxy_test_cfg_s {
    uint8_t vt_enabled; /** Virtual time or real time */
    uint8_t cli_rsrp_g; /** -dBm */
    uint8_t cli_rsrq_g; /** -dBm */
    uint16_t cli_periodicity_g; /** Subframes */
} test_cfg_t, *test_cfg_p;

typedef struct proxy_ss_cfg_s {
  uint8_t softmodem_mode;
  test_cfg_t cfg;
  bool     active;
  uint16_t sfn;
  uint8_t  sf;
  uint8_t  cfg_bitmap;
  uint16_t cell_id;
  uint16_t dl_earfcn;
  uint16_t ul_earfcn;
  uint32_t dl_freq;
  uint32_t ul_freq;
  int16_t  maxRefPower;
  uint8_t  initialAttenuation;
  uint8_t  currentAttnVal;
  uint8_t  Noc_activated; /** 0 Not activated, 1 activated */
  uint8_t  Noc_updated;
  int32_t  NocLevel;
  uint32_t Noc_Bandwidth;
  uint8_t  rsrp_dbm;
  uint8_t  rsrq_dbm;
} proxy_ss_cfg_t,
*proxy_ss_cfg_p;

/** Global pointer */
extern proxy_ss_cfg_p ss_cfg_g;

/** NOTE: Currently supporting only single cell */
#define MAX_CELLS 1

/** NOTE: Unused. There is only one cell at the moment. */
typedef struct proxy_ss_cb_s {
  uint8_t num_cells;
  proxy_ss_cfg_p proxy_ss_cfg[MAX_CELLS];
} proxy_ss_cb_t;

int init_ss_config(void);
#endif

#ifdef __cplusplus
}
#endif

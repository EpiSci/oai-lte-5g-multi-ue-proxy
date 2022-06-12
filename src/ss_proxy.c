#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "proxy_ss_interface.h"

extern proxy_ss_cfg_p ss_cfg_g;

int init_ss_config(void)
{
    ss_cfg_g = (proxy_ss_cfg_p) calloc (1, sizeof(proxy_ss_cfg_t));
    if (NULL != ss_cfg_g) 
    {
        ss_cfg_g->active = true;
        ss_cfg_g->cfg.cli_rsrp_g = DEFAULT_CELL_RSRP;
        ss_cfg_g->cfg.cli_rsrq_g = DEFAULT_CELL_RSRQ;
        ss_cfg_g->cfg.cli_periodicity_g = DEFAULT_CELL_SRCH_PERIOD;
        return 0;
    }
    else
    {
        printf("System simulator init failed\n");
	return -1;
    }
}

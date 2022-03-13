#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum softmodem_mode_t
{
    SOFTMODEM_LTE,
    SOFTMODEM_NR,
    SOFTMODEM_NSA,
    SOFTMODEM_LTE_HANDOVER,
} softmodem_mode_t;

struct oai_task_args
{
 softmodem_mode_t softmodem_mode;
 int node_id;
};

#ifdef __cplusplus
}
#endif

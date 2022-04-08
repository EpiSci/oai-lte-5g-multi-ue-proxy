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
#include <string>
#include <cstring>
#include <mutex>
#include <assert.h>
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include "proxy.h"

class Multi_UE_PNF
{
public:
    Multi_UE_PNF(int id, int num_of_ues, std::string enb_ip, std::string proxy_ip);
    ~Multi_UE_PNF() = default;
    void configure(std::string enb_ip, std::string proxy_ip);
    void start(softmodem_mode_t softmodem_mode);
private:
    std::string oai_ue_ipaddr;
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;
    int id;
    const int enb_port_delta = 200;
};

class Multi_UE_Proxy
{
public:
    Multi_UE_Proxy(int num_of_ues, std::vector<std::string> enb_ips, std::string proxy_ip, std::string ue_ip);
    ~Multi_UE_Proxy() = default;
    void configure(std::string ue_ip);
    int init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx);
    void oai_enb_downlink_nfapi_task(int id, void *msg);
    void testcode_tx_packet_to_UE( int ue_tx_socket_);
    void pack_and_send_downlink_sfn_sf_msg(uint16_t id, uint16_t sfn_sf);
    void receive_message_from_ue(int ue_id);
    void send_ue_to_enb_msg(void *buffer, size_t buflen);
    void send_received_msg_to_proxy_queue(void *buffer, size_t buflen);
    void send_uplink_oai_msg_to_proxy_queue(void *buffer, size_t buflen);
    void start(softmodem_mode_t softmodem_mode);
    std::vector<Multi_UE_PNF> lte_pnfs;
private:
    uint16_t eNB_id; // To identify the destination in uplink
    std::string oai_ue_ipaddr;
    std::string vnf_ipaddr;
    std::string pnf_ipaddr;
    int vnf_p5port = -1;
    int vnf_p7port = -1;
    int pnf_p7port = -1;

    struct sockaddr_in address_tx_;
    struct sockaddr_in address_rx_;
    int ue_tx_socket_ = -1;
    int ue_rx_socket_ = -1;
    int ue_rx_socket[100];
    int ue_tx_socket[100];

    typedef struct sfn_sf_info_s
    {
        uint16_t phy_id;
        uint16_t sfn_sf;
    } sfn_sf_info_t;

    std::recursive_mutex mutex;
    using lock_guard_t = std::lock_guard<std::recursive_mutex>;
    std::vector<std::thread> threads;
    bool stop_thread = false;
    const int port_delta = 2;
};

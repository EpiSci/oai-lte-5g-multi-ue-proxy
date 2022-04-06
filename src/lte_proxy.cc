#include <sys/stat.h>
#include <sstream>
#include "lte_proxy.h"
#include "nfapi_pnf.h"

namespace
{
    Multi_UE_Proxy *instance;
}

Multi_UE_Proxy::Multi_UE_Proxy(int num_of_ues, std::vector<std::string> enb_ips, std::string proxy_ip, std::string ue_ip)
{
    assert(instance == NULL);
    instance = this;
    taget_eNB_id = 0;

    num_ues = num_of_ues ;
    int num_of_enbs = enb_ips.size();

    for (int i = 0; i < num_of_enbs; i++)
    {
        lte_pnfs.push_back(Multi_UE_PNF(i, num_of_ues, enb_ips[i], proxy_ip));
    }
    configure(ue_ip);
}

Multi_UE_PNF::Multi_UE_PNF(int pnf_id, int num_of_ues, std::string enb_ip, std::string proxy_ip)
{
    num_ues = num_of_ues ;
    id = pnf_id;

    configure(enb_ip, proxy_ip);

    oai_subframe_init(pnf_id);
}

void Multi_UE_PNF::configure(std::string enb_ip, std::string proxy_ip)
{
    vnf_ipaddr = enb_ip;
    pnf_ipaddr = proxy_ip;
    std::cout<<"VNF is on IP Address "<<vnf_ipaddr<<std::endl;
    std::cout<<"PNF is on IP Address "<<pnf_ipaddr<<std::endl;
}

void Multi_UE_PNF::start(softmodem_mode_t softmodem_mode)
{
    pthread_t thread;
    vnf_p5port = 50001 + id * enb_port_delta;
    vnf_p7port = 50011 + id * enb_port_delta;
    pnf_p7port = 50010 + id * enb_port_delta;

    struct oai_task_args args {softmodem_mode, id};

    configure_nfapi_pnf(id, vnf_ipaddr.c_str(), vnf_p5port, pnf_ipaddr.c_str(), pnf_p7port, vnf_p7port);

    if (pthread_create(&thread, NULL, &oai_subframe_task, (void *)&args) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create failed for calling oai_subframe_task");
    }
}

void Multi_UE_Proxy::start(softmodem_mode_t softmodem_mode)
{
    int num_lte_pnfs = lte_pnfs.size();

    for (int i = 0; i < num_lte_pnfs; i++)
    {
        lte_pnfs[i].start(softmodem_mode);
        sleep(1);
    }

    for (int i = 0; i < num_ues; i++)
    {
        threads.push_back(std::thread(&Multi_UE_Proxy::receive_message_from_ue, this, i));
    }
    for (auto &th : threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }
}

void Multi_UE_Proxy::configure(std::string ue_ip)
{
    oai_ue_ipaddr = ue_ip;
    std::cout<<"OAI-UE is on IP Address "<<oai_ue_ipaddr<<std::endl;

    for (int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        int oai_rx_ue_port = 3211 + ue_idx * port_delta;
        int oai_tx_ue_port = 3212 + ue_idx * port_delta;
        init_oai_socket(oai_ue_ipaddr.c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
    }
}

int Multi_UE_Proxy::init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx)
{
    {   //Setup Rx Socket
        memset(&address_rx_, 0, sizeof(address_rx_));
        address_rx_.sin_family = AF_INET;
        address_rx_.sin_addr.s_addr = INADDR_ANY;
        address_rx_.sin_port = htons(rx_port);

        ue_rx_socket_ = socket(address_rx_.sin_family, SOCK_DGRAM, 0);
        ue_rx_socket[ue_idx] = ue_rx_socket_;
        if (ue_rx_socket_ < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
            return -1;
        }
        if (bind(ue_rx_socket_, (struct sockaddr *)&address_rx_, sizeof(address_rx_)) < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "bind failed in init_oai_socket: %s\n", strerror(errno));
            close(ue_rx_socket_);
            ue_rx_socket_ = -1;
            return -1;
        }
    }
    {   //Setup Tx Socket
        memset(&address_tx_, 0, sizeof(address_tx_));
        address_tx_.sin_family = AF_INET;
        address_tx_.sin_port = htons(tx_port);

        if (inet_aton(addr, &address_tx_.sin_addr) == 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "addr no good %s", addr);
            return -1;
        }

        ue_tx_socket_ = socket(address_tx_.sin_family, SOCK_DGRAM, 0);
        ue_tx_socket[ue_idx] = ue_tx_socket_;
        if (ue_tx_socket_ < 0)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "socket: %s", ERR);
            return -1;
        }

        if (connect(ue_tx_socket_, (struct sockaddr *)&address_tx_, sizeof(address_tx_)) < 0)
        {
          NFAPI_TRACE(NFAPI_TRACE_ERROR, "tx connection failed in init_oai_socket: %s\n", strerror(errno));
          close(ue_tx_socket_);
          return -1;
        }
    }
    return 0;
}

void Multi_UE_Proxy::receive_message_from_ue(int ue_idx)
{
    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    socklen_t addr_len = sizeof(address_rx_);

    while(true)
    {
        int buflen = recvfrom(ue_rx_socket[ue_idx], buffer, sizeof(buffer), 0, (sockaddr *)&address_rx_, &addr_len);
        if (buflen == -1)
        {
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Recvfrom failed %s", strerror(errno));
            return ;
        }
        if (buflen == 4)
        {
            //NFAPI_TRACE(NFAPI_TRACE_INFO , "Dummy frame");
            continue;
        }
        else
        {
            nfapi_p7_message_header_t header;
            if (nfapi_p7_message_header_unpack(buffer, buflen, &header, sizeof(header), NULL) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Header unpack failed for standalone pnf");
                return ;
            }
            uint16_t sfn_sf = nfapi_get_sfnsf(buffer, buflen);
            taget_eNB_id = header.phy_id;
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(Proxy) Proxy has received %d uplink message from OAI UE for eNB%u at socket. Frame: %d, Subframe: %d",
                    header.message_id, taget_eNB_id, NFAPI_SFNSF2SFN(sfn_sf), NFAPI_SFNSF2SF(sfn_sf));
        }
        oai_subframe_handle_msg_from_ue(taget_eNB_id, buffer, buflen, ue_idx + 2);
    }
}

void Multi_UE_Proxy::oai_enb_downlink_nfapi_task(int id, void *msg_org)
{
    lock_guard_t lock(mutex);

    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    int encoded_size = nfapi_p7_message_pack(msg_org, buffer, sizeof(buffer), nullptr);
    if (encoded_size <= 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "Message pack failed");
        return;
    }

    union
    {
        nfapi_p7_message_header_t header;
        nfapi_dl_config_request_t dl_config_req;
        nfapi_tx_request_t tx_req;
        nfapi_hi_dci0_request_t hi_dci0_req;
        nfapi_ul_config_request_t ul_config_req;
    } msg;

    if (nfapi_p7_message_unpack((void *)buffer, encoded_size, &msg, sizeof(msg), NULL) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_p7_message_unpack failed NEM ID: %d", 1);
        return;
    }

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_.sin_port = htons(3212 + ue_idx * port_delta);
        uint16_t id_=1;
        switch (msg.header.message_id)
        {

        case NFAPI_DL_CONFIG_REQUEST:
        {
            int dl_sfn = NFAPI_SFNSF2SFN(msg.dl_config_req.sfn_sf);
            int dl_sf = NFAPI_SFNSF2SF(msg.dl_config_req.sfn_sf);
            uint16_t dl_numPDU = msg.dl_config_req.dl_config_request_body.number_pdu;
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(UE) Prior to sending dl_config_req to OAI UE. Frame: %d,"
                       " Subframe: %d, Number of PDUs: %u",
                       dl_sfn, dl_sf, dl_numPDU);
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_DL_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "DL_CONFIG_REQ forwarded to UE from UE NEM: %u of cell id %d", id_, id);
            }
            break;
        }
        case NFAPI_TX_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_TX_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "TX_REQ forwarded to UE from UE NEM: %u of cell id %d", id_, id);
            }
            break;

        case NFAPI_UL_CONFIG_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_UL_CONFIG_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "UL_CONFIG_REQ forwarded to UE from UE NEM: %u of cell id %d", id_, id);
            }
            break;

        case NFAPI_HI_DCI0_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_HI_DCI0_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_HI_DCI0_REQ forwarded to UE from UE NEM: %u of cell id %d", id_, id);
            }
            break;

        default:
            NFAPI_TRACE(NFAPI_TRACE_INFO , "Unhandled message at UE NEM: %d message_id: %u", id_, msg.header.message_id);
            break;
        }
    }
}

void Multi_UE_Proxy::pack_and_send_downlink_sfn_sf_msg(uint16_t id, uint16_t sfn_sf)
{
    lock_guard_t lock(mutex);

    sfn_sf_info_t sfn_sf_info;
    sfn_sf_info.phy_id = id;
    sfn_sf_info.sfn_sf = sfn_sf;

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_.sin_port = htons(3212 + ue_idx * port_delta);
        assert(ue_tx_socket[ue_idx] > 2);
        if (sendto(ue_tx_socket[ue_idx], &sfn_sf_info, sizeof(sfn_sf_info), 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
        {
            int sfn = NFAPI_SFNSF2SFN(sfn_sf);
            int sf = NFAPI_SFNSF2SF(sfn_sf);
            NFAPI_TRACE(NFAPI_TRACE_DEBUG, "Send sfn_sf_tx to OAI UE FAIL Frame: %d,Subframe: %d from cell id %d\n", sfn, sf, id);
        }
    }
}

void transfer_downstream_nfapi_msg_to_proxy(uint16_t id, void *msg)
{
    instance->oai_enb_downlink_nfapi_task(id, msg);
}
void transfer_downstream_sfn_sf_to_proxy(uint16_t id, uint16_t sfn_sf)
{
    instance->pack_and_send_downlink_sfn_sf_msg(id, sfn_sf);
}

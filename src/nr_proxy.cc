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

#include <sys/stat.h>
#include <sstream>
#include "nr_proxy.h"
#include "nfapi_pnf.h"

namespace
{
    Multi_UE_NR_Proxy *instance;
}

Multi_UE_NR_Proxy::Multi_UE_NR_Proxy(int num_of_ues,  std::string gnb_ip, std::string proxy_ip, std::string ue_ip)
{
    assert(instance == NULL);
    instance = this;
    num_ues = num_of_ues ;

    configure(gnb_ip, proxy_ip, ue_ip);

    oai_slot_init();
}

void Multi_UE_NR_Proxy::start(softmodem_mode_t softmodem_mode)
{
    pthread_t thread;

    configure_nr_nfapi_pnf(vnf_ipaddr.c_str(), vnf_p5port, pnf_ipaddr.c_str(), pnf_p7port, vnf_p7port);

    if (pthread_create(&thread, NULL, &oai_slot_task, (void *)softmodem_mode) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "pthread_create failed for calling oai_slot_task");
    }

    for (int i = 0; i < num_ues; i++)
    {
        threads.push_back(std::thread(&Multi_UE_NR_Proxy::receive_message_from_nr_ue, this, i));
    }
    for (auto &th : threads)
    {
        if(th.joinable())
        {
            th.join();
        }
    }
}

void Multi_UE_NR_Proxy::configure(std::string gnb_ip, std::string proxy_ip, std::string ue_ip)
{
    oai_ue_ipaddr = ue_ip;
    vnf_ipaddr = gnb_ip;
    pnf_ipaddr = proxy_ip;
    vnf_p5port = 50601;
    vnf_p7port = 50611;
    pnf_p7port = 50610;

    std::cout<<"VNF is on IP Address "<<vnf_ipaddr<<std::endl;
    std::cout<<"PNF is on IP Address "<<pnf_ipaddr<<std::endl;
    std::cout<<"OAI-UE is on IP Address "<<oai_ue_ipaddr<<std::endl;

    for (int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        int oai_rx_ue_port = 3611 + ue_idx * port_delta;
        int oai_tx_ue_port = 3612 + ue_idx * port_delta;
        init_oai_socket(oai_ue_ipaddr.c_str(), oai_tx_ue_port, oai_rx_ue_port, ue_idx);
    }
}

int Multi_UE_NR_Proxy::init_oai_socket(const char *addr, int tx_port, int rx_port, int ue_idx)
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
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "rx addr: %s, rx port: %d", addr, rx_port);
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
        NFAPI_TRACE(NFAPI_TRACE_DEBUG, "tx addr: %s, tx port: %d", addr, tx_port);
    }
    return 0;
}

void Multi_UE_NR_Proxy::receive_message_from_nr_ue(int ue_idx)
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
            uint16_t sfn_slot = nfapi_get_sfnslot(buffer, buflen);
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(Proxy) Proxy has received %d uplink message from OAI UE at socket. Frame: %d, Slot: %d",
                    header.message_id, NFAPI_SFNSLOT2SFN(sfn_slot), NFAPI_SFNSLOT2SLOT(sfn_slot));
        }
        oai_slot_handle_msg_from_ue(buffer, buflen, ue_idx + 2);
    }
}

void Multi_UE_NR_Proxy::oai_gnb_downlink_nfapi_task(void *msg_org)
{
    lock_guard_t lock(mutex);

    char buffer[NFAPI_MAX_PACKED_MESSAGE_SIZE];
    int encoded_size = nfapi_nr_p7_message_pack(msg_org, buffer, sizeof(buffer), nullptr);
    if (encoded_size <= 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_nr_p7_message_pack failed");
        return;
    }

    union
    {
        nfapi_p7_message_header_t header;
        nfapi_nr_dl_tti_request_t dl_tti_request;
        nfapi_nr_tx_data_request_t tx_data_req;
        nfapi_nr_ul_dci_request_t ul_dci_req;
        nfapi_nr_ul_tti_request_t ul_tti_request;
    } msg;

    if (nfapi_nr_p7_message_unpack((void *)buffer, encoded_size, &msg, sizeof(msg), NULL) != 0)
    {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "nfapi_nr_p7_message_unpack failed NEM ID: %d", 1);
        return;
    }

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_.sin_port = htons(3612 + ue_idx * port_delta);
        switch (msg.header.message_id)
        {

        case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
        {
            int dl_sfn = msg.dl_tti_request.SFN;
            int dl_slot = msg.dl_tti_request.Slot;
            uint16_t dl_numPDU = msg.dl_tti_request.dl_tti_request_body.nPDUs;
            NFAPI_TRACE(NFAPI_TRACE_INFO , "(Proxy UE) Prior to sending dl_tti_req to OAI UE. Frame: %d,"
                       " Slot: %d, Number of PDUs: %u",
                       dl_sfn, dl_slot, dl_numPDU);
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST forwarded from Proxy to UE");
            }
            break;
        }

        case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST forwarded from Proxy to UE");
            }
            break;

        case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST forwarded from Proxy to UE");
            }
            break;

        case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
            assert(ue_tx_socket[ue_idx] > 2);
            if (sendto(ue_tx_socket[ue_idx], buffer, encoded_size, 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
            {
                NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST to OAI UE failed");
            }
            else
            {
                NFAPI_TRACE(NFAPI_TRACE_INFO , "NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST forwarded from Proxy to UE");
            }
            break;

        default:
            NFAPI_TRACE(NFAPI_TRACE_INFO , "Unhandled message at Proxy message_id: %u", msg.header.message_id);
            break;
        }
    }
}

void Multi_UE_NR_Proxy::pack_and_send_downlink_sfn_slot_msg(uint16_t sfn_slot)
{
    lock_guard_t lock(mutex);

    for(int ue_idx = 0; ue_idx < num_ues; ue_idx++)
    {
        address_tx_.sin_port = htons(3612 + ue_idx * port_delta);
        assert(ue_tx_socket[ue_idx] > 2);
        if (sendto(ue_tx_socket[ue_idx], &sfn_slot, sizeof(sfn_slot), 0, (const struct sockaddr *) &address_tx_, sizeof(address_tx_)) < 0)
        {
            int sfn = NFAPI_SFNSLOT2SFN(sfn_slot);
            int slot = NFAPI_SFNSLOT2SLOT(sfn_slot);
            NFAPI_TRACE(NFAPI_TRACE_ERROR, "Send sfn_slot to OAI UE FAIL Frame: %d,Slot: %d", sfn, slot);
        }
    }
}

void transfer_downstream_nfapi_msg_to_nr_proxy(void *msg)
{
    instance->oai_gnb_downlink_nfapi_task(msg);
}

void transfer_downstream_sfn_slot_to_proxy(uint16_t sfn_slot)
{
    instance->pack_and_send_downlink_sfn_slot_msg(sfn_slot);
}

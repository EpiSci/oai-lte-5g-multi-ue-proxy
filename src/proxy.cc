#include <sys/stat.h>
#include <sstream>
#include "proxy.h"
#include "lte_proxy.h"
#include "nr_proxy.h"

extern "C"
{
    int num_ues = 1;
}

void usage();
bool is_Numeric(char number[]);
void init_log_file();

int main(int argc, char *argv[])
{
    int ues = 0;
    std::string oai_ue_ipaddr = "127.0.0.1";
    std::string enb_ipaddr = "127.0.0.1";
    std::string gnb_ipaddr = "127.0.0.2";
    std::string proxy_ipaddr = "127.0.0.1";
    if (argc < 2)
    {
        ues = 1;
    }
    else if((argc >= 2) && (is_Numeric(argv[1])))
    {
        ues = atoi(argv[1]);
    }
    if (argc == 5) // lte or nr case
    {
        enb_ipaddr = argv[2];
        gnb_ipaddr = argv[2];
        proxy_ipaddr = argv[3];
        oai_ue_ipaddr = argv[4];
    }
    else if (argc == 6) // nsa case
    {
        enb_ipaddr = argv[2];
        gnb_ipaddr = argv[3];
        proxy_ipaddr = argv[4];
        oai_ue_ipaddr = argv[5];
    }
    if ( ues > 0 )
    {
        init_log_file();

        Multi_UE_Proxy lte_proxy(ues, enb_ipaddr, proxy_ipaddr, oai_ue_ipaddr);
        Multi_UE_NR_Proxy nr_proxy(ues, gnb_ipaddr, proxy_ipaddr, oai_ue_ipaddr);

        std::thread lte_th( &Multi_UE_Proxy::start, &lte_proxy);
        std::thread nr_th( &Multi_UE_NR_Proxy::start, &nr_proxy);

        lte_th.join();
        nr_th.join();
    }
    else
    {
        usage();
    }
    return EXIT_SUCCESS;
}

void usage()
{
    std::cout<<"usage: ./proxy number_of_UEs eNB_IP_addr proxy_IP_addr UE_IP_addr"<<std::endl;
    std::cout<<"  Mandatory:"<<std::endl;
    std::cout<<"       number_of_UEs needs to be a positive interger, with default number_of_UEs = 1."<<std::endl;
    std::cout<<"  Optional: (if not used, loopback will be used)"<<std::endl;
    std::cout<<"       eNB_IP_addr (gNB_IP_addr in NR standalone mode) shall be a valid IP address"<<std::endl;
    std::cout<<"       gNB_IP_addr shall be a valid IP address in NR non-standalone mode. Otherwise, it should be omitted"<<std::endl;
    std::cout<<"       proxy_IP_addr shall be a valid IP address"<<std::endl;
    std::cout<<"       UE_IP_addr shall be a valid IP address"<<std::endl;
}

bool is_Numeric(char number[]) 
{
    for (int i = 0; number[i] != 0; i++)
    {
        if(!isdigit(number[i]))
        {
            return false;
        }
    }
    return true;
}  

inline bool exists (const std::string& filename)
{
    struct stat buffer;
    return (stat (filename.c_str(), &buffer) == 0);
}

void init_log_file()
{
    static const char log_name[] = "nfapi.log";

    if (exists(log_name))
    {
        if (remove(log_name) != 0 && errno != ENFILE)
        {
            perror(log_name);
        }
    }
}

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
bool valid_IpAddress(std::string &ipAddress);

int main(int argc, char *argv[])
{
    int ues = 0;
    softmodem_mode_t softmodem_mode = SOFTMODEM_LTE;
    bool valid_ipaddr_args = true;
    std::string ue_ipaddr = "127.0.0.1";
    std::string enb_ipaddr = "127.0.0.1";
    std::string gnb_ipaddr = "127.0.0.2";
    std::string proxy_ipaddr = "127.0.0.1";
    if (argc < 2)
    {
        ues = 1;
    }
    else if (argc >= 2 && is_Numeric(argv[1]))
    {
        ues = atoi(argv[1]);
    }
    if (argc >= 3)
    {
        std::vector<std::string> ipaddrs;
        for (int i = 2; i < argc ; ++i)
        {
            std::string arg = argv[i];
            if (arg == "--lte")
            {
                softmodem_mode = SOFTMODEM_LTE;
            }
            else if (arg == "--nr")
            {
                softmodem_mode = SOFTMODEM_NR;
            }
            else if (arg == "--nsa")
            {
                softmodem_mode = SOFTMODEM_NSA;
            }
            else if (valid_IpAddress(arg))
            {
                ipaddrs.push_back(arg);
            }
            else
            {
                valid_ipaddr_args = false;
                break;
            }
        }
        if (ipaddrs.size() == 3) // lte or nr case
        {
            enb_ipaddr = ipaddrs[0];
            gnb_ipaddr = ipaddrs[0];
            proxy_ipaddr = ipaddrs[1];
            ue_ipaddr = ipaddrs[2];
        }
        else if (ipaddrs.size() == 4) // nsa case
        {
            enb_ipaddr = ipaddrs[0];
            gnb_ipaddr = ipaddrs[1];
            proxy_ipaddr = ipaddrs[2];
            ue_ipaddr = ipaddrs[3];
        }
        else if (ipaddrs.size() > 0)
        {
            valid_ipaddr_args = false;
        }
    }
    if (ues > 0 && valid_ipaddr_args)
    {
        init_log_file();

        if (softmodem_mode == SOFTMODEM_LTE)
        {
            Multi_UE_Proxy lte_proxy(ues, enb_ipaddr, proxy_ipaddr, ue_ipaddr);
            lte_proxy.start(softmodem_mode);
        }
        else if (softmodem_mode == SOFTMODEM_NR)
        {
            Multi_UE_NR_Proxy nr_proxy(ues, gnb_ipaddr, proxy_ipaddr, ue_ipaddr);
            nr_proxy.start(softmodem_mode);
        }
        else if (softmodem_mode == SOFTMODEM_NSA)
        {
            Multi_UE_Proxy lte_proxy(ues, enb_ipaddr, proxy_ipaddr, ue_ipaddr);
            Multi_UE_NR_Proxy nr_proxy(ues, gnb_ipaddr, proxy_ipaddr, ue_ipaddr);

            std::thread lte_th( &Multi_UE_Proxy::start, &lte_proxy, softmodem_mode);
            std::thread nr_th( &Multi_UE_NR_Proxy::start, &nr_proxy, softmodem_mode);

            lte_th.join();
            nr_th.join();
        }
    }
    else
    {
        usage();
    }
    return EXIT_SUCCESS;
}

void usage()
{
    std::cout << "usage: ./proxy number_of_UEs eNB_IP_addr proxy_IP_addr UE_IP_addr softmodem_mode\n";
    std::cout << "  Mandatory:\n";
    std::cout << "       number_of_UEs needs to be a positive interger, with default number_of_UEs = 1.";
    std::cout << "  Optional IP address: (if not used, loopback will be used)";
    std::cout << "       eNB_IP_addr (gNB_IP_addr in NR standalone mode) shall be a valid IP address\n";
    std::cout << "       gNB_IP_addr shall be a valid IP address in NR non-standalone mode. Otherwise, it should be omitted\n";
    std::cout << "       proxy_IP_addr shall be a valid IP address\n";
    std::cout << "       UE_IP_addr shall be a valid IP address\n";
    std::cout << "  Optional softmodem_mode: (if not used, --lte will be used)\n";
    std::cout << "       --lte if it runs in lte as a defalt value.\n";
    std::cout << "       --nr if it runs in nr standalone mode.\n";
    std::cout << "       --nsa if it runs in nr non-standalone mode.\n";
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

inline bool exists(const std::string& filename)
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

bool valid_IpAddress(std::string &ipAddress)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
    return result == 1;
}
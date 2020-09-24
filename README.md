# 1.0 Description #
The purpose of this repository is to demonstrate the Multi-UE Proxy capability that allows users to create individualized UEs communicating a single eNB based on the customzied Open Air Interface (OAI) open source software base. The repository of this customzied OAI code will be merged into OAIs develop branch, which is located at: https://gitlab.eurecom.fr/oai/openairinterface5g. These UEs are individual entities and communicate to a single eNB via the bypass PHY layer (wired connection). With this package, various multi-UE scenarios can be tested without the overhead of PHY-layer features of underlying radios. This proxy was created to allow users to test and utilize EpiSci's update version of the openairinterface5g code. The unmerged modified OAI code can be found at: https://github.com/EpiSci/openairinterface5G. The functional description of this multi-UE proxy is described by the follwowing image:
![Open Source Proxy Functional Diagram](https://github.com/EpiSci/oai-lte-multi-ue-proxy/blob/master/functional_diagram.png?raw=true)
## 1.1 Commit where OAI Code was forked ##
The initial OAI code was used as a baseline for this work. The initial framework for this development was started from the follwoing commit. The latest OAI code can be found at the link listed above.
https://gitlab.eurecom.fr/oai/openairinterface5g/-/commit/362da7c9205691a7314de56bbe8ec369f636da7b
## 1.2 Included Features ##
The latest version of the multi-UE proxy includes the following:
- Socket communciation from/to UE(s)
- Socket communcation from/to eNB
- Uplink/Downlink packet queueing
- nFAPI compatibility
- Logging mechanism
- Test scripts

# 2.0 Testing #
## 2.1 Prerequsites ##
    -The modified OAI source code should be installed and built according to https://gitlab.eurecom.fr/oai/openairinterface5g.

## 2.2 Build binary file ##
    2.2.1. Navigate to the proxy directory:
        $ cd oai-lte-multi-ue-proxy
    2.2.2. Compile and build:
        $ cpus=$(grep -c ^processor /proc/cpuinfo)
        $ make -j$cpus

## 2.3 Run overall program ##
    2.3.1. Run the testscript
        oai-lte-multi-ue-proxy$ ./proxy_testscript.py -s Core_PROXY_2_UEs.imn
        - where the -s option is used to specify a particular scenario file.
        - The default scenario file is Core_PROXY_1_UEs.imn which will launch one UE

    2.3.2. Run eNB/Proxy/UE program separately
        oai-lte-multi-ue-proxy$ ./proxy_testscript.py -r enb -t -s Core_PROXY_2_UEs.imn
        oai-lte-multi-ue-proxy$ ./proxy_testscript.py -r proxy -t -s Core_PROXY_2_UEs.imn
        oai-lte-multi-ue-proxy$ ./proxy_testscript.py -r ue -t -s Core_PROXY_2_UEs.imn
        - where the -r option allows the user to select the particular program to run
        - the -r options include [enb, ue, or proxy] as shown above
        - where the -t option allows the user to view all the logs on the current running terminal. (This is optional)

        2.3.2.1 Run a specified UE with -u option
            ./proxy_testscript.py -r ue -t -u 1 -s Core_PROXY_2_UEs.imn
            ./proxy_testscript.py -r ue -t -u 2 -s Core_PROXY_2_UEs.imn

## 2.4 Run program multiple times ##
    2.4.1. Run proxy_testscript 10 times for the Core_PROXY_2_UEs.imn file (2 UE scenario)
        ./run_proxy_num_ues --repeat=10 2
    2.4.2. Run proxy_testscript 20 times for the Core_PROXY_5_UEs.imn file (5 UE scenario)
        ./run_proxy_num_ues --repeat=20 5

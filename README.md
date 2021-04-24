# Description #

This repository contains the Multi-UE Proxy to allow UEs to communicate with a single eNB using the customzied OpenAir Interface (OAI) software.
The OAI code is located at https://gitlab.eurecom.fr/oai/openairinterface5g.
The UEs communicate to the eNB via the bypass PHY layer.
Various multi-UE scenarios can be tested without the overhead of a PHY layer.
This proxy currently requires EpiSci's modified version of the OAI repo available at: https://github.com/EpiSci/openairinterface5G.

The functional description of this multi-UE proxy is shown in the following image:

![Open Source Proxy Functional Diagram](functional_diagram.png)

## Included Features ##

The multi-UE proxy includes the following:

- Socket communciation from/to UE(s)
- Socket communication from/to eNB
- Uplink/downlink packet queueing
- nFAPI compatibility
- Logging mechanism
- Test scripts

## Prerequsites ##

Build and install the EpiSys version of the OAI repository.

## Build the proxy ##

```shell
cd .../oai-lte-multi-ue-proxy
make
```

## Run the proxy ##

```shell
./proxy_testscript.py -s Core_PROXY_2_UEs.imn
```

The `-s` option specifies the scenario file.
The default scenario file is Core_PROXY_1_UEs.imn which launches one UE.

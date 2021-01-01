# multi-ue-proxy
1. Prerequsite
    The oai code paired to this proxy code

2. Build binary file
    1. Move into your proxy folder as following example.
        Shell> cd multi-ue-proxy 
    2. Compile and build as following 
        Shell> cpus=$(grep -c ^processor /proc/cpuinfo)
        Shell> make -j$cpus
    3. To remove old binary
        Shell> make clean

3. Scenario file as a input
    -s option followed by scenario file name specifies the requied input file.
    Default file is Core_PROXY_1_UEs.imn

4. Run overall program
    1. Run overall program
        ./proxy_testscript.py -s Core_PROXY_2_UEs.imn

    2. Run enb program separately
        1. Run without -t option 
            ./proxy_testscript.py -r enb -s Core_PROXY_2_UEs.imn
        2. Run with -t option to see all log on the screen also
            ./proxy_testscript.py -r enb -t -s Core_PROXY_2_UEs.imn

    3. Run proxy program separately
        1. Run without -t option 
            ./proxy_testscript.py -r proxy -s Core_PROXY_2_UEs.imn
        2. Run with -t option to see all log on the screen also
            ./proxy_testscript.py -r proxy -t -s Core_PROXY_2_UEs.imn

    4. Run ue program separately
        1. Run without -t option 
            ./proxy_testscript.py -r ue -s Core_PROXY_2_UEs.imn
        2. Run with -t option to see all logs on the UE numbers of terminal.
            ./proxy_testscript.py -r ue -t -s Core_PROXY_2_UEs.imn
        3. Run specific UE with -u option
            ./proxy_testscript.py -r ue -t -u 1 -s Core_PROXY_2_UEs.imn
            ./proxy_testscript.py -r ue -t -u 2 -s Core_PROXY_2_UEs.imn

5. Run program multiple times
    1. Run proxy_testscript 10 times for the Core_PROXY_2_UEs.imn
        ./run_proxy_num_ues --repeat=10 2
    2. Run proxy_testscript 20 times for the Core_PROXY_5_UEs.imn
        ./run_proxy_num_ues --repeat=20 5

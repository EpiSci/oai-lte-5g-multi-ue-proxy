#!/usr/bin/env python3
#
# Automated tests for running PROXY simulations.
# See `--help` for more information.
#
import os
import sys
import argparse
import logging
import subprocess
import time
import re
import bz2
import signal

WORKSPACE_DIR = os.path.abspath(os.path.join(os.path.dirname(sys.argv[0]), './'))

# ----------------------------------------------------------------------------
# Command line argument parsing

parser = argparse.ArgumentParser(description="""

Automated tests for running PROXY simulations

""")
parser.add_argument('--scenario', '-s', metavar='IMN',
                    default='{}/Core_PROXY_1_UEs.imn'.format(WORKSPACE_DIR),
                    help="""
The CORE scenario .imn file (default: %(default)s)
""")
parser.add_argument('--duration', '-d', metavar='SECONDS', type=int, default=15,
                    help="""
How long to run the test before stopping to examine the logs
""")
parser.add_argument('--ue', '-u', type=int, default=0,
                    help="""
UE id starting from 1""")
parser.add_argument('--sleep', '-e', type=int, default=0,
                    help="""
sleep time for UE at the end of execution in seconds
""")
parser.add_argument('--log-dir', '-l', default=os.environ['HOME'],
                    help="""
Where to store log files
""")
parser.add_argument('--run-mode', '-r', type=str, default='',
                    help="""
enb: run for enb, ue: run for ue, proxy: run for proxy, check: run result
""")
parser.add_argument('--tee', '-t', action='store_true', help="""
Show logging output to the terminal as well as being captured to .log files
""")
parser.add_argument('--no-run', '-n', action='store_true', help="""
Don't run the scenario, only examine the logs in the --log-dir
directory from a previous run of the scenario
""")
parser.add_argument('--debug', action='store_true', help="""
Enable debug logging
""")
OPTS = parser.parse_args()
del parser

logging.basicConfig(level=logging.DEBUG if OPTS.debug else logging.INFO,
                    format='>>> %(name)s: %(levelname)s: %(message)s')
LOGGER = logging.getLogger(os.path.basename(sys.argv[0]))

# ----------------------------------------------------------------------------

def redirect_output(cmd, filename):
    if OPTS.tee:
        cmd += ' 2>&1 | tee {}'.format(filename)
    else:
        cmd += ' >{} 2>&1'.format(filename)
    return cmd

def compress(from_name, to_name=None, remove_original=False):
    if not os.path.exists(from_name):
        LOGGER.warning('No file: %s', from_name)
        return
    if to_name is None:
        to_name = from_name
    if not to_name.endswith('.bz2'):
        to_name += '.bz2'
    LOGGER.info('Compress %s to %s', from_name, to_name)
    with bz2.open(to_name, 'w') as outh, \
         open(from_name, 'rb') as inh:
        while True:
            data = inh.read(10240)
            if not data:
                break
            outh.write(data)
    if remove_original:
        LOGGER.debug('Remove %s', from_name)
        os.remove(from_name)

class CompressJobs:
    """
    Allow multiple invocations of `compress` to run in parallel
    """

    def __init__(self):
        self.kids = []

    def compress(self, from_name, to_name=None, remove_original=False):
        kid = os.fork()
        if kid != 0:
            self.kids.append(kid)
        else:
            LOGGER.debug('in pid %d compress %s...', os.getpid(), from_name)
            compress(from_name, to_name, remove_original)
            LOGGER.debug('in pid %d compress %s...done', os.getpid(), from_name)
            sys.exit()

    def wait(self):
        LOGGER.debug('wait %s...', self.kids)
        failed = []
        for kid in self.kids:
            LOGGER.debug('waitpid %d', kid)
            _pid, status = os.waitpid(kid, 0)
            if status != 0:
                failed.append(kid)
        if failed:
            raise Exception('compression failed: %s', failed)
        LOGGER.debug('wait...done')

class Scenario:
    """
    Represents a PROXY scenario
    """

    def __init__(self, filename):
        self.filename = filename
        self.parse_config()

    def parse_config(self):
        ext = self.filename.split('.')[-1]
        if ext == 'xml':
            self.parse_config_xml()
        elif  ext == 'imn':
            self.parse_config_imn()
        else:
            print("Unknow input file format(imn or xml are supported)")
            exit(0)

    def parse_config_xml(self):
        """
        Scan the scenario file to determine node numbers, etc.
        """
        self.enb_hostname = None
        self.enb_node_id = None
        self.ue_hostname = {}
        self.ue_node_id = {}

        node_re = re.compile(r'^\s*<(\S+) id="(\S+)" name="(\S+)" type=')

        with open(self.filename, 'rt') as filehandler:
            node_id = None
            for line in filehandler:
                match = node_re.match(line)
                if match:
                    node_type = match.group(1)
                    if node_type not in ['network', 'device']:
                        raise Exception('Bad node type: {}'.format(node_type))

                    node_id = int(match.group(2))
                    LOGGER.debug('node_id %r', node_id)

                    hostname = match.group(3)
                    LOGGER.debug('hostname %r', hostname)

                    if not node_id:
                        LOGGER.warning('No node ID for hostname %r', hostname)
                        continue

                    if hostname.lower() in ['proxy', 'lte']:
                        LOGGER.debug('Network node is %s', node_id)
                        continue

                    if hostname.lower() == 'enb':
                        LOGGER.debug('enb node is %s', node_id)
                        self.enb_hostname = hostname
                        self.enb_node_id = node_id
                        continue

                    if hostname.lower().startswith('ue'):
                        ue_number = int(hostname[2:])
                        LOGGER.debug('UE %d is node_id %s', ue_number, node_id)
                        self.ue_hostname[ue_number] = hostname
                        self.ue_node_id[ue_number] = node_id
                        continue

                    LOGGER.warning('Skipping unknown hostname %r of node_id %r', hostname, node_id)
                    continue

                LOGGER.debug('Unmatched line %r', line)

        if not self.enb_hostname:
            raise Exception('No eNB node in scenario ' + self.filename)

        if not self.ue_hostname:
            raise Exception('No UE nodes in scenario ' + self.filename)


    def parse_config_imn(self):
        """
        Scan the scenario file to determine node numbers, etc.
        """
        self.enb_hostname = None
        self.enb_node_id = None
        self.ue_hostname = {}
        self.ue_node_id = {}

        node_re = re.compile(r'^\s*node\s+(\S+)')
        hostname_re = re.compile(r'^\s*hostname\s+(\S+)')

        with open(self.filename, 'rt') as filehandler:
            node_id = None
            for line in filehandler:
                match = node_re.match(line)
                if match:
                    node_id = match.group(1)
                    if not node_id.startswith('n'):
                        raise Exception('Bad node ID: {}'.format(node_id))
                    node_id = int(node_id[1:])
                    LOGGER.debug('node_id %r', node_id)
                    continue

                match = hostname_re.match(line)
                if match:
                    hostname = match.group(1)
                    LOGGER.debug('hostname %r', hostname)

                    if not node_id:
                        LOGGER.warning('No node ID for hostname %r', hostname)
                        continue

                    if hostname.lower() in ['proxy', 'lte']:
                        LOGGER.debug('Network node is %s', node_id)
                        continue

                    if hostname.lower() == 'enb':
                        LOGGER.debug('enb node is %s', node_id)
                        self.enb_hostname = hostname
                        self.enb_node_id = node_id
                        continue

                    if hostname.lower().startswith('ue'):
                        ue_number = int(hostname[2:])
                        LOGGER.debug('UE %d is node_id %s', ue_number, node_id)
                        self.ue_hostname[ue_number] = hostname
                        self.ue_node_id[ue_number] = node_id
                        continue

                    LOGGER.warning('Skipping unknown hostname %r of node_id %r', hostname, node_id)
                    continue

                LOGGER.debug('Unmatched line %r', line)

        if not self.enb_hostname:
            raise Exception('No eNB node in scenario ' + self.filename)

        if not self.ue_hostname:
            raise Exception('No UE nodes in scenario ' + self.filename)

    def run(self, ue_id):
        """
        Run the scenario simulation
        """
        PROXY_DIR = os.environ['PWD']

        if OPTS.run_mode in ['enb', '']:
            log_name = '{}/eNB.log'.format(OPTS.log_dir)
            LOGGER.info('Launch eNB: %s', log_name)
            cmd = 'NODE_NUMBER=1 {WORKSPACE_DIR}/run-oai' \
                  .format(WORKSPACE_DIR=PROXY_DIR)
            enb_proc = subprocess.Popen(redirect_output(cmd, log_name), shell=True)

            # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
            # When nodes start at the same time, occasionally eNB will only recognize one UE
            # I think this bug has been fixed -- the random number generator initializer issue
            time.sleep(2)

        if OPTS.run_mode in ['proxy', '']:
            log_name = '{}/nfapi.log'.format(OPTS.log_dir)
            LOGGER.info('Launch Proxy: %s', log_name)
            cmd = '{WORKSPACE_DIR}/build/proxy {NUM_UES}' \
                  .format(WORKSPACE_DIR=PROXY_DIR, NUM_UES=len(self.ue_hostname))
            subprocess.Popen(redirect_output(cmd, log_name), shell=True)
            time.sleep(2)

        if OPTS.run_mode in ['ue', '']:
            if ue_id != None:
                # If ue_id is given, other ues will be ignored.
                hostname = self.ue_hostname[ue_id]
                node_id = self.ue_node_id[ue_id]
                self.ue_hostname = {}
                self.ue_node_id = {}
                self.ue_hostname[ue_id] = hostname
                self.ue_node_id[ue_id] = node_id

            #print (self.ue_hostname)
            ue_proc = {}
            for ue_number, ue_hostname in self.ue_hostname.items():
                log_name = '{}/{}.log'.format(OPTS.log_dir, ue_hostname)
                LOGGER.info('Launch UE%d: %s', ue_number, log_name)
                cmd = 'NODE_NUMBER={NODE_ID} {WORKSPACE_DIR}/run-oai' \
                      .format(NODE_ID=self.ue_node_id[ue_number], WORKSPACE_DIR=PROXY_DIR)
                output_cmd = redirect_output(cmd, log_name)
                if OPTS.run_mode == 'ue' and OPTS.tee:
                    output_cmd = 'gnome-terminal -- sh -c "{}"'.format(output_cmd)
                ue_proc[ue_number] = subprocess.Popen(output_cmd, stdin=subprocess.PIPE, shell=True)
                # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
                # When nodes start at the same time, occasionally eNB will only recognize one UE
                time.sleep(1)

        time.sleep(OPTS.duration)

        passed = True

        if OPTS.run_mode in ['enb', '']:
            # See if the eNB processes crashed
            status = enb_proc.poll()
            if status is None:
                LOGGER.info('eNB process is still running, which is good')
            else:
                passed = False
                LOGGER.critical('eNB process ended early: %r', status)

        if OPTS.run_mode in ['ue', '']:
            # See if the UE processes crashed
            for ue_number in self.ue_hostname:
                status = ue_proc[ue_number].poll()
                if status is None:
                    LOGGER.info('UE%d process is still running, which is good', ue_number)
                else:
                    passed = False
                    LOGGER.critical('UE%d process ended early: %r', ue_number, status)

        # Kill off the main simulation processes
        LOGGER.info('kill main simulation processes...')
        if OPTS.run_mode == '':
            subprocess.run(['sudo', 'killall', 'lte-softmodem', 'lte-uesoftmodem', 'proxy'])
        elif OPTS.run_mode == 'enb':
            subprocess.run(['sudo', 'killall', 'lte-softmodem'])
        elif OPTS.run_mode == 'proxy':
            subprocess.run(['sudo', 'killall', 'proxy'])
        elif OPTS.run_mode == 'ue':
            if OPTS.tee:
                for ue_numberin in self.ue_hostname:
                    ue_proc[ue_number].send_signal(signal.SIGINT)
            else:
                subprocess.run(['sudo', 'killall', 'lte-uesoftmodem'])
        LOGGER.info('kill main simulation processes...done')


        if OPTS.run_mode in ['proxy', '']:
            # Save Proxy nFAPI eNB and UE logs.
            jobs = CompressJobs()
            jobs.compress('{}/nfapi.log'.format(PROXY_DIR),
                          '{}/nfapi-eNB.log'.format(OPTS.log_dir))
            for ue_number, ue_hostname in self.ue_hostname.items():
                jobs.compress('{}/nfapi.log'.format(PROXY_DIR),
                              '{}/nfapi-{}.log'.format(OPTS.log_dir, ue_hostname))
            jobs.wait()

        if OPTS.run_mode in ['enb', 'ue', '']:
            # Compress eNB.log and UE*.log
            jobs = CompressJobs()
            if OPTS.run_mode in ['enb', '']:
                jobs.compress('{}/eNB.log'.format(OPTS.log_dir), remove_original=True)
            if OPTS.run_mode in ['ue', '']:
                for ue_hostname in self.ue_hostname.values():
                    jobs.compress('{}/{}.log'.format(OPTS.log_dir, ue_hostname), remove_original=True)
            jobs.wait()

        return passed

def get_analysis_messages(filename):
    """
    Scan the log file to yield LOG_A message
    """
    LOGGER.info('Scanning %s', filename)
    with bz2.open(filename, 'rt') as filehandler:
        for line in filehandler:
            # Log messages have a header like the following:
            # '4789629.289157 00018198 [MAC] A Message...'
            # The 'A' indicates this is a LOG_A message intended for automated analysis.
            fields = line.split(maxsplit=4)
            if len(fields) == 5 and fields[3] == 'A':
                yield line

def main():
    scenario = Scenario(OPTS.scenario)

    LOGGER.info('eNB node number %d, UE node number%s %s',
                scenario.enb_node_id,
                '' if len(scenario.ue_node_id) == 1 else 's',
                ' '.join(map(str, scenario.ue_node_id.values())))

    passed = True

    if OPTS.run_mode in ['enb', 'ue', 'proxy', '']:
        if not OPTS.no_run:
            arg = int(OPTS.ue) if int(OPTS.ue) > 0 else None
            passed = scenario.run(arg)

    if OPTS.run_mode in ['check', ''] or OPTS.no_run:
        # Examine the logs to determine if the test passed

        # --- eNB log file ---
        found = set()
        for line in get_analysis_messages('{}/eNB.log.bz2'.format(OPTS.log_dir)):
            # 94772.731183 [MAC] A Configuring MIB for instance 0, CCid 0 : (band
            # 7,N_RB_DL 50,Nid_cell 0,p 1,DL freq 2685000000,phich_config.resource 0,
            # phich_config.duration 0)
            if '[MAC] A Configuring MIB ' in line:
                found.add('configured')
                continue

            # 94777.679273 [MAC] A [eNB 0][RAPROC] CC_id 0 Frame 74, subframeP 3:
            # Generating Msg4 with RRC Piggyback (RNTI a67)
            if 'Generating Msg4 with RRC Piggyback' in line:
                found.add('msg4')
                continue

            # 94777.695277 [RRC] A [FRAME 00000][eNB][MOD 00][RNTI a67] [RAPROC]
            # Logical Channel UL-DCCH, processing LTE_RRCConnectionSetupComplete
            # from UE (SRB1 Active)
            if 'processing LTE_RRCConnectionSetupComplete from UE ' in line:
                found.add('setup')
                continue

            # 94776.725081 [RRC] A got UE capabilities for UE 6860
            match = re.search(r'\[RRC\] A got UE (capabilities for UE \w+)$', line)
            if match:
                found.add(match.group(1))
                continue

        LOGGER.debug('found: %r', found)

        num_ues = len(scenario.ue_hostname)
        LOGGER.debug('num UEs: %d', num_ues)

        if len(found) == 3 + num_ues:
            LOGGER.info('All eNB Tests Passed')
        else:
            passed = False
            LOGGER.error('!!! eNB Test Failed !!! -- found %r', found)

        # --- UE log files ---
        for ue_number in scenario.ue_hostname:
            found = set()
            for line in get_analysis_messages('{}/UE{}.log.bz2'.format(OPTS.log_dir, ue_number)):
                # 94777.660529 [MAC] A RACH_IND sent to Proxy, Size: 35 Frame 72 Subframe 1
                if '[MAC] A RACH_IND sent to Proxy' in line:
                    found.add('rach sent')
                    continue

                # 94777.669610 [MAC] A [UE 0][RAPROC] Frame 73 Received RAR
                # (50|00.00.05.4c.0a.67) for preamble 16/16
                if ' Received RAR ' in line:
                    found.add('received rar')
                    continue

                # 94777.679744 [RRC] A [UE0][RAPROC] Frame 74 : Logical Channel DL-CCCH
                # (SRB0), Received RRCConnectionSetup RNTI a67
                if 'Received RRCConnectionSetup' in line:
                    found.add('received setup')
                    continue

                # 94777.706964 [RRC] A UECapabilityInformation Encoded 148 bits
                # (19 bytes)
                if '[RRC] A UECapabilityInformation Encoded ' in line:
                    found.add('capabilities')
                    continue

            LOGGER.debug('found: %r', found)

            if len(found) == 4:
                LOGGER.info('All UE%d Tests Passed', ue_number)
            else:
                passed = False
                LOGGER.error('!!! UE%d Test Failed !!! -- found %r', ue_number, found)

        if not passed:
            LOGGER.critical('FAILED')
            return 1

        LOGGER.info('PASSED')

    return 0

sys.exit(main())

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
import glob
import bz2
import signal
import platform

WORKSPACE_DIR = os.path.abspath(os.path.dirname(sys.argv[0]))

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
parser.add_argument('--no-run', '-n', action='store_true', help="""
Don't run the scenario, only examine the logs in the --log-dir
directory from a previous run of the scenario
""")
parser.add_argument('--nsa', action='store_true', help="""
Enable NSA (LTE and 5G) mode
""")
parser.add_argument('--nr', '--5g', '-5', action='store_true', help="""
Enable NR/5G mode
""")
parser.add_argument('--nfapi-trace-level', '-N',
                    choices='none error warn note info debug'.split(),
                    help="""
Set the NFAPI trace level
""")
parser.add_argument('--debug', action='store_true', help="""
Enable debug logging (for this script only)
""")
OPTS = parser.parse_args()
del parser

if OPTS.nr and OPTS.nsa:
    raise Exception('Cannot do --nr and --nsa together')

logging.basicConfig(level=logging.DEBUG if OPTS.debug else logging.INFO,
                    format='>>> %(name)s: %(levelname)s: %(message)s')
LOGGER = logging.getLogger(os.path.basename(sys.argv[0]))

if OPTS.nfapi_trace_level:
    os.environ['NFAPI_TRACE_LEVEL'] = OPTS.nfapi_trace_level

# ----------------------------------------------------------------------------

def redirect_output(cmd, filename):
    cmd += ' >{} 2>&1'.format(filename)
    return cmd

def compress(from_name, to_name=None, remove_original=False):
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
        if not os.path.exists(from_name):
            # It's not necessarily an error if the log file does not exist.
            # For example, if nfapi_trace never gets invoked (e.g., because
            # NFAPI_TRACE_LEVEL is set to none), then the log file nfapi.log
            # will not get created.
            LOGGER.warning('No file: %s', from_name)
            return
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
        """
        Scan the scenario file to determine node numbers, etc.
        """
        self.enb_hostname = None
        self.enb_node_id = None
        self.ue_hostname = {}
        self.ue_node_id = {}
        self.gnb_hostname = None
        self.gnb_node_id = None
        self.nrue_hostname = {}
        self.nrue_node_id = {}

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

                    if hostname.lower() == 'gnb':
                        LOGGER.debug('gnb node is %s', node_id)
                        self.gnb_hostname = hostname
                        self.gnb_node_id = node_id
                        continue

                    if hostname.lower().startswith('nrue'):
                        nrue_number = int(hostname[4:])
                        LOGGER.debug('NRUE %d is node_id %s', nrue_number, node_id)
                        self.nrue_hostname[nrue_number] = hostname
                        self.nrue_node_id[nrue_number] = node_id
                        continue

                    if hostname.lower().startswith('nsaue'):
                        nsaue_number = int(hostname[5:])
                        LOGGER.debug('NSA-UE %d is node_id %s', nsaue_number, node_id)
                        self.ue_hostname[nsaue_number] = hostname + '-lte'
                        self.ue_node_id[nsaue_number] = node_id
                        self.nrue_hostname[nsaue_number] = hostname + '-nr'
                        self.nrue_node_id[nsaue_number] = node_id
                        continue

                    LOGGER.warning('Skipping unknown hostname %r of node_id %r', hostname, node_id)
                    continue

                LOGGER.debug('Unmatched line %r', line)

        if OPTS.nsa:
            if not self.enb_hostname:
                raise Exception('No eNB node in scenario ' + self.filename)
            if not self.gnb_hostname:
                raise Exception('No gNB node in scenario ' + self.filename)
            if not self.nrue_hostname or not self.ue_hostname:
                raise Exception('No NSA node in scenario ' + self.filename)
        elif OPTS.nr:
            if not self.gnb_hostname:
                raise Exception('No gNB node in scenario ' + self.filename)
            if not self.nrue_hostname:
                raise Exception('No nr-UE node in scenario ' + self.filename)
            if self.enb_hostname:
                raise Exception('Unexpected eNB node in scenario ' + self.filename)
            if self.ue_hostname:
                raise Exception('Unexpected UE node in scenario ' + self.filename)
        else:
            if not self.enb_hostname:
                raise Exception('No eNB node in scenario ' + self.filename)
            if not self.ue_hostname:
                raise Exception('No UE node in scenario ' + self.filename)
            if self.gnb_hostname:
                raise Exception('Unexpected gNB node in scenario ' + self.filename)
            if self.nrue_hostname:
                raise Exception('Unexpected nr-UE node in scenario ' + self.filename)

    def launch_enb(self):
        log_name = '{}/eNB.log'.format(OPTS.log_dir)
        LOGGER.info('Launch eNB: %s', log_name)
        cmd = 'NODE_NUMBER=1 {WORKSPACE_DIR}/run-oai enb' \
              .format(WORKSPACE_DIR=WORKSPACE_DIR)
        if OPTS.nsa:
              cmd += ' --nsa'
        proc = subprocess.Popen(redirect_output(cmd, log_name), shell=True)

        # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
        # When nodes start at the same time, occasionally eNB will only recognize one UE
        # I think this bug has been fixed -- the random number generator initializer issue
        time.sleep(2)

        return proc

    def launch_proxy(self):
        log_name = '{}/nfapi.log'.format(OPTS.log_dir)
        LOGGER.info('Launch Proxy: %s', log_name)
        cmd = '{WORKSPACE_DIR}/build/proxy {NUM_UES} {SOFTMODEM_MODE}' \
              .format(WORKSPACE_DIR=WORKSPACE_DIR, NUM_UES=len(self.ue_hostname), \
                      SOFTMODEM_MODE='--nsa' if OPTS.nsa else '--nr' if OPTS.nr else '--lte')
        proc = subprocess.Popen(redirect_output(cmd, log_name), shell=True)
        time.sleep(2)

        return proc

    def launch_ue(self):
        procs = {}
        for num, hostname in self.ue_hostname.items():
            log_name = '{}/{}.log'.format(OPTS.log_dir, hostname)
            LOGGER.info('Launch UE%d: %s', num, log_name)
            cmd = 'NODE_NUMBER={NODE_ID} {WORKSPACE_DIR}/run-oai ue' \
                  .format(NODE_ID=self.ue_node_id[num], WORKSPACE_DIR=WORKSPACE_DIR)
            if OPTS.nsa:
                cmd += ' --nsa'
            output_cmd = redirect_output(cmd, log_name)
            procs[num] = subprocess.Popen(output_cmd, stdin=subprocess.PIPE, shell=True)
            # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
            # When nodes start at the same time, occasionally eNB will only recognize one UE
            time.sleep(1)

        return procs

    def launch_gnb(self):
        log_name = '{}/gNB.log'.format(OPTS.log_dir)
        LOGGER.info('Launch gNB: %s', log_name)
        cmd = 'NODE_NUMBER=0 {WORKSPACE_DIR}/run-oai gnb' \
              .format(WORKSPACE_DIR=WORKSPACE_DIR)
        proc = subprocess.Popen(redirect_output(cmd, log_name), shell=True)

        # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
        # When nodes start at the same time, occasionally eNB will only recognize one UE
        # I think this bug has been fixed -- the random number generator initializer issue
        time.sleep(2)

        return proc

    def launch_nrue(self):
        procs = {}
        for num, hostname in self.nrue_hostname.items():
            log_name = '{}/{}.log'.format(OPTS.log_dir, hostname)
            LOGGER.info('Launch nrUE%d: %s', num, log_name)
            cmd = 'NODE_NUMBER={NODE_ID} {WORKSPACE_DIR}/run-oai nrue' \
                  .format(NODE_ID=len(self.ue_node_id)+self.ue_node_id[num], WORKSPACE_DIR=WORKSPACE_DIR)
            if OPTS.nsa:
                cmd += ' --nsa'
            output_cmd = redirect_output(cmd, log_name)
            procs[num] = subprocess.Popen(output_cmd, stdin=subprocess.PIPE, shell=True)
            # TODO: Sleep time needed so eNB and UEs don't start at the exact same time
            # When nodes start at the same time, occasionally eNB will only recognize one UE
            time.sleep(1)

        return procs

    def run(self, ue_id):
        """
        Run the scenario simulation
        """

        enb_proc = None
        proxy_proc = None
        ue_proc = {}
        gnb_proc = None
        nrue_proc = {}

        # ------------------------------------------------------------------------------------
        # Launch the softmodem processes

        if self.enb_hostname:
            enb_proc = self.launch_enb()

        if self.gnb_hostname:
            gnb_proc = self.launch_gnb()

        proxy_proc = self.launch_proxy()

        if self.ue_hostname:
            ue_proc = self.launch_ue()

        if self.nrue_hostname:
            nrue_proc = self.launch_nrue()

        # ------------------------------------------------------------------------------------

        time.sleep(OPTS.duration)

        passed = True

        if enb_proc:
            # See if the eNB crashed
            status = enb_proc.poll()
            if status is None:
                LOGGER.info('eNB process is still running, which is good')
            else:
                passed = False
                LOGGER.critical('eNB process ended early: %r', status)

        if proxy_proc:
            # See if the proxy crashed
            status = proxy_proc.poll()
            if status is None:
                LOGGER.info('proxy process is still running, which is good')
            else:
                passed = False
                LOGGER.critical('proxy process ended early: %r', status)

        if ue_proc:
            # See if the UE processes crashed
            for ue_number in self.ue_hostname:
                status = ue_proc[ue_number].poll()
                if status is None:
                    LOGGER.info('UE%d process is still running, which is good', ue_number)
                else:
                    passed = False
                    LOGGER.critical('UE%d process ended early: %r', ue_number, status)

        if gnb_proc:
            # See if the gNB processes crashed
            status = gnb_proc.poll()
            if status is None:
                LOGGER.info('gNB process is still running, which is good')
            else:
                passed = False
                LOGGER.critical('gNB process ended early: %r', status)

        if nrue_proc:
            # See if the NRUE processes crashed
            for nrue_number in self.nrue_hostname:
                status = nrue_proc[nrue_number].poll()
                if status is None:
                    LOGGER.info('NRUE%d process is still running, which is good', nrue_number)
                else:
                    passed = False
                    LOGGER.critical('NRUE%d process ended early: %r', nrue_number, status)

        LOGGER.info('kill main simulation processes...')
        all_procs = ['proxy']
        if enb_proc:
            all_procs.append('lte-softmodem')
        if gnb_proc:
            all_procs.append('nr-softmodem')
        if ue_proc:
            all_procs.append('lte-uesoftmodem')
        if nrue_proc:
            all_procs.append('nr-uesoftmodem')
        subprocess.run(['sudo', 'killall'] + all_procs)
        proxy_proc.wait()
        if enb_proc:
            enb_proc.wait()
        if gnb_proc:
            gnb_proc.wait()
        for proc in ue_proc.values():
            proc.wait()
        for proc in nrue_proc.values():
            proc.wait()
        LOGGER.info('kill main simulation processes...done')

        if proxy_proc:
            # Save Proxy nFAPI eNB and UE logs.
            jobs = CompressJobs()
            jobs.compress('{}/nfapi.log'.format(WORKSPACE_DIR),
                          '{}/nfapi-eNB.log'.format(OPTS.log_dir))
            for ue_number, ue_hostname in self.ue_hostname.items():
                jobs.compress('{}/nfapi.log'.format(WORKSPACE_DIR),
                              '{}/nfapi-{}.log'.format(OPTS.log_dir, ue_hostname))
            jobs.wait()

        # Compress eNB.log, UE*.log, gNB.log and NRUE*.log
        jobs = CompressJobs()
        if enb_proc:
            jobs.compress('{}/eNB.log'.format(OPTS.log_dir), remove_original=True)
        if ue_proc:
            for ue_hostname in self.ue_hostname.values():
                jobs.compress('{}/{}.log'.format(OPTS.log_dir, ue_hostname), remove_original=True)
        if gnb_proc:
            jobs.compress('{}/gNB.log'.format(OPTS.log_dir), remove_original=True)
        if nrue_proc:
            for nrue_hostname in self.nrue_hostname.values():
                jobs.compress('{}/{}.log'.format(OPTS.log_dir, nrue_hostname), remove_original=True)
        jobs.wait()

        if save_core_files():
            passed = False

        return passed

# ----------------------------------------------------------------------------

def get_analysis_messages(filename):
    """
    Find all the LOG_A log messages in the given log file `filename`
    and yield them one by one.  The file is a .bz2 compressed log.
    """
    LOGGER.info('Scanning %s', filename)
    with bz2.open(filename, 'rt') as fh:
        for line in fh:
            line = line.rstrip('\r\n')

            # Log messages have a header like the following:
            # '4789629.289157 00000057 [MAC] A Message...'
            # The 'A' indicates this is a LOG_A message intended for automated analysis.
            #
            # The second field (00000057) is the thread ID.  This field was not
            # present in earlier versions of the OAI code.
            fields = line.split(maxsplit=4)
            if len(fields) == 5 and (fields[2] == 'A' or fields[3] == 'A'):
                yield line

            # Look for -fsanitize=address problems.  But ignore heap leaks for
            # now because there are many due to failure to cleanup on shutdown.
            if 'Sanitizer' in line:
                if 'LeakSanitizer' in line:
                    pass
                elif line.startswith('SUMMARY: '):
                    pass
                else:
                    LOGGER.warning('%s', line)

def set_core_pattern():
    # Set the core_pattern so we can save the core files from any crashes.
    #
    # On Ubuntu, the pattern is normally `|/usr/share/apport/apport %p %s %c %d %P %E`
    #
    # If you want to restore the default pattern, do the following:
    #
    # sudo sh -c 'echo > /proc/sys/kernel/core_pattern "|/usr/share/apport/apport %p %s %c %d %P %E"'
    #
    # Doing that will allow Ubuntu to report any application crashes.
    # Or just reboot.
    #
    # See also the core(5) man page.

    # Remove any existing core files
    core_files = glob.glob('/tmp/coredump-*')
    if core_files:
        subprocess.run(['sudo', 'rm', '-fv'] + core_files)

    pattern_file = '/proc/sys/kernel/core_pattern'
    LOGGER.info('Core pattern was: %r', open(pattern_file, 'rt').read())
    subprocess.run(['sudo', 'sh', '-c',
                    'echo /tmp/coredump-%P > {}'.format(pattern_file)])

def save_core_files():
    core_files = glob.glob('/tmp/coredump-*')
    if not core_files:
        LOGGER.info('No core files')
        return False
    LOGGER.error('Found %d core file%s', len(core_files),
                 '' if len(core_files) == 1 else 's')

    # Core files tend to be owned by root and not readable by others.
    # Change the ownership so we can both read them and then remove them.
    subprocess.run(['sudo', 'chown', str(os.getuid())] + core_files)

    jobs = CompressJobs()
    for core_file in core_files:
        jobs.compress(core_file,
                      '{}/{}'.format(OPTS.log_dir, os.path.basename(core_file)),
                      remove_original=True)
    jobs.wait()

    return True

def main():
    scenario = Scenario(OPTS.scenario)

    if scenario.enb_hostname:
        LOGGER.info('eNB node number %d', scenario.enb_node_id)

    if scenario.gnb_hostname:
        LOGGER.info('gNB node number %d', scenario.gnb_node_id)

    if scenario.ue_hostname:
        LOGGER.info('UE node number%s %s',
                    '' if len(scenario.ue_node_id) == 1 else 's',
                    ' '.join(map(str, scenario.ue_node_id.values())))

    if scenario.nrue_hostname:
        LOGGER.info('nr-UE node number%s %s',
                    '' if len(scenario.nrue_node_id) == 1 else 's',
                    ' '.join(map(str, scenario.nrue_node_id.values())))

    set_core_pattern()

    passed = True

    if not OPTS.no_run:
        arg = int(OPTS.ue) if int(OPTS.ue) > 0 else None
        passed = scenario.run(arg)

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
    num_ue_failed = 0
    for ue_number, ue_hostname in scenario.ue_hostname.items():
        found = set()
        for line in get_analysis_messages('{}/{}.log.bz2'.format(OPTS.log_dir, ue_hostname)):
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
            num_ue_failed += 1
            passed = False
            LOGGER.error('!!! UE%d Test Failed !!! -- found %r', ue_number, found)

    # TODO: Analyze gNB and nr-UE logs

    if not passed:
        LOGGER.critical('FAILED, %d of %d UEs failed',
                        num_ue_failed, len(scenario.ue_hostname))
        return 1

    LOGGER.info('PASSED')

    return 0

sys.exit(main())

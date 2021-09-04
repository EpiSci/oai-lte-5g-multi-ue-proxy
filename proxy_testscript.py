#!/usr/bin/env python3
#
# Automated tests for running proxy simulations.
# See `--help` for more information.
#
import os
import sys
import argparse
import logging
import time
import re
import glob
import bz2
import subprocess
from subprocess import Popen
from typing import Optional, Dict, List, Generator, Union

WORKSPACE_DIR = os.path.abspath(os.path.dirname(sys.argv[0]))

# ----------------------------------------------------------------------------
# Command line argument parsing

parser = argparse.ArgumentParser(description="""

Automated tests for running proxy simulations

""")

parser.add_argument('--num-ues', '-u', metavar='N', type=int, default=1,
                    help="""
The number of UEs to launch (default: %(default)s)
""")

parser.add_argument('--duration', '-d', metavar='SECONDS', type=int, default=30,
                    help="""
How long to run the test before stopping to examine the logs
""")

parser.add_argument('--log-dir', '-l', default='.',
                    help="""
Where to store log files
""")

parser.add_argument('--no-run', '-n', action='store_true', help="""
Don't run the scenario, only examine the logs in the --log-dir
directory from a previous run of the scenario
""")

parser.add_argument('--mode', default='lte', choices='lte nr nsa'.split(),
                    help="""
The kind of simulation scenario to run
(default: %(default)s)
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

logging.basicConfig(level=logging.DEBUG if OPTS.debug else logging.INFO,
                    format='>>> %(name)s: %(levelname)s: %(message)s')
LOGGER = logging.getLogger(os.path.basename(sys.argv[0]))

RUN_OAI = os.path.join(WORKSPACE_DIR, 'run-oai')

if OPTS.nfapi_trace_level:
    os.environ['NFAPI_TRACE_LEVEL'] = OPTS.nfapi_trace_level

# ----------------------------------------------------------------------------

def redirect_output(cmd: str, filename: str) -> str:
    cmd += ' >{} 2>&1'.format(filename)
    return cmd

def compress(from_name: str, to_name: Optional[str]=None, remove_original: bool=False) -> None:
    """
    Compress the file `from_name` and store it as `to_name`.
    `to_name` defaults to `from_name` with `.bz2` appended.
    If `remove_original` is True, removes `from_name` when the compress finishes.
    """
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

    def __init__(self) -> None:
        self.kids: List[int] = []

    def compress(self, from_name: str, to_name: Optional[str]=None, remove_original: bool=False) -> None:
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

    def wait(self) -> None:
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

class NodeIdGenerator:
    id: int = 0

    def __call__(self):
        self.id += 1
        return self.id

class Scenario:
    """
    Represents a proxy scenario
    """

    def __init__(self) -> None:
        self.enb_hostname: Optional[str] = None
        self.enb_node_id: Optional[int] = None
        self.ue_hostname: Dict[int, str] = {}
        self.ue_node_id: Dict[int, int] = {}
        self.gnb_hostname: Optional[str] = None
        self.gnb_node_id: Optional[int] = None
        self.nrue_hostname: Dict[int, str] = {}
        self.nrue_node_id: Dict[int, int] = {}

        # Setup our data structures according to the command-line options

        node_ids = NodeIdGenerator()

        if OPTS.mode == 'nsa':
            # Non-standalone mode: eNB, gNB, UEs and NRUEs
            self.enb_hostname = 'eNB'
            self.enb_node_id = node_ids()
            for i in range(OPTS.num_ues):
                ue_num = i + 1
                node_id = node_ids()
                self.ue_hostname[ue_num] = f'UE{ue_num}'
                self.ue_node_id[ue_num] = node_id
                self.nrue_hostname[ue_num] = f'NRUE{ue_num}'
                self.nrue_node_id[ue_num] = node_id
            self.gnb_hostname = 'gNB'
            self.gnb_node_id = node_ids()

        if OPTS.mode == 'nr':
            # NR mode: gNB and NRUEs, no eNB, no UEs
            self.gnb_hostname = 'gNB'
            self.gnb_node_id = node_ids()
            for i in range(OPTS.num_ues):
                ue_num = i + 1
                self.nrue_hostname[ue_num] = f'NRUE{ue_num}'
                self.nrue_node_id[ue_num] = node_ids()

        if OPTS.mode == 'lte':
            # LTE mode: eNB and UEs, no gNB, no NRUEs
            self.enb_hostname = 'eNB'
            self.enb_node_id = node_ids()
            for i in range(OPTS.num_ues):
                ue_num = i + 1
                self.ue_hostname[ue_num] = f'UE{ue_num}'
                self.ue_node_id[ue_num] = node_ids()

    def launch_enb(self) -> Popen:
        log_name = '{}/eNB.log'.format(OPTS.log_dir)
        LOGGER.info('Launch eNB: %s', log_name)
        cmd = 'NODE_NUMBER=1 {RUN_OAI} enb' \
              .format(RUN_OAI=RUN_OAI)
        if OPTS.mode == 'nsa':
            cmd += ' --nsa'
        proc = Popen(redirect_output(cmd, log_name), shell=True)

        # TODO: Sleep time needed so eNB and UEs don't start at the exact same
        # time When nodes start at the same time, occasionally eNB will only
        # recognize one UE I think this bug has been fixed -- the random
        # number generator initializer issue
        time.sleep(1)

        return proc

    def launch_proxy(self) -> Popen:
        log_name = '{}/nfapi.log'.format(OPTS.log_dir)
        LOGGER.info('Launch Proxy: %s', log_name)
        cmd = 'exec sudo -E {WORKSPACE_DIR}/build/proxy {NUM_UES} {SOFTMODEM_MODE}' \
              .format(WORKSPACE_DIR=WORKSPACE_DIR, NUM_UES=len(self.ue_hostname), \
                      SOFTMODEM_MODE=f'--{OPTS.mode}')
        proc = Popen(redirect_output(cmd, log_name), shell=True)
        time.sleep(2)

        return proc

    def launch_ue(self) -> Dict[int, Popen]:
        procs = {}
        for num, hostname in self.ue_hostname.items():
            log_name = '{}/{}.log'.format(OPTS.log_dir, hostname)
            LOGGER.info('Launch UE%d: %s', num, log_name)
            cmd = 'NODE_NUMBER={NODE_ID} {RUN_OAI} ue' \
                  .format(NODE_ID=self.ue_node_id[num],
                          RUN_OAI=RUN_OAI)
            if OPTS.mode == 'nsa':
                cmd += ' --nsa'
            procs[num] = Popen(redirect_output(cmd, log_name), shell=True)

            # TODO: Sleep time needed so eNB and UEs don't start at the exact
            # same time When nodes start at the same time, occasionally eNB
            # will only recognize one UE
            time.sleep(1)

        return procs

    def launch_gnb(self) -> Popen:
        log_name = '{}/gNB.log'.format(OPTS.log_dir)
        LOGGER.info('Launch gNB: %s', log_name)
        cmd = 'NODE_NUMBER=0 {RUN_OAI} gnb' \
              .format(RUN_OAI=RUN_OAI)
        proc = Popen(redirect_output(cmd, log_name), shell=True)

        # TODO: Sleep time needed so eNB and UEs don't start at the exact same
        # time When nodes start at the same time, occasionally eNB will only
        # recognize one UE I think this bug has been fixed -- the random
        # number generator initializer issue
        time.sleep(1)

        return proc

    def launch_nrue(self) -> Dict[int, Popen]:
        procs = {}
        for num, hostname in self.nrue_hostname.items():
            log_name = '{}/{}.log'.format(OPTS.log_dir, hostname)
            LOGGER.info('Launch nrUE%d: %s', num, log_name)
            cmd = 'NODE_NUMBER={NODE_ID} {RUN_OAI} nrue' \
                  .format(NODE_ID=self.ue_node_id[num],
                          RUN_OAI=RUN_OAI)
            if OPTS.mode == 'nsa':
                cmd += ' --nsa'
            procs[num] = Popen(redirect_output(cmd, log_name), shell=True)

            # TODO: Sleep time needed so eNB and NRUEs don't start at the
            # exact same time When nodes start at the same time, occasionally
            # eNB will only recognize one NRUE
            time.sleep(1)

        return procs

    def run(self) -> bool:
        """
        Run the simulation.
        Return True if the test passes
        """

        enb_proc: Optional[Popen] = None
        proxy_proc: Optional[Popen] = None
        ue_proc: Dict[int, Popen] = {}
        gnb_proc: Optional[Popen] = None
        nrue_proc: Dict[int, Popen] = {}

        # ------------------------------------------------------------------------------------
        # Launch the softmodem processes

        if self.enb_hostname:
            enb_proc = self.launch_enb()

        if self.gnb_hostname:
            gnb_proc = self.launch_gnb()

        proxy_proc = self.launch_proxy()

        if self.nrue_hostname:
            nrue_proc = self.launch_nrue()

        if self.ue_hostname:
            ue_proc = self.launch_ue()

        # ------------------------------------------------------------------------------------
        # Let the simulation run for a while

        time.sleep(OPTS.duration)

        # ------------------------------------------------------------------------------------
        # Analyze the log files to see if the test run passed

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
        cmd = ['sudo', 'killall']
        # TODO: softmodem processes tend to crash while trying to shutdown.
        # They don't actually need to do anything on shutdown, so for now we
        # use -KILL because the softmodem processes can't catch that signal
        # and so they don't get a chance to try to shutdown
        cmd.append('-KILL')
        cmd.append('proxy')
        if enb_proc:
            cmd.append('lte-softmodem')
        if gnb_proc:
            cmd.append('nr-softmodem')
        if ue_proc:
            cmd.append('lte-uesoftmodem')
        if nrue_proc:
            cmd.append('nr-uesoftmodem')
        subprocess.run(cmd)

        # Wait for the processes to end
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
            nfapi_log_file = glob.glob('{}/nfapi.log'.format(WORKSPACE_DIR))
            chown(nfapi_log_file)
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

        return passed

# ----------------------------------------------------------------------------

def get_analysis_messages(filename: str) -> Generator[str, None, None]:
    """
    Find all the LOG_A log messages in the given log file `filename`
    and yield them one by one.  The file is a .bz2 compressed log.
    """
    LOGGER.info('Scanning %s', filename)
    with bz2.open(filename, 'rb') as fh:
        for line_bytes in fh:
            line = line_bytes.decode('utf-8', 'backslashreplace')
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

def chown(files: Union[str, List[str]]) -> None:
    if isinstance(files, str):
        files = [files]
    subprocess.run(['sudo', 'chown', '--changes', str(os.getuid())] + files)

def set_core_pattern() -> None:
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

def save_core_files() -> bool:
    core_files = glob.glob('/tmp/coredump-*')
    if not core_files:
        LOGGER.info('No core files')
        return False
    LOGGER.error('Found %d core file%s', len(core_files),
                 '' if len(core_files) == 1 else 's')

    # Core files tend to be owned by root and not readable by others.
    # Change the ownership so we can both read them and then remove them.
    chown(core_files)

    jobs = CompressJobs()
    for core_file in core_files:
        jobs.compress(core_file,
                      '{}/{}'.format(OPTS.log_dir, os.path.basename(core_file)),
                      remove_original=True)
    jobs.wait()

    return True

def analyze_enb_logs(scenario: Scenario) -> bool:
    if not scenario.enb_hostname:
        LOGGER.info('No eNB in this scenario')
        return True

    found = set()
    for line in get_analysis_messages('{}/eNB.log.bz2'.format(OPTS.log_dir)):
        # 94772.731183 00000057 [MAC] A Configuring MIB for instance 0, CCid 0 : (band
        # 7,N_RB_DL 50,Nid_cell 0,p 1,DL freq 2685000000,phich_config.resource 0,
        # phich_config.duration 0)
        if '[MAC] A Configuring MIB ' in line:
            found.add('configured')
            continue

        # 94777.679273 00000057 [MAC] A [eNB 0][RAPROC] CC_id 0 Frame 74, subframeP 3:
        # Generating Msg4 with RRC Piggyback (RNTI a67)
        if 'Generating Msg4 with RRC Piggyback' in line:
            found.add('msg4')
            continue

        # 94777.695277 00000057 [RRC] A [FRAME 00000][eNB][MOD 00][RNTI a67] [RAPROC]
        # Logical Channel UL-DCCH, processing LTE_RRCConnectionSetupComplete
        # from UE (SRB1 Active)
        if 'processing LTE_RRCConnectionSetupComplete from UE ' in line:
            found.add('setup')
            continue

        # 94776.725081 00000057 [RRC] A got UE capabilities for UE 6860
        match = re.search(r'\[RRC\] A (got UE capabilities for UE \w+)$', line)
        if match:
            found.add(match.group(1))
            continue

        # 2075586.647598 00006b4a [RRC] A Generating
        # RRCCConnectionReconfigurationRequest (NRUE Measurement Report
        # Request).
        if '[RRC] A Generating RRCCConnectionReconfigurationRequest (NRUE Measurement Report Request)' in line:
            found.add('gen nrue meas report req')
            continue

        # 2075586.671139 00006b4a [RRC] A [FRAME 00000][eNB][MOD 00][RNTI
        # e9fb] Logical Channel DL-DCCH, Generate NR UECapabilityEnquiry
        # (bytes 11)
        if ' Logical Channel DL-DCCH, Generate NR UECapabilityEnquiry' in line:
            found.add('dl-dcch gen nrue cap enq')
            continue

        # 2075586.677066 00006b4a [RRC] A got nrUE capabilities for UE e9fb
        match = re.search(r'\[RRC\] (got nrUE capabilities for UE \w+)$', line)
        if match:
            found.add(match.group(1))
            continue

        # 2075586.756959 00006b4a [RRC] A [eNB 0] frame 0 subframe 0: UE rnti
        # e9fb switching to NSA mode
        match = re.search(r': (UE rnti \w+ switching to NSA mode)', line)
        if match:
            found.add(match.group(1))
            continue

        # 2075586.911204 00006b4a [RRC] A Sent rrcReconfigurationComplete to gNB
        if '[RRC] A Sent rrcReconfigurationComplete to gNB' in line:
            found.add('rrc reconf complete to gnb')
            continue

    LOGGER.debug('found: %r', found)

    num_ues = len(scenario.ue_hostname)
    LOGGER.debug('num UEs: %d', num_ues)

    if OPTS.mode == 'lte' and len(found) == 3 + num_ues:
        LOGGER.info('eNB passed')
        return True

    if OPTS.mode == 'nsa' and len(found) == 6 + 2 * num_ues:
        LOGGER.info('eNB passed')
        return True

    LOGGER.error('eNB failed -- found %d %r', len(found), found)
    return False

def analyze_ue_logs(scenario: Scenario) -> bool:
    if not scenario.ue_hostname:
        LOGGER.info('No UEs in this scenario')
        return True

    num_failed = 0
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

            # 2350014.072310 0001d422 [RRC] A rrc_ue_process_ueCapabilityEnquiry:
            # UECapabilityInformation Encoded 316 bits (40 bytes)
            if '[RRC] A rrc_ue_process_ueCapabilityEnquiry: UECapabilityInformation Encoded ' in line:
                found.add('capabilities')
                continue

            # 2075586.627878 00006d6e [RRC] A Initial ueCapabilityEnquiry sent
            # to NR UE with size 5
            if '[RRC] A Initial ueCapabilityEnquiry sent to NR UE with size ' in line:
                found.add('cap enq to nrue')
                continue

            # 2075586.675497 00006d6e [RRC] A
            # rrc_ue_process_nrueCapabilityEnquiry: NR_UECapInfo LTE_RAT_Type_nr Encoded
            # Encoded 108 bits (14 bytes)
            if '[RRC] A rrc_ue_process_nrueCapabilityEnquiry: NR_UECapInfo LTE_RAT_Type_nr Encoded ' in line:
                found.add('nrue cap info encoded')
                continue

            # 2075586.675497 00006d6e [RRC] A
            # rrc_ue_process_nrueCapabilityEnquiry: NR_UECapInfo LTE_RAT_Type_eutra_nr Encoded
            # Encoded 108 bits (14 bytes)
            if '[RRC] A rrc_ue_process_nrueCapabilityEnquiry: NR_UECapInfo LTE_RAT_Type_eutra_nr Encoded ' in line:
                found.add('ue cap info encoded')
                continue

            # 2075586.662377 00006d6e [RRC] A Encoded measurement object 101
            # bits (13 bytes) and sent to NR UE
            if '[RRC] A Encoded measurement object ' in line and \
               ' and sent to NR UE' in line:
                found.add('meas to nrue')
                continue

            # 2075586.673264 00006d6e [RRC] A Second ueCapabilityEnquiry
            # (request for NR capabilities) sent to NR UE with size 5
            if '[RRC] A Second ueCapabilityEnquiry (request for NR capabilities) sent to NR UE' in line:
                found.add('2nd cap enq to nrue')
                continue

            # 2075586.884697 00006d6e [RRC] A Sent RRC_CONFIG_COMPLETE_REQ to
            # the NR UE
            if '[RRC] A Sent RRC_CONFIG_COMPLETE_REQ to the NR UE' in line:
                found.add('config complete to nrue')
                continue

        if OPTS.mode == 'lte' and len(found) == 4:
            LOGGER.info('UE%d passed', ue_number)
        elif OPTS.mode == 'nsa' and len(found) == 10:
            LOGGER.info('UE%d passed', ue_number)
        else:
            num_failed += 1
            LOGGER.error('UE%d failed -- found %d %r', ue_number, len(found), found)

    if num_failed != 0:
        LOGGER.critical('%d of %d UEs failed', num_failed, len(scenario.ue_hostname))
        return False

    LOGGER.info('All UEs passed')
    return True

def analyze_gnb_logs(scenario: Scenario) -> bool:
    if not scenario.gnb_hostname:
        LOGGER.info('No gNB in this scenario')
        return True

    found = set()
    for line in get_analysis_messages('{}/gNB.log.bz2'.format(OPTS.log_dir)):
        # 2075586.780257 00006cd4 [NR_RRC] A Successfully parsed CG_ConfigInfo
        # of size 19 bits. (19 bytes)
        if '[NR_RRC] A Successfully parsed CG_ConfigInfo ' in line:
            found.add('configured')
            continue

        # 2075586.912997 00006cd4 [NR_RRC] A
        # Handling of reconfiguration complete message at RRC gNB is pending
        if 'Handling of reconfiguration complete message at RRC gNB is pending' in line:
            timestamp = line.split()[0]
            found.add('rrc complete ' + timestamp)
            continue

        # 2075586.763190 00006cd4 [NR_RRC] A Successfully decoded UE NR
        # capabilities (NR and MRDC)
        if '[NR_RRC] A Successfully decoded UE NR capabilities (NR and MRDC)' in line:
            found.add('nr capabilites')
            continue

        # 364915.873598 000062e2 [NR_MAC] A (ue 0, rnti 0xb094) CFRA procedure succeeded!
        match = re.search(r'\[NR_MAC\] A \(ue \d+, (rnti \w+)\) CFRA procedure succeeded!$', line)
        if match:
            found.add(f'cfra {match.group(1)}')
            continue

    LOGGER.debug('found: %r', found)

    num_ues = len(scenario.ue_hostname)
    LOGGER.debug('num UEs: %d', num_ues)

    num_expect = 2 + 2 * num_ues
    if len(found) != num_expect:
        LOGGER.error('gNB failed -- found %d/%d %r', len(found), num_expect, found)
        return False

    LOGGER.info('gNB passed')
    return True

def analyze_nrue_logs(scenario: Scenario) -> bool:
    if not scenario.nrue_hostname:
        LOGGER.info('No NRUEs in this scenario')
        return True

    num_failed = 0
    for nrue_number, nrue_hostname in scenario.nrue_hostname.items():
        found = set()
        expected = {
            'sent cap',
            'cap encoded',
            'reconf encoded',
            'rrc meas sent',
            'rar',
        }
        for line in get_analysis_messages('{}/{}.log.bz2'.format(OPTS.log_dir, nrue_hostname)):
            # 2075586.628184 00006d66 [NR_RRC] A Sent initial NRUE Capability
            # response to LTE UE
            if '[NR_RRC] A Sent initial NRUE Capability response to LTE UE' in line:
                found.add('sent cap')
                continue

            # 2075586.674747 00006d66 [NR_RRC] A [NR_RRC] NRUE Capability
            # encoded, 10 bytes (86 bits)
            if '[NR_RRC] NRUE Capability encoded,' in line:
                found.add('cap encoded')
                continue

            # 2075586.895662 00006d66 [NR_RRC] A rrcReconfigurationComplete
            # Encoded 5 bits (1 bytes)
            if '[NR_RRC] A rrcReconfigurationComplete Encoded' in line:
                found.add('reconf encoded')
                continue

            # 2075586.949504 00006d72 [NR_RRC] A Populated
            # NR_UE_RRC_MEASUREMENT information and sent to LTE UE
            if '[NR_RRC] A Populated NR_UE_RRC_MEASUREMENT information and sent to LTE UE' in line:
                found.add('rrc meas sent')
                continue

            # 1314472.368736 0000008f [NR_MAC] A [UE 0][RAPROC][926.7]
            # Found RAR with the intended RAPID 63
            if 'Found RAR with the intended RAPID' in line:
                found.add('rar')
                continue

        LOGGER.debug('found: %r', found)
        num_expect = len(expected)
        if len(found) == num_expect:
            LOGGER.info('NRUE%d passed', nrue_number)
        else:
            num_failed += 1
            LOGGER.error('NRUE%d failed -- found %d/%d %r', nrue_number, len(found), len(expected), found)
            LOGGER.error('NRUE%d failed -- missed %d %r', nrue_number, len(expected - found), expected - found)

    if num_failed != 0:
        LOGGER.critical('%d of %d NRUEs failed', num_failed, len(scenario.nrue_hostname))
        return False

    LOGGER.info('All NRUEs passed')
    return True

def main() -> int:
    scenario = Scenario()

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

    passed = True

    if not OPTS.no_run:
        set_core_pattern()

        passed = scenario.run()

        if save_core_files():
            passed = False

    # Examine the logs to determine if the test passed

    if not analyze_enb_logs(scenario):
        passed = False

    if not analyze_ue_logs(scenario):
        passed = False

    if not analyze_gnb_logs(scenario):
        passed = False

    if not analyze_nrue_logs(scenario):
        passed = False

    if not passed:
        LOGGER.critical('FAILED')
        return 1

    LOGGER.info('PASSED')
    return 0

sys.exit(main())

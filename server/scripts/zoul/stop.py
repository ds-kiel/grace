#!/usr/bin/env python

# Simon Duquennoy (simonduq@sics.se)

import sys
import os
import subprocess
import sys
sys.path.insert(1,'/usr/testbed/scripts')
from psshlib import *

REMOTE_LOGS_PATH = "/home/user/logs"
REMOTE_SCRIPTS_PATH = "/home/user/scripts"
REMOTE_ZOUL_SCRIPTS_PATH = os.path.join(REMOTE_SCRIPTS_PATH, "zoul")
REMOTE_TMP_PATH = "/home/user/tmp"
REMOTE_NULL_FIRMWARE_PATH = os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "null.bin")
REMOTE_BSL_ADDRESS_PATH = os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "null_bsl_address.txt")
  
if __name__=="__main__":
  
  if len(sys.argv)<2:
    print "Job directory parameter not found!"
    sys.exit(1)
    
  # The only parameter contains the job directory
  job_dir = sys.argv[1]
       
  hosts_path = os.path.join(job_dir, "hosts")
  # Kill serialdump
  # pssh(hosts_path, "killall contiki-serialdump -9", "Stopping serialdump")
  pssh(hosts_path, 'if pgrep picocom; then killall -9 picocom;fi', "Stopping picocom")
  pssh(hosts_path, 'if pgrep screen; then screen -X -S zoulscreen quit;fi', "Quitting screen")
  pssh(hosts_path, 'if pgrep serial_forwarder; then killall -9 serial_forwarder;fi; if pgrep cat; then killall -9 cat;fi; if pgrep netcat; then killall -9 netcat;fi; if pgrep tee; then killall -9 tee;fi;', "Stopping serial_forwarder")
  # Program the nodes with null firmware
  if pssh(hosts_path, "%s %s %s"%(os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "install.sh"), REMOTE_NULL_FIRMWARE_PATH, REMOTE_BSL_ADDRESS_PATH), "Uninstalling zoul firmware") != 0:
    sys.exit(4)


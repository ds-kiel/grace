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
REMOTE_JN_SCRIPTS_PATH = os.path.join(REMOTE_SCRIPTS_PATH, "nrf52")
REMOTE_TMP_PATH = "/home/user/tmp"
REMOTE_NULL_FIRMWARE_PATH = os.path.join(REMOTE_JN_SCRIPTS_PATH, "null.nrf52.hex")

  
if __name__=="__main__":

  if len(sys.argv)<2:
    print "Job directory parameter not found!"
    sys.exit(1)
    
  # The only parameter contains the job directory
  job_dir = sys.argv[1]
       
  hosts_path = os.path.join(job_dir, "hosts")
  # Kill serialdump
  pssh(hosts_path, 'if pgrep picocom; then killall -9 picocom; fi; if pgrep screen; then screen -X -S nrf52screen quit; fi; if pgrep screen; then killall -9 screen; fi; if pgrep contiki-timestamp; then killall -9 contiki-timestamp; fi;', "Stopping picocom, screen and contiki-timestamp")
  # pssh(hosts_path, 'if pgrep screen; then screen -X -S nrf52screen quit; fi', "Quitting screen")
  #pssh(hosts_path, "killall -9 screen", "Killing screen")

  # Program the nodes with null firmware
  if pssh(hosts_path, "%s %s"%(os.path.join(REMOTE_JN_SCRIPTS_PATH, "install.sh"), REMOTE_NULL_FIRMWARE_PATH), "Uninstalling nrf52 firmware") != 0:
    sys.exit(4)


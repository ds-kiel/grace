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
REMOTE_GPIO_APP_PATH = "/usr/testbed/gpio-tracer"
REMOTE_ZOUL_SCRIPTS_PATH = os.path.join(REMOTE_SCRIPTS_PATH, "zoul")
REMOTE_TMP_PATH = "/home/user/tmp"
REMOTE_FIRMWARE_PATH = os.path.join(REMOTE_TMP_PATH, "firmware.bin")
REMOTE_SERIAL_FORWARDER_PATH = os.path.join(REMOTE_TMP_PATH, "serial_forwarders.txt")
REMOTE_BSL_ADDRESS_PATH = os.path.join(REMOTE_TMP_PATH, "bsl_address.txt")

  
if __name__=="__main__":

  if len(sys.argv)<2:
    print "Job directory parameter not found!"
    sys.exit(1)

  try:
    opts, args = getopt.getopt(sys.argv[2:], "", ["trace-gpio"])
  except getopt.GetoptError:
    usage()

  for opt, value in opts:
       if opt == "--trace-gpio":
         trace_gpio = True


  # The only parameter contains the job directory
  job_dir = sys.argv[1]
  forward_serial = True if sys.argv[2] == "forward" else False
  if forward_serial:
    forward_serial_hosts_path = os.path.join(job_dir, "forward_serial_hosts")
    if not os.path.exists(forward_serial_hosts_path):
      forward_serial_hosts_path = None

  # Look for the firmware
  elf_path = None
  if os.path.isdir(job_dir):
   for f in os.listdir(job_dir):
    if f.endswith(".zoul"):
      elf_path = os.path.join(job_dir, f)
      break

  if elf_path == None:
    print "No zoul firmware found!"
    sys.exit(2)

  bin_path = elf_path[0:elf_path.rfind(".")]+".bin"
  bsl_address_path = os.path.join(job_dir, "bsl_address.txt")

  print "Extracting BSL address from %s to %s"%(elf_path, bsl_address_path)
  os.system("arm-none-eabi-objdump -h %s | grep -B1 LOAD | grep -Ev 'LOAD|\-\-' | awk '{print \"0x\" $5}' | sort -g | head -1 > %s" %(elf_path, bsl_address_path))
  print "Generating bin from %s to %s"%(elf_path, bin_path)
  os.system("arm-none-eabi-objcopy -O binary --gap-fill 0xff %s %s" %(elf_path, bin_path))

  hosts_path = os.path.join(job_dir, "hosts")
  # Copy firmware to the nodes
  if pscp(hosts_path, bsl_address_path, REMOTE_BSL_ADDRESS_PATH, "Copying zoul bsl address to the PI nodes") != 0:
    sys.exit(3)
  if pscp(hosts_path, bin_path, REMOTE_FIRMWARE_PATH, "Copying zoul firmware to the PI nodes") != 0:
    sys.exit(4)
  # Copy forwarder list to nodes
  if forward_serial and forward_serial_hosts_path:
    if pscp(hosts_path, forward_serial_hosts_path, REMOTE_SERIAL_FORWARDER_PATH, "Copying forward serial hosts to the PI nodes") != 0:
      sys.exit(5)
  # Program the nodes
  if pssh(hosts_path, "%s %s %s"%(os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "install.sh"), REMOTE_FIRMWARE_PATH, REMOTE_BSL_ADDRESS_PATH), "Installing zoul firmware") != 0:
    sys.exit(6)
  # Start serialdump
  remote_log_dir = os.path.join(REMOTE_LOGS_PATH, os.path.basename(job_dir), "log.txt")
  if not forward_serial:
    if pssh(hosts_path, "%s %s"%(os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "serialdump.sh"), remote_log_dir), "Starting serialdump") != 0:
      sys.exit(7)
  else:
    if pssh(hosts_path, "%s %s"%(os.path.join(REMOTE_ZOUL_SCRIPTS_PATH, "serial_forwarder.sh"), remote_log_dir), "Starting serialdump with forwarding") != 0:
      sys.exit(8)

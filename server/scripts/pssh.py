#!/usr/bin/env python

# Simon Duquennoy (simonduq@sics.se)

import sys
import os
import subprocess

TESTBED_PI_PATH = "/home/user/raspi/scripts"

def pssh(hosts_path, cmd, message, inline=False):
  print "%s (on all: %s)" %(message, cmd)
  cmdpth = os.path.join(TESTBED_PI_PATH, cmd)
  return subprocess.call(["parallel-ssh", "-h", hosts_path, "-o", "pssh-out", "-e", "pssh-err", "-l", "user", "-i" if inline else "", cmdpth])
  
def pscp(hosts_path, src, dst, message):
  print "%s (on all: %s -> %s)" %(message, src, dst)
  return subprocess.call(["parallel-scp", "-h", hosts_path, "-o", "pssh-out", "-e", "pssh-err", "-l", "user", "-r", src, dst])

def pslurp(hosts_path, src, dst, message):
  print "%s (on all: %s -> %s)" %(message, src, dst)
  return subprocess.call(["parallel-slurp", "-h", hosts_path, "-o", "pssh-out", "-e", "pssh-err", "-l", "user", "-r", "-L", dst, src, "."])


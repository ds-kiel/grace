#!/bin/bash

log_path=$1
tty_path="/dev/ttyBLENANO"
nohup ~/scripts/contiki-serialdump -b115200 $tty_path | ~/scripts/contiki-timestamp > $log_path & > /dev/null 2> /dev/null
sleep 1
ps | grep "$! "
exit $?

tty=`ls /dev/serial/by-id/*Zolertia_Firefly*` 
port=50000
log_path=$1
serial_forwarder_path="/home/user/tmp/serial_forwarders.txt"

if [ -z "$tty" ]
then
        echo "Could not find Zolertia Firefly"
        exit 1
fi

# avoid serial port being blocked
if pgrep picocom; then killall -9 picocom;fi
if pgrep serialdump; then killall -9 serialdump;fi
if pgrep serial_forwarder; then killall -9 serial_forwarder;fi
# kill previous screen
if pgrep screen; then screen -X -S zoulscreen quit;fi
if pgrep screen; then killall -9 screen;fi

if [[ -f "$serial_forwarder_path" ]]; then
        # if a file is given, choose only those nodes that are defined to forward serial
        if grep -q `hostname` "$serial_forwarder_path"; then
                #set baudrate
                stty -F $tty 115200 min 1 cs8 -cstopb -parenb -brkint -icrnl -imaxbel -opost -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke

                screen -wipe
                screen -dmS zoulscreen bash

                screen -S zoulscreen -X stuff "cat $tty | ~/scripts/contiki-timestamp | tee -a $log_path | netcat -lkt 0.0.0.0 $port >$tty\n"
        else
                screen -wipe
                screen -dmS zoulscreen bash

                screen -S zoulscreen -X stuff "picocom --noreset -fh -b 115200 --imap lfcrlf $tty | ~/scripts/contiki-timestamp > $log_path\n"
        fi
else
        # if no file is given, do serial forwarding on all
        #set baudrate
        stty -F $tty 115200 min 1 cs8 -cstopb -parenb -brkint -icrnl -imaxbel -opost -isig -icanon -iexten -echo -echoe -echok -echoctl -echoke

        screen -wipe
        screen -dmS zoulscreen bash

        screen -S zoulscreen -X stuff "cat $tty | ~/scripts/contiki-timestamp | tee -a $log_path | netcat -lkt 0.0.0.0 $port >$tty\n"
fi

sleep 1

ps | grep "$! "
exit $?

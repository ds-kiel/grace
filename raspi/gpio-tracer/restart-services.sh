#! /usr/bin/env bash

remotes=(uni-rasp-15 uni-rasp-07 uni-rasp-05 uni-rasp-20)

for i in "${remotes[@]}"; do
    if [ $i == uni-rasp-15 ]; then
        ssh $i "sudo systemctl restart sync-node-rest-server"
    else
        ssh $i "sudo systemctl restart dbus-org.cau.gpiot && sudo systemctl restart collector-rest-server"
    fi
done

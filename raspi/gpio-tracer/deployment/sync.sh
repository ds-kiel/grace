#!/usr/bin/env bash

remotes=(uni-rasp-15 uni-rasp-07 uni-rasp-05 uni-rasp-20)

for i in "${remotes[@]}"; do
    rsync -a ./deployment.tar.gz $i:/usr/testbed/gpio-tracer/
    if [ $i == uni-rasp-15 ]; then
        ssh $i "mkdir -p /usr/testbed/gpio-tracer && cd /usr/testbed/gpio-tracer && tar xvf deployment.tar.gz"
    else
        ssh $i "mkdir -p /usr/testbed/gpio-tracer && cd /usr/testbed/gpio-tracer && tar xvf deployment.tar.gz"
    fi
done



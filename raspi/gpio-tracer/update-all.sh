#! /usr/bin/env bash

build_host=192.168.1.228

# Build gpiotd and gpiotc on pi with debian buster
# rsync --filter='merge ./sync.rules' -a ./ user@$build_host:/usr/testbed/gpio-tracer/
# ssh user@$build_host "cd /usr/testbed/gpio-tracer/build && make"
# rsync  -a user@$build_host:/usr/testbed/gpio-tracer/build/src/gpiotd ./deployment/gpiotd
# rsync  -a user@$build_host:/usr/testbed/gpio-tracer/build/src/gpiotc ./deployment/gpiotc

cd deployment
bash generate-new.sh
bash sync.sh

# rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.109:/usr/testbed/gpio-tracer/

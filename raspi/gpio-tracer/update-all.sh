#! /usr/bin/env bash

build_host=build-pi

# Build gpiotd and gpiotc on pi with debian buster
rsync --filter='merge ./sync.rules' -a ./ $build_host:/usr/testbed/gpio-tracer/
ssh $build_host "cd /usr/testbed/gpio-tracer/build && make"
rsync  -a $build_host:/usr/testbed/gpio-tracer/build/src/gpiotd ./deployment/gpiotd
rsync  -a $build_host:/usr/testbed/gpio-tracer/build/src/gpiotc ./deployment/gpiotc
rsync  -a $build_host:/usr/testbed/gpio-tracer/build/lib/libtransmitter.so ./deployment/libtransmitter.so

cd deployment
bash generate-new.sh
bash sync.sh

# rsync --filter='merge ./sync.rules' -a ./ user@192.168.1.109:/usr/testbed/gpio-tracer/

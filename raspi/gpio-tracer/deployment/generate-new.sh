#!/usr/bin/env bash

tar -czvf deployment.tar.gz ./gpiotd ./gpiotc ./libsigrok* ../python/ ../systemd/ ./libtransmitter.so ./libpigpio.so.1 ./firmware ./install.sh ./gpiotd-wrapper.sh ./sync-server-wrapper.sh


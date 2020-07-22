#!/usr/bin/sh

tar -czvf deployment.tar.gz ./gpiotd ./gpiotc ./libsigrok* ../python/ ../systemd/ ./libtransmitter.so ./libpigpio.so.1 ./firmwares ./install.sh ./gpiotd-wrapper.sh


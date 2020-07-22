#!/usr/bin/env bash
apt update
apt install pigpio libglib2.0-dev python3
apt install libftdi1-dev libzip-dev libserialport-dev pigpio python3 python3-pip sdcc
pip3 install flask-restful dbus-python requests

cp systemd/dbus-org.cau.gpiot.service /etc/systemd/system/
cp systemd/collector-rest-server.service /etc/systemd/system/
cp systemd/sync-node-rest-server.service /etc/systemd/system/
cp systemd/org.cau.gpiot.service /usr/share/dbus-1/system-services/
cp systemd/gpiot.conf /etc/dbus-1/system.d/

mkdir -p /usr/share/sigrok-firmware/
cp firmware/* /usr/share/sigrok-firmware/

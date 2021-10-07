# Installation - per user session bus
See [stackoverflow](https://serverfault.com/questions/892465/starting-systemd-services-sharing-a-session-d-bus-on-headless-system)

# Dependencies
### Installation on Raspbian

#### Install dependencies for gpio-tracer
```
  apt update
  apt install libsigrok-dev libglib2.0-dev python3 sigrok-firmware-fx2lafw cmake
  apt install default-dbus-session-bus # Needed for running dbus-user sessions. Only needed for development
  reboot
```


#### Enable spidev
Either use raspi-config and enable SPI or add `dtparam=spi=on` to `/boot/config.txt`
additionaly the user running the daemon will have to be added to the `spi`  and the `plugdev` group.
```
usermod -a -G spi user
usermod -a -G plugdev user
```

#### Enable udev rules
```
ln -s /usr/lib/udev/rules.d/61-libsigrok-plugdev.rules /etc/udev/rules.d/61-libsigrok-plugdev.rules
ln -s /usr/lib/udev/rules.d/61-libsigrok-uaccess.rules /etc/udev/rules.d/61-libsigrok-uaccess.rules
```

```
  pip3 install flask-restful dbus-python requests
```

# Installation - System bus
`/ssh:user@raspiot|sudo:root@raspiot:/usr/share/dbus-1/system-services/org.cau.gpiot.service`
`/ssh:user@raspiot|sudo:root@raspiot:/etc/dbus-1/system.d/gpiot.conf`

```
    cp systemd/dbus-org.cau.gpiot.service /etc/systemd/system/
    cp systemd/org.cau.gpiot.service /usr/share/dbus-1/system-services/
    cp systemd/gpiot.conf /etc/dbus-1/system.d/
```

# Note - Installation on rasbian stretch
Raspbian stretch still packages an old version of libsigrok. A new version has to be build on the target system
and be included with the binary. The fx2lafw firmware also has to be build an installed.
```
  sudo apt -y install libftdi1-dev libzip-dev libserialport-dev python3 python3-pip sdcc libsigrok-dev libglib2.0-dev cmake

  sudo pip3 install flask-restful dbus-python requests
```

# Enabling Collector Services

First make sure to enable and start the systemd service:

```
    systemctl daemon-reload
    systemctl enable dbus-org.cau.gpiot
    systemctl start dbus-org.cau.gpiot

    journalctl -u dbus-org.cau.gpiot -f
```

After that the daemon can be controlled with `gpiotc`.

# Enabling Synchronization Node Services

First make sure to enable and start the systemd service:

```
    echo dtoverlay=pps-gpio,gpiopin=18 >> /boot/config.txt
    echo pps-gpio >> /etc/modules
    apt install -y pps-tools
    systemctl daemon-reload
    systemctl enable sync-node-rest-server.service
    systemctl start sync-node-rest-server.service
```

# Example of CLI

Start tracing with sigrok and save traces in a log file:

```
    ./gpiotc --start --device=sigrok --logpath=/home/user/logs/analyse/sigrok-count-timestamps-2000hz.csv
    ./gpiotc --stop --device=sigrok
```


# Installation sync node NTP disclipined by PPS


# shell snippets

```
    systemctl restart dbus-org.cau.gpiot.service && journalctl -u dbus-org.cau.gpiot -f
    lsyncd -rsyncssh ./gpio-tracer/ uni-rasp-03-new-raspbian /usr/testbed/gpio-tracer/ -nodaemon
    ./gpiotc --start --device=sigrok --logpath=/home/user/logs/analyse/sigrok-count-timestamps-1hz.csv && sleep 20 && ./gpiotc --stop --device=sigrok
```

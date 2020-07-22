# Installation - per user session bus
See [stackoverflow](https://serverfault.com/questions/892465/starting-systemd-services-sharing-a-session-d-bus-on-headless-system)

# Dependencies
## general dependencies
- `pigpio`
- `libsigrok`
- `glib`
- `python3`

### Installation on Raspbian
```
  apt update
  apt install pigpio libsigrok-dev libglib2.0-dev python3
```

## Python dependencies:
- `flask-restful`
- `dbus-python`
- `requests`

### Installation on Raspbian
```
  pip3 install flask-restful dbus-python requests
```


# Installation - System bus
`/ssh:user@raspiot|sudo:root@raspiot:/usr/share/dbus-1/system-services/org.cau.gpiot.service`
`/ssh:user@raspiot|sudo:root@raspiot:/etc/dbus-1/system.d/gpiot.conf`

```
    cp systemd/dbus-org.cau.gpiot.service /etc/systemd/system/
    cp systemd/collector-rest-server.service /etc/systemd/system/
    cp systemd/sync-node-rest-server.service /etc/systemd/system/
    cp systemd/org.cau.gpiot.service /usr/share/dbus-1/system-services/
    cp systemd/gpiot.conf /etc/dbus-1/system.d/
```

# Note - Installation on rasbian stretch
Raspbian stretch still packages an old version of libsigrok. A new version has to be build on the target system
and be included with the binary. The fx2lafw firmware also has to be build an installed.
```
  sudo apt install libftdi1-dev libzip-dev libserialport-dev pigpio python3 python3-pip sdcc

  sudo pip3 install flask-restful dbus-python requests
```

# Running

First make sure to enable and start the systemd service:

```
    ln -s build/src/gpiotd
    ln -s build/src/gpiotc
    systemctl daemon-reload
    systemctl enable dbus-org.cau.gpiot
    systemctl start dbus-org.cau.gpiot
```

After that the daemon can be controlled with `gpiotc`.

# Example

Start tracing with sigrok and save traces in a log file:

```
    ./gpiotc --start --device=sigrok --logpath=/home/user/logs/analyse/sigrok-count-timestamps-2000hz.csv
    ./gpiotc --stop --device=sigrok
```


# shell snippets

```
    systemctl restart dbus-org.cau.gpiot.service && journalctl -u dbus-org.cau.gpiot -f
    ./gpiotc --start --device=sigrok --logpath=/home/user/logs/analyse/sigrok-count-timestamps-1hz.csv && sleep 20 && ./gpiotc --stop --device=sigrok
```

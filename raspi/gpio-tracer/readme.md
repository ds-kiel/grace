# Installation - per user session bus
See [stackoverflow](https://serverfault.com/questions/892465/starting-systemd-services-sharing-a-session-d-bus-on-headless-system)

# Installation - System bus
`/ssh:user@raspiot|sudo:root@raspiot:/usr/share/dbus-1/system-services/org.cau.gpiot.service`
`/ssh:user@raspiot|sudo:root@raspiot:/etc/dbus-1/system.d/gpiot.conf`

```
  cp systemd/dbus-org.cau.gpiot.service /etc/systemd/system/
  cp systemd/org.cau.gpiot.service /usr/share/dbus-1/system-services/
  cp systemd/gpiot.conf /etc/dbus-1/system.d/
```

# Running

First make sure to enable and start the systemd service:

```
    systemctl enable gpiot
    systemctl start gpiot
```

After that the daemon can be controlled with `gpiotc`.

# Example

Start tracing with sigrok and save traces in a log file:

```
  gpiotc --start --device=sigrok --logpath=log.csv
```



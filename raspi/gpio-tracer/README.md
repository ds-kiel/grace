# Installation Observer Software
The following instructions are for a deployment on raspbian/debian 11 (bullseye).  Since we only
depent on libusb, dbus and glib the installation should be similar for other distributions.

On your observer node you will have to install both `gpiotd` and `gpiotc`. hereby `gpiotd` is a daemon
application running as a systemd service that takes action from `gpiotc` through dbus.

## Install Dependencies
```sh
apt update
apt install -y libglib2.0-dev python3 cmake libusb-1.0-0-dev  
apt install default-dbus-session-bus # Needed for running gpiotd in a dbus-user sessions.
```

## Build
For development purposes it is possible to disable the time synchronization aspect.
```sh
mkdir build
cd build
cmake .. -DWITH_RADIO_SYNC=On
    make
```

## Enable spidev
Either use raspi-config to enable SPI or add `dtparam=spi=on` to `/boot/config.txt`
additionaly the user running the daemon will have to be added to the `spi`  and the `plugdev` group.
```sh
# Only needed if you don't enable spi manually
sed -i 's/#.*dtparam=spi=on/dtparam=spi=on/g' /boot/config.txt 

usermod -a -G spi user
usermod -a -G plugdev user
```

## Install udev Rules
```sh
# copy or link the udev rules. The location for udev rules might differ for other
cp udev/60-gpio-tracer.rules /usr/lib/udev/rules.d/
cp udev/60-gpio-tracer.rules /etc/udev/rules.d/
  
udevadm control --reload-rules && udevadm trigger
```

## Install and Enable `gpiotd` Service
Install the systemd service. Be sure to adapt the install location in `dbus-org.cau.gpiot.service`.
```sh
# Install the service
cp systemd/dbus-org.cau.gpiot.service /etc/systemd/system/
# Auxillary files so gpiotd can acquire bus from dbus
cp systemd/org.cau.gpiot.service /usr/share/dbus-1/system-services/
cp systemd/gpiot.conf /etc/dbus-1/system.d/
```

## Start/Stop Tracing using `gpiotc`

Start tracing with sigrok and save traces in a log file:

```sh
# instruct gpiotd to start tracing. Make sure that the daemon is running.
./gpiotc --start --tracedir=<some path to store traces>
    
# stop tracing again
./gpiotc --stop
```

# Installation Reference Clock/Logic Analyzer Firmware

We use [PlatformIO](https://platformio.org/), for building the firmware both for the reference clock (`firmware/stm32`) and the Logic Analyzer (`firmware/fx2`).
Building should be as simple as executing `pio run` in the respective folders for each firmware.

for example for building the reference clock firmware:
```sh
cd firmware/stm32
pio run
# Execute only if you want to upload firmware to mcu
pio run -t upload # after setting mcu in dfu mode
```

Building the Logic Analyzer firmware will result in two firmware files: `firmware.bix` and `firmware.iic`. 
Note that the firmware (`firmware.bix`) of the Logic Analyzer will be installed automatically by the gpio tracing daemon `gpiotd`.

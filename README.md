# IoT-Testbed

A very simple testbed for the IoT!

## GPIO Tracing

Please refer to `raspi/gpio-tracer` for our gpio tracing implementation.

## Installation

### for platform Zoul

It might be necessary to install `pyserial` and `libusb`:

```bash
sudo apt update
sudo apt install python3-pip -y
pip3 install pyserial

sudo apt install libusb-0.1-4
```

### preparation for platform nRF52840

#### COPY JLINK rules file to allow flashing nrf52 for non-root users

```bash
parallel-ssh --timeout 0 --hosts /usr/testbed/scripts/all-hosts --user user --inline "sudo cp /home/user/scripts/nrf52/JLink_Linux_arm/99-jlink.rules /etc/udev/rules.d"
```

#### Flash/Update JLINK Firmware

```bash
parallel-ssh --timeout 0 --hosts /usr/testbed/scripts/all-hosts --user user --inline "sudo -i && cd /home/user/scripts/nrf52 && ./install.sh null.nrf52.hex"
```

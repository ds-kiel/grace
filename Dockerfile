FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
RUN apt update --fix-missing
RUN apt install -y python2.7 iptables-persistent dhcpdump rsync default-jre default-jdk      \
                             pssh putty-tools clusterssh libffi* cmake python-pip libssl-dev \
                             python-libssh2 python-openssl python3 python3-pip screen at ntp \
                             tree isc-dhcp-server wget bash

RUN apt install -y gcc-arm-linux-gnueabihf
RUN apt install -y binutils-arm-none-eabi build-essential

RUN pip install --upgrade pip
RUN pip install parallel-ssh
RUN pip install pytz

RUN wget https://linuxnet.ca/ieee/oui.txt -O /usr/local/etc/oui.txt

RUN useradd testbed
RUN useradd -m developer

RUN usermod -aG dialout testbed
RUN usermod -aG testbed developer

RUN mkdir -p /usr/testbed
RUN chown testbed:testbed /usr/testbed


ENTRYPOINT [ "/bin/sh" ]





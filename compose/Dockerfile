FROM ubuntu:18.04

MAINTAINER Oren Lederman <orenled@mit.edu>
# Based on https://github.com/FruityLoopers/fruityfactory

###########################
## Compilation environment
###########################
RUN apt-get update && apt-get install -y \
	build-essential \
  libbluetooth-dev \
  bluez \
  rfkill \
  bluetooth \
  lib32z1 \
  lib32ncurses5 \
  unzip \
  binutils-avr \
  git \
  cmake \
  minicom screen \
  vim \
  lcov \
  wget

RUN mkdir -p /opt/downloads

WORKDIR /opt

# GCC ARM
RUN wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 -O downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
RUN tar -C /usr/local -xjf downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

# nRF SDK
ENV NRF_SDK_PATH=/opt/nrf/sdk/nrf_sdk_12_3/nrf5_12.3.0_d7731ad
run mkdir -p $NRF_SDK_PATH
RUN wget https://developer.nordicsemi.com/nRF5_SDK/nRF5_SDK_v12.x.x/nRF5_SDK_12.3.0_d7731ad.zip -O downloads/nRF5_SDK_12.3.0_d7731ad.zip
RUN unzip -q downloads/nRF5_SDK_12.3.0_d7731ad.zip -d $NRF_SDK_PATH
#RUN wget https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/nRF51_SDK_8.1.0_b6ed55f.zip -O downloads/nRF51_SDK_8.1.0_b6ed55f.zip
#RUN unzip -q downloads/nRF51_SDK_8.1.0_b6ed55f.zip -d $NRF_SDK_PATH

COPY compose/Makefile.posix $NRF_SDK_PATH/components/toolchain/gcc/Makefile.posix

# SEGGER
copy compose/JLink_Linux_V616i_x86_64.deb /opt/downloads/JLink_Linux_V616i_x86_64.deb

RUN dpkg -i downloads/JLink_Linux_V616i_x86_64.deb

ENV DEBIAN_FRONTEND=noninteractive

################################
## Bluetooth Python environment
################################
RUN apt-get update && apt-get install -y \
    python \
    python-dev \
    python-pip \
    python-setuptools \
	python-tk \
    gcc \
    build-essential \
    libglib2.0-dev \
    bluez \
    --no-install-recommends && \
    rm -rf /var/lib/apt/lists/*

COPY ./requirements.txt /tmp/requirements.txt
RUN pip install -r /tmp/requirements.txt

ENV DISPLAY :0

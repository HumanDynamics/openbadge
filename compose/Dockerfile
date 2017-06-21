FROM ubuntu:12.04

MAINTAINER Oren Lederman <orenled@mit.edu>
# Based on https://github.com/FruityLoopers/fruityfactory

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
	minicom screen

RUN mkdir -p /opt/downloads

WORKDIR /opt

# GCC ARM
RUN wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 -O downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
RUN tar -C /usr/local -xjf downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

# nRF SDK
ENV NRF_SDK_PATH=/opt/nrf/sdk/nrf_sdk_8_1
run mkdir -p $NRF_SDK_PATH
RUN wget https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/nRF51_SDK_8.1.0_b6ed55f.zip -O downloads/nRF51_SDK_8.1.0_b6ed55f.zip
RUN unzip -q downloads/nRF51_SDK_8.1.0_b6ed55f.zip -d $NRF_SDK_PATH

COPY compose/Makefile.posix $NRF_SDK_PATH/components/toolchain/gcc/Makefile.posix

# SEGGER
copy compose/jlink_5.2.9_x86_64.deb /opt/downloads/jlink_5.2.9_x86_64.deb
RUN dpkg -i downloads/jlink_5.2.9_x86_64.deb

#FROM ubuntu:12.04
FROM resin/rpi-raspbian:jessie


MAINTAINER Oren Lederman <orenled@mit.edu>
# Based on https://github.com/FruityLoopers/fruityfactory

RUN apt-get update && apt-get install -y \
	build-essential \
  libbluetooth-dev \
  bluez \
  rfkill \
  bluetooth \
#  lib32z1 \
#  lib32ncurses5 \
  unzip \
  binutils-avr \
	git \
  cmake \
  minicom screen \
  wget \ 
  gcc-arm-none-eabi \
  libnewlib-arm-none-eabi \
  vim



RUN mkdir -p /opt/downloads

WORKDIR /opt

# GCC ARM
#RUN wget https://launchpad.net/gcc-arm-embedded/4.9/4.9-2015-q3-update/+download/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2 -O downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2
#RUN tar -C /usr/local -xjf downloads/gcc-arm-none-eabi-4_9-2015q3-20150921-linux.tar.bz2

# nRF SDK
ENV NRF_SDK_PATH=/opt/nrf/sdk/nrf_sdk_8_1
run mkdir -p $NRF_SDK_PATH
RUN wget https://developer.nordicsemi.com/nRF51_SDK/nRF51_SDK_v8.x.x/nRF51_SDK_8.1.0_b6ed55f.zip -O downloads/nRF51_SDK_8.1.0_b6ed55f.zip
RUN unzip -q downloads/nRF51_SDK_8.1.0_b6ed55f.zip -d $NRF_SDK_PATH

COPY compose/Makefile.posix $NRF_SDK_PATH/components/toolchain/gcc/Makefile.posix
RUN ln -s /lib/arm-linux-gnueabihf/libudev.so.1 /lib/arm-linux-gnueabihf/libudev.so.0

# SEGGER
COPY compose/JLink_Linux_V616i_arm.tgz /opt/downloads/JLink_Linux_V616i_arm.tgz
RUN tar -C /opt/ -xzf downloads/JLink_Linux_V616i_arm.tgz
RUN ln -s /opt/JLink_Linux_V616i_arm/JLinkExe /usr/local/bin/JLinkExe

COPY .  /app

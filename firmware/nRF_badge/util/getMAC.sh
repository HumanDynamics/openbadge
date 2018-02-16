#!/bin/bash
JLinkExe -device nrf51422_xxac -if swd -speed 500 readRawMAC.jlink | grep "100000A4" | awk -f fixMAC

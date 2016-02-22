#!/usr/bin/python
import os
import subprocess
import shlex
import sys
import time

pull_timeout = 5

def get_devices(device_file="device_macs.txt"):
    with open(device_file) as f:
        file = f.read()[:-1]
    return file.split("\n")

def send_time(addr):
    print "Sending date to " + addr
    time_str = subprocess.check_output("date +%F\ %T", shell=True)[:-1]

    arduino_time = time_str.replace("-","").replace(" ","").replace(":","")[4:]
    hex_array = ["3" + c for c in arduino_time]
    hex_time = "".join(hex_array)

    command =  "timeout " + str(pull_timeout) + " gatttool -b " + addr + " -t random --char-write-req --handle=0x0011 --value=" + hex_time
    print command
    p = subprocess.call(command, shell=True)


def hex_to_dec(hexstr):
    return int(hexstr, 16)

if len(sys.argv) == 2:
   addr=sys.argv[1]
   send_time(addr)
else:
   print "syncing all"
   devices = get_devices()
   for addr in devices:
        print addr
        send_time(addr)
        time.sleep(2)  # requires sleep between devices

exit(0)


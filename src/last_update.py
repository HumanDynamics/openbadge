#!/usr/bin/python
import re
import csv
import os

def get_devices(device_file="device_macs.txt"):
    if not os.path.isfile(device_file):
        print("Cannot find devices file: {}".format(device_file))
        exit(1)
    print("Reading whitelist:")
    devices=[]

    with open(device_file, 'r') as csvfile:
        fil = filter(lambda row: row[0]!='#', csvfile)
        fil = filter(lambda x: not re.match(r'^\s*$', x), fil)
        rdr = csv.reader(fil, delimiter=' ')
        for row in rdr:
            device = row[0]
            devices.append(device)

        csvfile.close()

    for device in devices:
        print("    {}".format(device))
    return devices


devices = get_devices()
for addr in devices:
    outfile = addr.replace(":","_") + ".scn"
    with open(outfile) as f:
        lines = f.readlines()[-1] #last line of the file
    if len(lines) > 0:
        lastLine = lines[-1]
        my_list = lastLine.split(",")
        lastDate = my_list[0]
        lastVoltage = my_list[1]
    else:
        lastDate = "none"
        lastVoltage = "none"
    print(addr + "   :   " + lastDate + " , " + lastVoltage)

exit(0)

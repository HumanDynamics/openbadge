#!/usr/bin/python
import os
import subprocess
import shlex
import sys
import time

pull_timeout = 10
sync_timeout = 5

'''
Returns a list of devices included in device_macs.txt which were also
detected by a low-energy scan. Some work should be done to
read from the lescan command more elegantly.
'''
def get_devices(device_file="device_macs.txt", active_file="active_devices.txt"):
    if not os.path.isfile(device_file):
        print str(device_file) + " cannot be found"
        exit(1)

    if os.path.isfile(active_file):
        print str(active_file) + " currently contains these devices:"
        with open(active_file, "r") as f:
            for line in f:
                print line[:-1]
        response = raw_input("Do you want to edit " + str(active_file) + "?[y/N] ")
        if response == "n" or response == "N" or response == "":
            active_devices = []
            with open(active_file, "r") as f:
                for line in f:
                    active_devices.append(line[:-1])
            return active_devices

    with open(device_file) as f:
        file = f.read()[:-1]
    all_devices = file.split("\n")
    print "Which devices should be managed? (ex. 1 2 4)"
    for index, device in enumerate(all_devices):
        print str(index + 1) + ": " + device
    tokens = raw_input().split(" ")
    active_devices = []
    f = open(active_file, "w")
    for token in tokens:
        index = int(token)
        if index < 1 or index > len(all_devices):
            print "Invalid input!"
            f.close()
            os.remove(active_file)
            exit(1)
        else:
            chosen_device = all_devices[index - 1]
            active_devices.append(chosen_device)
            f.write(str(chosen_device) + "\n")
    f.close()
    return active_devices

'''
Attempts to read data from the device specified by the address. Reading is handled by gatttool.
'''
def pull_data(addr="EF:B8:45:CF:B1:57"):
    print "Pulling data from "+addr
    start = "timeout " + str(pull_timeout) + " gatttool -b "
    end = " -t random --char-write-req --handle=0x000b --value=0100 --char-write-req --handle=0x000f --value=0100 --listen"
    pull_command = start + addr + end
    args = shlex.split(pull_command)
    p = subprocess.Popen(pull_command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

    outfile = addr.replace(":","_") + ".scn"
    i = 0
    fout = open(outfile, "a")
    fout.write("\n")
    for line in iter(p.stdout.readline, ''):
        hexstr = line.split()[-1]
        if hexstr == "successfully":
            continue
        decimal = hex_to_dec(hexstr)
        if decimal == -1:
            continue
        if i < 2:
            fout.write(str(decimal) + ":")
            i += 1
        elif i == 2:
            fout.write(str(decimal) + ",")
            i += 1
        elif i == 3:
            fout.write(str(decimal))
            i += 1
        else:
            fout.write("," + str(decimal))

def send_time(addr="EF:B8:45:CF:B1:57"):
    print "Sending date to " + addr
    time_str = subprocess.check_output("date +%F\ %T", shell=True)[:-1]

    arduino_time = time_str.replace("-","").replace(" ","").replace(":","")[4:]
    hex_array = ["3" + c for c in arduino_time]
    hex_time = "".join(hex_array)

    command = "timeout " + str(sync_timeout) + " gatttool -b " + addr + " -t random --char-write-req --handle=0x0011 --value=" + hex_time
    print command
    retcode = subprocess.call(command, shell=True)

    if retcode != 0:
        print "Syncing device " + str(addr) + \
              " failed with exit code " + str(retcode)
        print "Removing device " + str(addr) + " from active devices"
    return retcode

def check_active_devices(old_devices, active_file="active_devices.txt"):
    new_devices = []
    to_sync = []
    with open(active_file) as f:
        for line in f:
            device = line[:-1]
            new_devices.append(device)
            if device not in old_devices:
                print "New device " + device + " read from " + active_file
                to_sync.append(device)
    for device in old_devices:
        if device not in new_devices:
            print "Device " + device + " removed from active devices"

    to_remove = []
    for addr in to_sync:
        status = send_time(addr)
        if status != 0:
            to_remove.append(addr)
    for addr in to_remove:
        new_devices.remove(addr)
    with open(active_file, "w") as f:
        for device in new_devices:
            f.write(device + "\n")
    return new_devices

def reset():
    reset_command = "hciconfig hci0 reset"
    args = shlex.split(reset_command)
    p = subprocess.Popen(args)

def hex_to_dec(hexstr):
    try:
        return int(hexstr, 16)
    except ValueError:
        return -1

if len(sys.argv) > 1:
    if sys.argv[1] == "sync":
        send_time()
    elif sys.argv[1] == "pull":
        pull_data()
    elif sys.argv[1] == "scan":
        scan_devices()
    else:
        print "I can't do that"
else:
    active_devices = get_devices()
    to_remove = []
    for addr in active_devices:
        status = send_time(addr)
        #if status != 0:
        #    to_remove.append(addr)
	time.sleep(2)  # requires sleep between devices
    for addr in to_remove:
        active_devices.remove(addr)
    with open("active_devices.txt", "w") as f:
        for device in active_devices:
            f.write(device + "\n")

    time.sleep(5) # allow BLE time to disconnect (especially if using only one device)

    while True:
        active_devices = check_active_devices(active_devices)
        for device in active_devices:
            pull_data(device)
        time.sleep(10)
reset()
exit(0)

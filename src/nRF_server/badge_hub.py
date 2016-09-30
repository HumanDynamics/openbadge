#!/usr/bin/env python

from __future__ import absolute_import, division, print_function

import os
import re
import shlex
import subprocess

import requests

from server import BADGE, BADGES
from badge import *
from badge_discoverer import BadgeDiscoverer


log_file_name = 'server.log'
scans_file_name = 'scan.txt'
audio_file_name = 'log_audio.txt'
proximity_file_name = 'log_proximity.txt'

SCAN_DURATION = 3  # seconds

# create logger with 'badge_server'
logger = logging.getLogger('badge_server')
logger.setLevel(logging.DEBUG)

# create file handler which logs even debug messages
fh = logging.FileHandler(log_file_name)
fh.setLevel(logging.DEBUG)

# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

# create formatter and add it to the handlers
# formatter = logging.Formatter('%(asctime)s - %(levelname)s - [%(mac)s] %(message)s')
formatter = logging.Formatter('%(asctime)s - %(levelname)s - %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)

# add the handlers to the logger
logger.addHandler(fh)
logger.addHandler(ch)


def get_devices(device_file="device_macs.txt"):
    """
    Returns a list of devices included in device_macs.txt
    Format is device_mac<space>device_name
    :param device_file:
    :return:
    """
    if not os.path.isfile(device_file):
        logger.error("Cannot find devices file: {}".format(device_file))
        exit(1)
    logger.info("Reading whitelist:")

    regex = re.compile(r'^([A-Fa-f0-9]{2}(?::[A-Fa-f0-9]{2}){5}).*')

    with open(device_file, 'r') as devices_macs:
        devices = [regex.findall(line) for line in devices_macs]
        devices = filter(lambda x: x, map(lambda x: x[0] if x else False, devices))
        devices = [d.upper() for d in devices]

    for d in devices:
        logger.info("    {}".format(d))

    return devices

    # with open(device_file, 'r') as csvfile:
    #     fil = filter(lambda row: row[0]!='#', csvfile)
    #     fil = filter(lambda x: not re.match(r'^\s*$', x), fil)
    #     rdr = csv.reader(fil, delimiter=b' ')
    #     for row in rdr:
    #         device = row[0]
    #         devices.append(device)
    #
    #     csvfile.close()
    #
    #
    # return devices


def dialogue(bdg):
    """
    Attempts to read data from the device specified by the address. Reading is handled by gatttool.
    :param bdg:
    :return:
    """
    ret = bdg.pull_data()
    addr = bdg.addr
    if ret == 0:
        logger.info("Successfully pulled data")

        if bdg.dlg.chunks:
            logger.info("Chunks received: {}".format(len(bdg.dlg.chunks)))
            logger.info("saving chunks to file")

            # store in CSV file
            with open(audio_file_name, "a") as fout:
                for chunk in bdg.dlg.chunks:
                    ts_with_ms = '{}.{:03d}'.format(chunk.ts, chunk.fract)
                    logger.info("CSV: Chunk timestamp: {}, Voltage: {}, Delay: {}, Samples in chunk: {}".format(
                        ts_with_ms, chunk.voltage, chunk.sampleDelay, len(chunk.samples)))
                    fout.write(bytes("{},{},{},{}".format(addr, ts_with_ms, chunk.voltage, chunk.sampleDelay)))
                    for sample in chunk.samples:
                        fout.write(",{}".format(sample))
                    fout.write("\n")
                logger.info("done writing")

        else:
            logger.info("No mic data ready")

        if bdg.dlg.scans:
            logger.info("Proximity scans received: {}".format(len(bdg.dlg.scans)))
            logger.info("saving proximity scans to file")
            with open(proximity_file_name, "a") as fout:
                for scan in bdg.dlg.scans:
                    ts_with_ms = "%0.3f" % scan.ts
                    logger.info("SCAN: scan timestamp: {}, voltage: {}, number: {}".format(
                        ts_with_ms, scan.voltage, scan.numDevices))
                    if scan.devices:
                        device_list = ''
                        for dev in scan.devices:
                            device_list += "[#{:x},{},{}]".format(dev.ID, dev.rssi, dev.count)
                            fout.write(bytes("{},{},{},{},{},{}\n".format(addr, scan.voltage, ts_with_ms, dev.ID,
                                                                          dev.rssi, dev.count)))
                        logger.info('  >  ' + device_list)

        else:
            logger.info("No proximity scans ready")

        # TODO: cache the badge mac with timestamp
        # TODO: send the timestamp to the server
        try:
            last_chunk = bdg.dlg.chunks[-1]
            last_scan = bdg.dlg.scans[-1]
            bdg.set_audio_ts(last_chunk.ts, last_chunk.fract)
            bdg.last_proximity_ts = last_scan.ts
        except Exception as e:
            print(e)

        try:
            for b in bdg.children.values():
                response = requests.patch(BADGE(b.key), data={
                    'last_audio_ts': b.last_audio_ts,
                    'last_audio_ts_fract': b.last_audio_ts_fract,
                    'last_proximity_ts': b.last_proximity_ts,
                })
                if response.ok is False:
                    raise Exception('Server sent a {} status code instead of 200\n{}'.format(response.status_code,
                                                                                             response.text))

        except Exception as e:
            print('Exception with Requests {}'.format(e))


def scan_for_devices(devices_whitelist):
    bd = BadgeDiscoverer()
    try:
        all_devices = bd.discover(scan_duration=SCAN_DURATION)
    except Exception as e: # catch *all* exceptions
        logger.error("Scan failed,{}".format(e))
        all_devices = {}

    scanned_devices = []
    for addr,device_info in all_devices.iteritems():
        if addr in devices_whitelist:
            logger.debug("\033[1;7m\033[1;32mFound {}, added. Device info: {}\033[0m".format(addr, device_info))
            scanned_devices.append({'mac':addr,'device_info':device_info})
        else:
            logger.debug("Found {}, but not on whitelist. Device info: {}".format(addr, device_info))
    return scanned_devices


def reset():
    reset_command = "hciconfig hci0 reset"
    args = shlex.split(reset_command)
    p = subprocess.Popen(args)


def add_pull_command_options(subparsers):
    pull_parser = subparsers.add_parser('pull', help='Continuously pull data from badges')

def add_scan_command_options(subparsers):
    pull_parser = subparsers.add_parser('scan', help='Continuously scan for badges')


def add_sync_all_command_options(subparsers):
    sa_parser = subparsers.add_parser('sync_all', help='Send date to all devices in whitelist')


def add_sync_device_command_options(subparsers):
    sd_parser = subparsers.add_parser('sync_device', help='Send date to a given device')
    sd_parser.add_argument('-d',
                           '--device',
                           required=True,
                           action='store',
                           dest='device',
                           help='device to sync')


def add_start_all_command_options(subparsers):
    st_parser = subparsers.add_parser('start_all', help='Start recording on all devices in whitelist')
    st_parser.add_argument('-w','--use_whitelist', action='store_true', default=False, help="Use whitelist instead of continuously scanning for badges")


def pull_devices():
    logger.info('Started')

    conv = lambda x: int(float(x))
    first = True
    badges = {}

    while True:
        logger.info("Scanning for devices...")
        try:
            response = requests.get(BADGES)
            if response.ok:

                for d in response.json():
                    ts = datetime.datetime.fromtimestamp(float('{}.{}'.format(conv(d.get('last_audio_ts')),
                                                                              conv(d.get('last_audio_ts_fract')))))
                    badges[d.get('badge')] = Badge(d.get('badge'),
                                                   logger,
                                                   d.get('key'),
                                                   init_audio_ts=ts,
                                                   init_proximity_ts=conv(d.get('last_proximity_ts'))
                                                   )
            else:
                raise Exception('Got a {} from the server'.format(response.status_code))

        except (requests.exceptions.ConnectionError, Exception) as e:
            logging.error(e)
            if first:
                badges = {mac: Badge(mac,
                                     logger,
                                     key='randomChars',  # Needs to be fixed
                                     init_audio_ts=datetime.datetime(1970, 1, 1),
                                     init_proximity_ts=0,
                                     ) for mac in get_devices()
                          }
        first = False

        scanned_devices = scan_for_devices(badges.keys())

        time.sleep(2)  # ignore this sleep

        for device in scanned_devices:
            # addr = device['mac']
            # if addr not in badges:
            #     logger.debug("Unseen device. Adding to dict: %s" % addr)
            #     # init new badge. set last seen chunk to the time the pull command was called
            #     new_badge = Badge(addr, logger)
            #     # TODO: Call backend and ask for the latest timestamp for a specific mac address
            #     new_badge.set_last_ts(
            #         init_audio_ts, init_audio_ts_fract, init_proximity_ts)
            #     badges[addr] = new_badge

            # badge = badges[addr]
            b = badges.get(device['mac'])

            # TODO: analyze the critical path
            dialogue(b)
            time.sleep(2)  # requires sleep between devices

        logger.info("Sleeping...")
        time.sleep(6)


def sync_all_devices():
    whitelist_devices = get_devices()
    for addr in whitelist_devices:
        bdg = Badge(addr, logger)
        bdg.sync_timestamp()
        time.sleep(2)  # requires sleep between devices

    time.sleep(5)  # allow BLE time to disconnect


def devices_scanner():
    logger.info('Scanning for badges')
    while True:
        whitelist_devices = get_devices()
        logger.info("Scanning for devices...")
        scanned_devices = scan_for_devices(whitelist_devices)
        with open(scans_file_name, "a") as fout:
            for device in scanned_devices:
                mac = device['mac']
                scan_date = device['device_info']['scan_date']
                rssi = device['device_info']['rssi']
                voltage = device['device_info']['adv_payload']['voltage']
                logger.debug("{},{},{:.2f},{:.2f}".format(scan_date, mac, rssi, voltage))
                fout.write("{},{},{:.2f},{:.2f}\n".format(scan_date, mac, rssi, voltage))
        time.sleep(5)  # give time to Ctrl-C


def start_all_devices():
    logger.info('Starting all badges recording.')
    whitelist_devices = get_devices()
    for addr in whitelist_devices:
        bdg = Badge(addr, logger)
        bdg.start_recording()
        time.sleep(2)  # requires sleep between devices
    time.sleep(5)  # allow BLE time to disconnect


if __name__ == "__main__":
    import time
    import argparse

    parser = argparse.ArgumentParser(description="Run scans, send dates, or continuously pull data")
    parser.add_argument('-dr','--disable_reset_ble', action='store_true', default=False, help="Do not reset BLE")

    subparsers = parser.add_subparsers(help='Program mode (e.g. Scan, send dates, pull, scan etc.)', dest='mode')
    add_pull_command_options(subparsers)
    add_scan_command_options(subparsers)
    add_sync_all_command_options(subparsers)
    add_sync_device_command_options(subparsers)
    add_start_all_command_options(subparsers)

    args = parser.parse_args()

    if not args.disable_reset_ble:
        logger.info("Resetting BLE")
        reset()
        time.sleep(2)  # requires sleep after reset
        logger.info("Done resetting BLE")

    if args.mode == "sync_device":
        badge = Badge(args.device, logger)
        badge.sync_timestamp()
        del badge

    if args.mode == "sync_all":
        sync_all_devices()

    # scan for devices
    if args.mode == "scan":
        devices_scanner()

    # pull data from all devices
    if args.mode == "pull":
        pull_devices()

    if args.mode == "start_all":
        start_all_devices()

    exit(0)

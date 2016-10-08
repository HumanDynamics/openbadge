#!/usr/bin/env python

from __future__ import absolute_import, division, print_function

import os
import re
import shlex
import subprocess

import logging


from badge import *
from badge_discoverer import BadgeDiscoverer
from badge_manager_server import BadgeManagerServer
from badge_manager_standalone import BadgeManagerStandalone

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

def dialogue(bdg, activate_audio, activate_proximity):
    """
    Attempts to read data from the device specified by the address. Reading is handled by gatttool.
    :param bdg:
    :return:
    """
    ret = bdg.pull_data(activate_audio, activate_proximity)
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
                    logger.debug("CSV: Chunk timestamp: {}, Voltage: {}, Delay: {}, Samples in chunk: {}".format(
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
                    logger.debug("SCAN: scan timestamp: {}, voltage: {}, Devices in scan: {}".format(
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

        # update badge object to hold latest timestamps
        if len(bdg.dlg.chunks) > 0:
            last_chunk = bdg.dlg.chunks[-1]
            logger.debug("Setting last badge audio timestamp to {} {}".format(last_chunk.ts, last_chunk.fract))
            bdg.set_audio_ts(last_chunk.ts, last_chunk.fract)

        if len(bdg.dlg.scans) > 0:
            last_scan = bdg.dlg.scans[-1]
            logger.debug("Setting last badge proximity timestamp to {}".format(last_scan.ts))
            bdg.last_proximity_ts = last_scan.ts


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

    time.sleep(2)  # requires sometimes to prevent connection from failing
    return scanned_devices


def reset():
    reset_command = "hciconfig hci0 reset"
    args = shlex.split(reset_command)
    p = subprocess.Popen(args)


def add_pull_command_options(subparsers):
    pull_parser = subparsers.add_parser('pull', help='Continuously pull data from badges')
    pull_parser.add_argument('--m', choices=('server', 'standalone'), required=True)
    pull_parser.add_argument('--d', choices=('audio', 'proximity', 'both'), required=False,
                             help='data activation option')

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


def pull_devices(mode, data_activation):
    logger.info('Started pulling')
    activate_audio = False
    activate_proximity = False
    if data_activation is None or data_activation == "both":
        activate_audio = True
        activate_proximity = True
    elif data_activation == "audio":
        activate_audio = True
    elif data_activation == "proximity":
        activate_proximity = True

    logger.info("Data activation: Audio = {}, Proximity = {}".format(activate_audio,activate_proximity))
    mgr = None
    if mode == "server":
        mgr = BadgeManagerServer(logger=logger)
    else:
        mgr = BadgeManagerStandalone(logger=logger)

    while True:
        mgr.pull_badges_list()

        logger.info("Scanning for devices...")
        scanned_devices = scan_for_devices(mgr.badges.keys())

        for device in scanned_devices:
            b = mgr.badges.get(device['mac'])
            # pull data
            dialogue(b, activate_audio, activate_proximity)

            # update timestamps on server
            mgr.send_badge(device['mac'])

            time.sleep(2)  # requires sleep between devices

        #logger.info("Sleeping...")
        #time.sleep(6)


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
        pull_devices(args.m, args.d)

    if args.mode == "start_all":
        start_all_devices()

    exit(0)

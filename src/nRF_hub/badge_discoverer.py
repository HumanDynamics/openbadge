#!/usr/bin/env python

from __future__ import print_function

import itertools
import datetime
import time
import struct
from bluepy import btle


class BadgeDiscoverer:
    """
    Scan for badges
    """
    DEVICE_NAME = "HDBDG"
    DEVICE_NAME_FIELD_ID = 9
    BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME = 0x08
    BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME = 0x09
    BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA = 0xFF
    CUSTOM_DATA_LEN = 14 # length of badge custom data adv
    MAC_LENGTH = 6 # length of a MAC address

    def __init__(self):
        pass

    def discover(self, scan_duration = 1): #seconds
        btle.Debugging = False
        scanner = btle.Scanner().withDelegate(ScanDummy())
        raw_devices = scanner.scan(scan_duration)

        scan_items = {}
        for scan_item in raw_devices:
            device_name = None
            for (sdid, desc, val) in scan_item.getScanData():
                if sdid  == self.DEVICE_NAME_FIELD_ID: device_name = val

            if device_name == self.DEVICE_NAME:
                rssi = scan_item.rssi
                mac = scan_item.addr.upper()
                scan_date = datetime.datetime.now()
                adv_payload = self.unpack_broadcast_data(scan_item.rawData)
                if not (mac in scan_items):
                    scan_items[mac] = {'scan_date':scan_date,'rssi':rssi,'adv_payload':adv_payload}
                else:
                    scan_items[mac]['rssi']=rssi
                    scan_items[mac]['scan_date'] = scan_date

        return scan_items

    @staticmethod
    def print_bytes(data):
        """
        Prints a given string as an array of unsigned bytes
        :param data:
        :return:
        """
        raw_arr = struct.unpack('<%dB' % len(data), data)
        print(raw_arr)

    def unpack_broadcast_data(self, data):
        """
        Extract badge specific data from the broadcasting message
        :param data:
        :return:
        """
        data_length = len(data)
        adv_payload = None
        index = 0
        name = None
        while ((adv_payload is None) | (name is None)) & (index < data_length):
            field_len = struct.unpack('<B', data[index])[0]
            index += 1
            field_type = struct.unpack('<B', data[index])[0]

            # is it a name field?
            if (field_type == self.BLE_GAP_AD_TYPE_SHORT_LOCAL_NAME |
                    field_type == self.BLE_GAP_AD_TYPE_COMPLETE_LOCAL_NAME):
                name_field = data[index+1:index+field_len]
                name_as_bytes = struct.unpack('<%dB' % len(name_field), name_field)
                name = "".join(map(chr, name_as_bytes)) # converts byte to string

            # is it the adv payload?
            elif field_type == self.BLE_GAP_AD_TYPE_MANUFACTURER_SPECIFIC_DATA:
                if field_len == self.CUSTOM_DATA_LEN:
                    payload_field = data[index + 1:index + field_len]
                    payload = struct.unpack('<HBBHB%dB' % self.MAC_LENGTH, payload_field)
                    adv_payload = {}
                    adv_payload['voltage'] =  1 + 0.01*payload[1]
                    adv_payload['status_flags'] = payload[2]
                    adv_payload['badge_id'] = payload[3]
                    adv_payload['project_id'] = payload[4]

                    # Check if the 1st bit is set
                    sync_status = 1 if adv_payload['status_flags'] & 0b1 > 0 else 0
                    adv_payload['sync_status'] = sync_status

                    # Check if the 2nd bit is set:
                    audio_status = 1 if adv_payload['status_flags'] & 0b10 > 0 else 0
                    adv_payload['audio_status'] = audio_status

                    # Check if the 3rd bit is set:
                    proximity_status = 1 if adv_payload['status_flags'] & 0b100 > 0 else 0
                    adv_payload['proximity_status'] = proximity_status

                    mac = payload[5:5+self.MAC_LENGTH]
                    mac = list(mac)
                    mac = mac[::-1]  # reverse
                    adv_payload['mac'] = mac

            # advance to next field
            index += field_len;
        return adv_payload;


class ScanDummy(btle.DefaultDelegate):
    def handleDiscovery(self, dev, is_new_dev, is_new_data):
        pass

if __name__ == "__main__":
    bd = BadgeDiscoverer()

    devices = bd.discover()
    for device in devices:
        print(device,devices[device])

    '''
    str = "0609424144474502010605030F1801000BFF00FF19CA0E4000000303"
    hexData = str.decode("hex")
    import array

    strAsBytes = array.array('B', hexData)
    a = bd.unpackBroadcastData(strAsBytes);
    print(a);
    '''
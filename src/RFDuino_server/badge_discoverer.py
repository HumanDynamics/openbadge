from __future__ import print_function

import datetime
import time

from bluepy import btle

# Looks for badges and return their state (synced or not), the 
# date of the latest scan, and a list of RSSIs
class BadgeDiscoverer:
	def __init__(self):
		self.DEVICE_NAME = "MiniBadge"
		self.CLOCK_STATE_SYNC = "CLKSYN"
		self.CLOCK_STATE_UNSYNC = "CLKUN"
		self.DEVICE_NAME_FIELD_ID = 9
	
	def discover(self, scanDuration = 1): #seconds
		btle.Debugging = False
		scanner = btle.Scanner().withDelegate(ScanDummy())
		raw_devices = scanner.scan(scanDuration)
		
		devices={}
		for device in raw_devices:
			device_name = None
			for (sdid, desc, val) in device.getScanData():
				if sdid  == self.DEVICE_NAME_FIELD_ID: device_name = val

			if device_name == self.DEVICE_NAME:
				rssi = device.rssi
				mac = device.addr.upper()

				is_sync = not(self.CLOCK_STATE_UNSYNC in device.rawData)
				scan_date = datetime.datetime.now()
				if not (mac in devices):
					devices[mac] = {'scan_date':scan_date,'is_sync':is_sync,'rssi':rssi}
				else:
					devices[mac]['rssi']=rssi
					devices[mac]['scan_date'] = scan_date

		return devices

class ScanDummy(btle.DefaultDelegate):
    def handleDiscovery(self, dev, isNewDev, isNewData):
    	pass

if __name__ == "__main__":
	bd = BadgeDiscoverer()
	devices = bd.discover()
	print(devices)
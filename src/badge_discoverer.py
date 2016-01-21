import ble_scanner
import threading
import datetime
import time

# Looks for badges and return their state (synced or not), the 
# date of the latest scan, and a list of RSSIs
class BadgeDiscoverer:
	def __init__(self):
		self.DEVICE_NAME = "MiniBadge"
		self.CLOCK_STATE_SYNC = "CLKSYN"
		self.CLOCK_STATE_UNSYNC = "CLKUN"
		None
	
	def discover(self, scanDuration = 1): #seconds
		bleScanner = ble_scanner.BleScanner()
		
		# stop if there is any running scan, then start a new one
		bleScanner.hci_disable_le_scan()
		bleScanner.hci_le_set_scan_parameters()
		bleScanner.hci_enable_le_scan()
		
		devices={}
		
		t_end = time.time() + scanDuration
		
		# collect devices
		while time.time() < t_end:
			event = bleScanner.parse_events()
			if event is not None:
				device_name = event['name']
				if device_name == self.DEVICE_NAME:
					mac = event['mac']
					rssi = event['rssi']
					is_sync = not(self.CLOCK_STATE_UNSYNC in event['data'])
					scan_date = datetime.datetime.now()
					if not (mac in devices):
						devices[mac] = {'scan_date':scan_date,'is_sync':is_sync,'rssis':[rssi]}
					else:
						devices[mac]['rssis'].append(rssi)
						devices[mac]['scan_date'] = scan_date
				
		# stop scan
		bleScanner.hci_disable_le_scan()
		return devices
	
if __name__ == "__main__":
	bd = BadgeDiscoverer()
	devices = bd.discover()
	print devices
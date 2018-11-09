from __future__ import division, absolute_import, print_function
import time
from integration_test import *

class ScanRecordTimeoutTestCase(IntegrationTest):

	def testCase(self, badge, logger):
		badge.start_microphone(timeout_minutes=1)
		badge.start_scan(timeout_minutes=1)
		badge.start_accelerometer(timeout_minutes=1)
		badge.start_battery(timeout_minutes=1)

		status = badge.get_status()
		self.assertTrue(status.microphone_status)
		self.assertTrue(status.scan_status)
		self.assertTrue(status.accelerometer_status)
		self.assertTrue(status.battery_status)

		time.sleep(59)
		status = badge.get_status()
		self.assertTrue(status.microphone_status)
		self.assertTrue(status.scan_status)
		self.assertTrue(status.accelerometer_status)
		self.assertTrue(status.battery_status)

		time.sleep(121)
		status = badge.get_status()
		self.assertFalse(status.microphone_status)
		self.assertFalse(status.scan_status)
		self.assertFalse(status.accelerometer_status)
		self.assertFalse(status.battery_status)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = ScanRecordTimeoutTestCase(device_addr)
	testCase.runTest()

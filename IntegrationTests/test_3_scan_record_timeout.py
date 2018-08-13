from __future__ import division, absolute_import, print_function
import time
from integration_test import *

class ScanRecordTimeoutTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		badge.start_recording(timeout_minutes=1)
		badge.start_scanning(timeout_minutes=1)

		status = badge.get_status()
		self.assertTrue(status.collector_status)
		self.assertTrue(status.scanner_status)

		time.sleep(59)
		status = badge.get_status()
		self.assertTrue(status.collector_status)
		self.assertTrue(status.scanner_status)

		time.sleep(121)
		status = badge.get_status()
		self.assertFalse(status.collector_status)
		self.assertFalse(status.scanner_status)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = ScanRecordTimeoutTestCase(device_addr)
	testCase.runTest()
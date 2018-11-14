from __future__ import division, absolute_import, print_function
import time
from integration_test import *

class ScanRecordTimeoutTestCase(IntegrationTest):

	def testCase(self, badge, logger):
		# sync time
		badge.get_status()

		# start recording all for 1 minute
		badge.start_microphone(timeout_minutes=1)
		badge.start_scan(timeout_minutes=1)
		badge.start_accelerometer(timeout_minutes=1)
		badge.start_accelerometer_interrupt(timeout_minutes=1)
		badge.start_battery(timeout_minutes=1)

		time.sleep(.25)

		# just make sure it all got started
		status = badge.get_status()
		expected_values = {
			'microphone_status': True,
			'scan_status': True,
			'accelerometer_status': True,
			'accelerometer_interrupt_status': True,
			'battery_status': True,
		}
		self.assertStatusesEqual(status, expected_values)


		# badge should still be recording
		time.sleep(55)
		status = badge.get_status()
		self.assertStatusesEqual(status, expected_values)

		# ok, we should be finished now
		# note that every time you call status (or send any message to the badge),
		# the timer resets
		time.sleep(122)

		for key, val in expected_values:
			expected_values[key] = False

		status = badge.get_status()
		self.assertStatusesEqual(status, expected_values)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = ScanRecordTimeoutTestCase(device_addr)
	testCase.runTest()

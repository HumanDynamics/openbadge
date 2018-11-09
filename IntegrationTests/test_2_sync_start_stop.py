from __future__ import division, absolute_import, print_function
import time
from integration_test import *

class SyncStartStopTestCase(IntegrationTest):


	def assertStatuses(self, status, expected_values):
		self.assertEqual(status.microphone_status,
				 expected_values.microphone_status)
		self.assertEqual(status.scan_status,
				 expected_values.scan_status)
		self.assertEqual(status.accelerometer_status,
				 expected_values.accelerometer_status)
		self.assertEqual(status.battery_status,
				 expected_values.battery_status)

	def testCase(self, badge, logger):
		# Sync time
		logger.info("Syncing time %f", time.time())
		status = badge.get_status()
		time.sleep(.25)

		expected_values = {
			'microphone_status': False,
			'scan_status': False,
			'accelerometer_status': False,
			'battery_status': False,
		}

		# Check that badge now has correct time.
		status = badge.get_status()
		logger.info("Status after time set: {}".format(status))
		# FIRMWARE BUG: Badge time is always off by four seconds. (Why?)
		self.assertAlmostEqual(status.timestamp_seconds, time.time(), delta=1)

		# Start microphone, check that status changes.
		badge.start_microphone()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.microphone_status = True
		self.assertStatuses(status, expected_values)

		# Start scanner, check that status changes.
		badge.start_scan()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.scan_status = True
		self.assertStatuses(status, expected_values)

		# Start accelerometer, check that status changes.
		badge.start_accelerometer()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.accelerometer_status = True
		self.assertStatuses(status, expected_values)

		# Start battery, check that status changes & prev statuses remain.
		badge.start_battery()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.battery_status = True
		self.assertStatuses(status, expected_values)

		# Stop scanner, check that status changes.
		badge.stop_scan()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.scan_status = False
		self.assertStatuses(status, expected_values)

		# Stop battery, check that status changes.
		badge.stop_battery()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.battery_status = False
		self.assertStatuses(status, expected_values)

		# Stop accelerometer, check that status changes.
		badge.stop_accelerometer()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.accelerometer_status = False
		self.assertStatuses(status, expected_values)

		# Stop microphone, check that status changes.
		# Note that microphone will only stop after scanning and recording does.
		# TODO is this still true?
		badge.stop_microphone()
		time.sleep(.25)
		status = badge.get_status()
		expected_values.microphone_status = False
		self.assertStatuses(status, expected_values)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = SyncStartStopTestCase(device_addr)
	testCase.runTest()

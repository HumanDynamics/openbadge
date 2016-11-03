import time
from integration_test import *

class SyncStartStopTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		# Sync time
		logger.info("Syncing time %f", time.time())
		status = badge.get_status()
		time.sleep(.25)

		# Check that badge now has correct time.
		status = badge.get_status()
		logger.info("Status after time set: {}".format(status))
		# FIRMWARE BUG: Badge time is always off by four seconds. (Why?)
		self.assertAlmostEqual(status.timestamp_seconds, time.time(), delta=1)

		# Start collector, check that status changes.
		badge.start_recording()
		time.sleep(.25)
		status = badge.get_status()
		self.assertTrue(status.collector_status)

		# Start scanner, check that status changes.
		badge.start_scanning()
		time.sleep(.25)
		status = badge.get_status()
		self.assertTrue(status.scanner_status)

		# Stop scanner, check that status changes.
		badge.stop_scanning()
		time.sleep(.25)
		status = badge.get_status()
		self.assertFalse(status.scanner_status)

		# Stop collector, check that status changes.
		# Note that collector will only stop after scanning and recording does.
		badge.stop_recording()
		time.sleep(.25)
		status = badge.get_status()
		self.assertFalse(status.collector_status)

if __name__ == "__main__":
	testCase = SyncStartStopTestCase()
	testCase.runTest()
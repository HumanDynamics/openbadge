import time
from integration_test import *

class SyncStartStopTestCase(IntegrationTest):
	def testCase(self, badge):
		# Sync time
		print "Syncing time {}".format(time.time())
		status = badge.get_status()
		print status
		time.sleep(.25)

		# Check that badge now has correct time.
		status = badge.get_status()
		print "Check Status:", status
		self.assertAlmostEqual(status.timestamp_seconds, int(time.time() - 4), delta=1)

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


testCase = SyncStartStopTestCase()
testCase.runTest()
import time
from integration_test import *

class ScanRecordTimeoutTestCase(IntegrationTest):
	def testCase(self, badge):
		print "Starting...", badge.get_status()

		badge.start_recording(timeout_minutes=1)
		badge.start_scanning(timeout_minutes=1)

		for x in range(75):
			# We have to constantly ping the badge to keep the connection alive.
			status = badge.get_status()
			print status
			time.sleep(1)

		status = badge.get_status()
		print status

		self.assertFalse(status.collector_status)
		self.assertFalse(status.scanner_status)


testCase = ScanRecordTimeoutTestCase()
testCase.runTest()
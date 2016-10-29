import time
from integration_test import *

class ClockTestCase(IntegrationTest):
	def testCase(self, badge):
		# Sync current time.
		badge.get_status()

		time.sleep(.25)

		start_time = badge.get_status().timestamp_seconds
		print "Start time:", start_time

		time.sleep(5)

		end_time = badge.get_status().timestamp_seconds
		print "End time:", end_time

		self.assertAlmostEqual(end_time, start_time + 5, delta=1)


testCase = ClockTestCase()
testCase.runTest()
import time
from integration_test import *

class ClockTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		# Sync current time.
		badge.get_status()

		time.sleep(.25)

		start_time = badge.get_status().timestamp_seconds
		logger.info("Start time: %f", start_time)

		time.sleep(5)

		end_time = badge.get_status().timestamp_seconds
		logger.info("End time: %f", end_time)

		self.assertAlmostEqual(end_time, start_time + 5, delta=1)

if __name__ == "__main__":
	testCase = ClockTestCase()
	testCase.runTest()
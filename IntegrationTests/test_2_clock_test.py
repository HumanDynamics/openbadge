from __future__ import division, absolute_import, print_function
import time
from integration_test import *

class ClockTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		# Sync current time.
		badge.get_status()

		time.sleep(.25)

		start_time = badge.get_status().timestamp.seconds
		logger.info("Start time: %f", start_time)

		time.sleep(5)

		end_time = badge.get_status().timestamp.seconds
		logger.info("End time: %f", end_time)

		self.assertAlmostEqual(end_time, start_time + 5, delta=1)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = ClockTestCase(device_addr)
	testCase.runTest()

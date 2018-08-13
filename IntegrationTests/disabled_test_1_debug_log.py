from __future__ import division, absolute_import, print_function
from integration_test import *

class DebugLogTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		debug_log = badge.get_debug_log()
		self.assertEqual(debug_log, "Andrew Rulez!\x00")

if __name__ == '__main__':
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = DebugLogTestCase(device_addr)
	testCase.runTest()
from integration_test import *

class DebugLogTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		debug_log = badge.get_debug_log()
		self.assertEqual(debug_log, "Andrew Rulez!\x00")

if __name__ == '__main__':
	testCase = DebugLogTestCase()
	testCase.runTest()
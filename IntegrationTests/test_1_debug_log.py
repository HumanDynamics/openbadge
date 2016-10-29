from integration_test import *

class DebugLogTestCase(IntegrationTest):
	def testCase(self, badge):
		debug_log = badge.get_debug_log()
		self.assertEqual(debug_log, "Andrew Rulez!\x00")

testCase = DebugLogTestCase()
testCase.runTest()
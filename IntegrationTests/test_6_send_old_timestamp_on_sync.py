from __future__ import division, absolute_import, print_function
import time
from integration_test import *


MAXIMUM_SECONDS_AFTER_START = 5000
class SendOldTimestampOnSyncTestCase(IntegrationTest):
        """
        When a badge updates its internal time, it should respond with the old
        timestamp it is replacing.
        """
        def testCase(self, badge, logger):
                # Badge is currently unsynced. We expect the timestamp to be either
                # zero if it is the old protocol implementation,
                # OR a very small value if it is the new implementation of the old
                #  protocol

                # sync timestamps
                set_time = time.time()
                initial_time = badge.get_status().timestamp_seconds
                
                self.assertTrue(initial_time < MAXIMUM_SECONDS_AFTER_START, msg="""
                    Initial timestamp should be a very small value. 
                    Actual value: {}
                """.format(initial_time))

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = SendOldTimestampOnSyncTestCase(device_addr)
	testCase.runTest()

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
                # Badge is currently unsynced.
                # We expect the initial timestamp to be a small value
                # (should be number of seconds passed since
                # the badge was turned on)


                # sync timestamps
                status = badge.get_status()
                initial_time = status.timestamp.seconds

                self.assertEqual(status.clock_status, 0, msg="""
                    Initial clock status should be 0
                """)

                self.assertTrue(initial_time < MAXIMUM_SECONDS_AFTER_START, msg="""
                    Initial timestamp should be a very small value.
                    Actual value: {}
                """.format(initial_time))

                time.sleep(0.25)

                # just make sure the clock synced afterward
                synced_status = badge.get_status()
                self.assertEqual(synced_status.clock_status, 1, msg="""
                    Synced clock status should be 1, was {}
                """.format(synced_status.clock_status))


if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = SendOldTimestampOnSyncTestCase(device_addr)
	testCase.runTest()

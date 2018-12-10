from __future__ import division, absolute_import, print_function
import time
from integration_test import *


class BadgeSpeedTestCase(IntegrationTest):

    def __init__(self, device_addr, test_duration_minutes=5):
        self.test_duration_minutes = test_duration_minutes
        super(BadgeSpeedTestCase, self).__init__(device_addr)


    def testCase(self, badge, logger):
        """
        Record five minutes worth of microphone data,
        make sure it pulls it at a reasonable speed.
        """
        # sync time
        badge.get_status()

        test_start_time = time.time()
        badge.start_microphone()
        time.sleep(self.test_duration_minutes * 60)
        badge.stop_microphone()

        pull_start_time = time.time()
        badge_data = badge.get_microphone_data(t=test_start_time)
        pull_end_time = time.time()

        pull_duration = pull_end_time - pull_start_time # seconds

        print("It took {} seconds to pull {} minutes worth of data"
                .format(pull_duration, self.test_duration_minutes))

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Please enter badge MAC address")
        exit(1)
    device_addr = sys.argv[1]


    if len(sys.argv) == 3:
        test_duration = sys.argv[2]
        print("starting speed test w/ length {} minutes".format(test_duration))
        testCase = BadgeSpeedTestCase(device_addr, test_duration_minutes=test_duration)
        testCase.runTest()
    else:
        print("starting speed test w/ length 5 minutes")
        testCase = BadgeSpeedTestCase(device_addr)
        testCase.runTest()


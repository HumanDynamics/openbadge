from __future__ import division, absolute_import, print_function
import time
from integration_test import *


# badges sample voltage every minute iirc
WAIT_FOR_SAMPLE = 65

class ReceiveDataTestCase(IntegrationTest):
    """
    Ensure we receive data from the badges

    Ignore proximity/scan because we don't receive data
      if there are no other devices around

    Ignore accelerometer_interrupt because we don't receive
      interrupt data unless someone moves the badge during testing
    """

    def testCase(self, badge, logger):
        # sync badge
        badge.get_status()

        ts = time.time()
        time.sleep(WAIT_FOR_SAMPLE)

        # sanity check - should not have any data because we havent been recording
        self.assertTrue(len(badge.get_microphone_data(ts)) == 0)
        self.assertTrue(len(badge.get_battery_data(ts)) == 0)
        self.assertTrue(len(badge.get_accelerometer_data(ts)) == 0)

        # record
        badge.start_battery()
        badge.start_microphone()
        badge.start_accelerometer()

        # make sure we wait long enough to have collected samples
        time.sleep(WAIT_FOR_SAMPLE)

        self.assertFalse(len(badge.get_microphone_data(ts)) == 0)
        self.assertFalse(len(badge.get_battery_data(ts)) == 0)
        self.assertFalse(len(badge.get_accelerometer_data(ts)) == 0)



if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Please enter badge MAC address")
        exit(1)

    device_addr = sys.argv[1]

    testCase = ReceiveDataTestCase(device_addr)
    testCase.runTest()


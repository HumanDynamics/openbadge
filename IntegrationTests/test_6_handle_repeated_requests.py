from __future__ import division, absolute_import, print_function
import time
from integration_test import *

sys.path.append('../BadgeFramework')
from badge import timestamps_to_time

BADGE_TIMER_TICK_RATE=101

def sync(badge):
    status = badge.get_status()
    time.sleep(.25)

class RepeatedRecordRequestTestCase(IntegrationTest):

    def startAllRecorders(self, badge, timeout):
        badge.start_microphone(timeout_minutes=timeout)
        badge.start_scan(timeout_minutes=timeout)
        badge.start_accelerometer(timeout_minutes=timeout)
        badge.start_accelerometer_interrupt(timeout_minutes=timeout)
        badge.start_battery(timeout_minutes=timeout)

    def testTimeout(self, badge, logger):
        sync(badge)

        # set to some relatively long timeout
        # we're going to change it anyway
        print("starting rec")
        self.startAllRecorders(badge, 20)

        print("confirming start worked")
        status = badge.get_status()
        expected_values = {
            'microphone_status': True,
            'scan_status': True,
            'accelerometer_status': True,
            'accelerometer_interrupt_status': True,
            'battery_status': True,
        }
        self.assertStatusesEqual(status, expected_values)

        print("pass a newer, shorter timeout")
        self.startAllRecorders(badge, 1)

        # in case we got bad RNG, wait for the maximum
        # timer tick rate
        # otherwise we get false failures
        # TODO maybe this changed in new implementation?
        time.sleep(BADGE_TIMER_TICK_RATE)

        print("We expect the badge to have used the more recent, shorter timeout")
        status = badge.get_status()
        for key, val in expected_values.iteritems():
            expected_values[key] = False
        self.assertStatusesEqual(status, expected_values)


        print("Start all recorders w/ 1 minutes timeout")
        # repeat as above but from shorter -> longer
        self.startAllRecorders(badge, 1)

        time.sleep(0.25)
        status = badge.get_status()
        # check to make sure start worked
        for key, val in expected_values.iteritems():
            expected_values[key] = True
        self.assertStatusesEqual(status, expected_values)

        print("Change timeout to 3 minutes")
        self.startAllRecorders(badge, 3)

        time.sleep(BADGE_TIMER_TICK_RATE)

        print("We expect the badge to have used the more recent, longer timeout")
        status = badge.get_status()
        self.assertStatusesEqual(status, expected_values)


    def checkTimestampContinuity(self, data, expected_delta):
        first_chunk  = data[0]
        second_chunk = data[1]

        first_time =  timestamps_to_time(
                first_chunk.timestamp.seconds,
                first_chunk.timestamp.ms)

        last_time = timestamps_to_time(
                second_chunk.timestamp.seconds,
                second_chunk.timestamp.ms)

        last_diff = last_time - first_time

        for ele in data[2:]:
            secs = ele.timestamp.seconds
            millis = ele.timestamp.ms

            current_time = timestamps_to_time(secs, millis)

            current_diff = current_time - last_time

            self.assertAlmostEqual(current_diff, last_diff, delta=expected_delta, msg="""
            The difference between the sample timestamps should be approximately equal
            Actual difference: {}
            Expected within: {}
            """.format(abs(current_diff - last_diff), expected_delta))

            last_time = current_time
            last_diff = current_diff

    def testRecordNotRestarted(self, badge, logger):
        sync(badge)

        badge.start_microphone(timeout_minutes=5)

        test_start_time = time.time()
        time.sleep(60)

        # allow the badges to run the scan for a little while

        # send new record/scan requests, sleep for a min, repeat
        # doing this multiple times to make sure we're not
        # getting false positive from RNG
        for i in range(3):
            badge.start_microphone(timeout_minutes=5)
            time.sleep(60)

        # stop recording/scanning and inspect the data
        badge.stop_microphone()

        # Normally, the start-timepoints that are written to the chunk
        # should be more or less equidistant.
        # So if the sampling is restarted,
        # this shouldn't be the case anymore.
        print("checking mic data")
        mic_data = badge.get_microphone_data(t=test_start_time)
        self.checkTimestampContinuity(mic_data, 0.3)



    def testCase(self, badge, logger):
        """
        When the badge receives a repeated record request,
            - it should use the timeout value of the newest request
            - it should not restart the actual recording process
                i.e. we should see no disruption in the data
        """

        self.testRecordNotRestarted(badge, logger)
        badge = self.restart_badge(badge)
        self.testTimeout(badge, logger)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Please enter badge MAC address")
        exit(1)
    device_addr = sys.argv[1]

    print("creating testcase")
    testCase = RepeatedRecordRequestTestCase(device_addr)
    testCase.runTest()

from __future__ import division, absolute_import, print_function
import time
from integration_test import *

sys.path.append('../BadgeFramework')
from badge import timestamps_to_time
from badge_protocol import MicrophoneDataHeader

BADGE_TIMER_TICK_RATE=101

# NOTE - milliseconds is spelled wrong in the badge protocol
def sync(badge):
    status = badge.get_status()
    time.sleep(.25)

class RepeatedRecordRequestTestCase(IntegrationTest):
    
    def testTimeout(self, badge, logger):
        sync(badge)

        # set to some relatively long timeout
        # we're going to change it anyway
        print("starting rec")
        badge.start_recording(timeout_minutes=20)
        badge.start_scanning(timeout_minutes=20)

        
        print("confirming start worked")
        status = badge.get_status()
        self.assertTrue(status.collector_status)
        self.assertTrue(status.scanner_status)

        print("pass a newer, shorter timeout")
        badge.start_recording(timeout_minutes=1)
        badge.start_scanning(timeout_minutes=1)
        
        # in case we got bad RNG, wait for the maximum
        # timer tick rate
        # otherwise we get false failures
        time.sleep(BADGE_TIMER_TICK_RATE)

        print("we expect the badge to have used the more recent, shorter timeout")
        status = badge.get_status()
        self.assertFalse(status.collector_status, msg="""
        Audio recording should have timed out! 
        Initial timeout set to 20 minutes.
        Second timeout set to 1 minute.
        Still recording {} seconds after second timeout set.
        """.format(BADGE_TIMER_TICK_RATE))
        self.assertFalse(status.scanner_status, msg="""
        Proximity recording should have timed out! 
        Initial timeout set to 20 minutes.
        Second timeout set to 1 minute.
        Still recording {} seconds after second timeout set.
        """.format(BADGE_TIMER_TICK_RATE))

        # just in case
        badge.stop_recording()
        badge.stop_scanning()

        print("lets go again")
        # repeat as above but from shorter -> longer
        badge.start_recording(timeout_minutes=1)
        badge.start_scanning(timeout_minutes=1)

        status = badge.get_status()
        self.assertTrue(status.collector_status)
        self.assertTrue(status.scanner_status)

        print("change timeout to longer")
        badge.start_recording(timeout_minutes=3)
        badge.start_scanning(timeout_minutes=3)

        time.sleep(BADGE_TIMER_TICK_RATE)

        # we expect the badge to have used the more recent, longer timeout
        status = badge.get_status()
        self.assertTrue(status.collector_status, msg="""
        Audio recording should not have timed out! 
        Initial timeout set to 1 minute.
        Second timeout set to 3 minutes.
        No longer recording {} seconds after second timeout set.
        """.format(BADGE_TIMER_TICK_RATE))
        self.assertTrue(status.scanner_status, msg="""
        Proximity recording should not have timed out! 
        Initial timeout set to 1 minute.
        Second timeout set to 3 minutes.
        No longer recording {} seconds after second timeout set.
        """.format(BADGE_TIMER_TICK_RATE))
        


    def getMillis(self, header):
        # because scan headers don't have milliseconds apparently?
        if isinstance(header, MicrophoneDataHeader):
            return header.timestamp_miliseconds 
        else:
            return 0

    def checkTimestampContinuity(self, data, expected_delta):
        first_chunk_header, _  = data[0]
        second_chunk_header, _ = data[1]

        first_time =  timestamps_to_time(
                first_chunk_header.timestamp_seconds,
                self.getMillis(first_chunk_header))

        last_time = timestamps_to_time(
                second_chunk_header.timestamp_seconds,
                self.getMillis(second_chunk_header))
        last_diff = last_time - first_time

        for header, _ in data[2:]:
            secs = header.timestamp_seconds
            millis = self.getMillis(header)
                

            current_time = timestamps_to_time(secs, millis)

            current_diff = current_time - last_time
            self.assertAlmostEqual(current_diff, last_diff, delta=expected_delta, msg="""
            The difference between the sample timestamps should be approximately equal
            Actual differnce: {}
            Expected within: {}
            """.format(abs(current_diff - last_diff), expected_delta))
                
            last_time = current_time
            last_diff = current_diff

    def testRecordNotRestarted(self, badge, logger):
        sync(badge)

        badge.start_recording(timeout_minutes=5)
        badge.start_scanning(timeout_minutes=5)

        test_start_time = time.time()
        time.sleep(60)
        
        # allow the badges to run the scan for a little while
        
        # send new record/scan requests, sleep for a min, repeat
        # doing this multiple times to make sure we're not 
        # getting false positive from RNG
        for i in range(3):
            badge.start_recording(timeout_minutes=5)
            badge.start_scanning(timeout_minutes=5)
            time.sleep(60)

        # stop recording/scanning and inspect the data
        badge.stop_recording()
        badge.stop_scanning()

        # Normally, the start-timepoints that are written to the chunk 
        # should be more or less equidistant. 
        # So when the sampling would be restarted,
        # this shouldn't be the case anymore.
             

        print("checking mic data")
        mic_data = badge.get_mic_data(t=test_start_time)
        self.checkTimestampContinuity(mic_data, 0.3)

        print("checking scan data")
        scan_data = badge.get_scan_data(t=test_start_time)
        self.checkTimestampContinuity(scan_data, 2)
        

    def testCase(self, badge, logger):
        """ 
        When the badge receives a repeated record request,
            - it should use the timeout value of the newest request
            - it should not restart the actual recording process
                i.e. we should see no disruption in the data
        """

        self.testRecordNotRestarted(badge, logger)
        # restart badge to clean slate for next test
        restart_badge(self.uartSerial)
        self.testTimeout(badge, logger)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Please enter badge MAC address")
        exit(1)
    device_addr = sys.argv[1]

    print("creating testcase")
    testCase = RepeatedRecordRequestTestCase(device_addr)
    testCase.runTest()

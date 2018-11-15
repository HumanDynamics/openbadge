from __future__ import division, absolute_import, print_function
import time
import sys
from integration_test import *

sys.path.append('../BadgeFramework')
from badge import timestamps_to_time

TEST_LENGTH_SECONDS = 3 * 60
SAMPLE_PERIOD_TICKS = 1638.0
NRF_CLOCK_FREQ = 32768.0
SAMPLE_PERIOD_MS = SAMPLE_PERIOD_TICKS * (1000.0 / NRF_CLOCK_FREQ)
SAMPLES_PER_SECOND = 1000 / SAMPLE_PERIOD_MS

# Maximum allowed delay between recording start command sent and first sample recorded in seconds.
MAX_ALLOWED_STARTUP_DELAY = 10

class RecordNoGapsTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		# Sync time
		status = badge.get_status()

		# Set this here (before 0.25sec wait) because of the moving average clock mechanism on the badge
		# TODO what?
		test_start_time = time.time()
		time.sleep(.25)		
		badge.start_microphone()
		time.sleep(TEST_LENGTH_SECONDS)
		badge.stop_microphone()

		mic_data = badge.get_microphone_data(t=test_start_time)

		num_samples_taken = 0

		# We give extra leway on the first chunk to allow for startup time.
		first_chunk = mic_data[0]
		first_chunk_time = timestamps_to_time(first_chunk.timestamp.seconds, first_chunk.timestamp.ms)
		self.assertAlmostEqual(test_start_time, first_chunk_time, delta=MAX_ALLOWED_STARTUP_DELAY)
		# If we passed our startup delay test, use our first_chunk_time to calibrate all other expected times
		expected_next_chunk_time = first_chunk_time
		for data in mic_data:
			# <data> has four keys:
			# 'sample_period_ms': an int
			# 'timestamp': an object with two key/val pairs, 'seconds' and 'ms'
			# 'microphone_data': an array of objects with a single key/val pair, 'value'
			# 'last_response': an int, TODO on what it means

			num_samples_taken += len(data.microphone_data)

			# Check that timestamps are continous
			sample_time = timestamps_to_time(data.timestamp.seconds, data.timestamp.ms)
			self.assertAlmostEqual(expected_next_chunk_time, sample_time, delta=0.005)
			print("Chunk {}: OK".format(data))
			expected_next_chunk_time = sample_time + (float(len(data.microphone_data)) / SAMPLES_PER_SECOND)

		# Check that there were the correct number of total samples for the amount of time spent recording
		actual_test_duration = expected_next_chunk_time - first_chunk_time
		expected_num_samples = actual_test_duration * SAMPLES_PER_SECOND
		self.assertAlmostEqual(TEST_LENGTH_SECONDS, actual_test_duration, delta=5) # Increased to 5 becuase we don't send partial chunks
		self.assertAlmostEqual(num_samples_taken, expected_num_samples, delta=1)

if __name__ == "__main__":
	if len(sys.argv) != 2:
		print("Please enter badge MAC address")
		exit(1)
	device_addr = sys.argv[1]

	testCase = RecordNoGapsTestCase(device_addr)
	testCase.runTest()

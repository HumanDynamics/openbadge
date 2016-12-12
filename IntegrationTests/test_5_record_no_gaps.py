import time
from integration_test import *
from BadgeFramework.badge import timestamps_to_time

TEST_LENGTH_SECONDS = 3 * 60;
SAMPLE_PERIOD_MS = 50
SAMPLES_PER_SECOND = 1000 / SAMPLE_PERIOD_MS

# Maximum allowed delay between recording start command sent and first sample recorded in seconds.
MAX_ALLOWED_STARTUP_DELAY = 10

class RecordNoGapsTestCase(IntegrationTest):
	def testCase(self, badge, logger):
		# Sync time
		status = badge.get_status()
		time.sleep(.25)

		badge.start_recording()
		test_start_time = time.time()
		time.sleep(TEST_LENGTH_SECONDS)
		badge.stop_recording()

		mic_data = badge.get_mic_data(timestamp_seconds=test_start_time)

		num_samples_taken = 0

		# We give extra leway on the first chunk to allow for startup time.
		first_chunk_header, first_chunk_data = mic_data[0]
		first_chunk_time = timestamps_to_time(first_chunk_header.timestamp_seconds, first_chunk_header.timestamp_miliseconds)
		self.assertAlmostEqual(test_start_time, first_chunk_time, delta=MAX_ALLOWED_STARTUP_DELAY)
		# If we passed our startup delay test, use our first_chunk_time to calibrate all other expected times
		expected_next_chunk_time = first_chunk_time

		for header, data in mic_data:
			# Check that there's the correct number of samples
			self.assertEqual(header.num_samples_in_chunk, len(data))
			num_samples_taken += header.num_samples_in_chunk

			# Check that timestamps are continous
			sample_time = timestamps_to_time(header.timestamp_seconds, header.timestamp_miliseconds)
			self.assertAlmostEqual(expected_next_chunk_time, sample_time, delta=0.0014)
			expected_next_chunk_time = sample_time + (float(header.num_samples_in_chunk) / SAMPLES_PER_SECOND)

		# Check that there were the correct number of total samples for the amount of time spent recording
		actual_test_duration = expected_next_chunk_time - first_chunk_time
		expected_num_samples = actual_test_duration * SAMPLES_PER_SECOND
		self.assertAlmostEqual(TEST_LENGTH_SECONDS, actual_test_duration, delta=2.5)
		self.assertAlmostEqual(num_samples_taken, expected_num_samples, delta=1)

if __name__ == "__main__":
	testCase = RecordNoGapsTestCase()
	testCase.runTest()
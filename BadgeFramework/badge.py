from __future__ import division, absolute_import, print_function
import time
import logging
import sys

# Default scan settings from openbadge-hub-py.
DEFAULT_SCAN_WINDOW = 100
DEFAULT_SCAN_INTERVAL = 300
DEFAULT_SCAN_DURATION = 5
DEFAULT_SCAN_PERIOD = 60

from badge_protocol import *

logger = logging.getLogger(__name__)

# -- Helper methods used often in badge communication -- 

# We generally define timestamp_seconds to be in number of seconds since UTC epoch
# and timestamp_miliseconds to be the miliseconds portion of that UTC timestamp.

# Returns the current timestamp as two parts - seconds and milliseconds
def get_timestamps():
	return get_timestamps_from_time(time.time())

# Returns the given time as two parts - seconds and milliseconds
def get_timestamps_from_time(t):
	timestamp_seconds = int(t)
	timestamp_fraction_of_second = t - timestamp_seconds
	timestamp_miliseconds = int(1000 * timestamp_fraction_of_second)
	return (timestamp_seconds, timestamp_miliseconds)
	

# Convert badge timestamp representation to python representation
def timestamps_to_time(timestamp_seconds, timestamp_miliseconds):
	return float(timestamp_seconds) + (float(timestamp_miliseconds) / 1000.0)

# Returns true if a given header message indicates end of stream. (i.e. last chunk)
# See badge communication protocols for more information.
def is_end_header(header_message):
	# The end header is defined to be a header full of 0s. This is kind of hacky...
	return header_message.serialize_message() == chr(0) * header_message.length()

# Represents an OpenBadge currently connected via the BadgeConnection 'connection'.
#    The 'connection' should already be connected when it is used to initialize this class.
# Implements methods that allow for interaction with that badge. 
class OpenBadge(object):
	def __init__(self, connection):
		self.connection = connection

	# Helper function to send a BadgeMessage `command_message` to a device, expecting a response
	# of class `response_type` that is a subclass of BadgeMessage, or None if no response is expected.
	def send_command(self, command_message, response_type):
		expected_response_length = response_type.length() if response_type else 0

		serialized_command = command_message.serialize_message()
		logger.debug("Sending: {}, Raw: {}".format(command_message, serialized_command.encode("hex")))
		serialized_response = self.connection.send(serialized_command, response_len=expected_response_length)

		if expected_response_length > 0:
			response = response_type.deserialize_message(serialized_response)
			logger.info("Recieved response {}".format(response))
			return response
		else:
			logger.info("No response expected, transmission successful.")
			return True

	# Sends a status request to this Badge.
	#   Optional fields new_id and new_group number will set the badge's id
	#     and group number. They must be sent together. 
	# Returns a StatusResponse() representing badge's response.
	def get_status(self, t=None, new_id=None, new_group_number=None):
		if t is None:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps_from_time(t)

		status_request = StatusRequest(timestamp_seconds, timestamp_miliseconds, badge_id=new_id, group_number=new_group_number)

		return self.send_command(status_request, StatusResponse)

	# Sends a request to the badge to start recording microphone data.
	#  timeout_minutes is the number of minutes the badge is to record, 
	#    or 0 if the badge is to scan indefinetely.
	# Returns a StartRecordResponse() representing the badges response.
	def start_recording(self, t=None, timeout_minutes=0):
		if t is None:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps_from_time(t)

		start_record_request = StartRecordRequest(timestamp_seconds, timestamp_miliseconds, timeout_minutes)

		return self.send_command(start_record_request, StartRecordResponse)

	# Sends a request to the badge to stop recording.
	# Returns True if request was successfuly sent.
	def stop_recording(self):
		stop_record_request = StopRecordRequest()
		return self.send_command(stop_record_request, None)

	# Sends a request to the badge to start performing scans and collecting scan data.
	#   timeout_minutes is the number of minutes the badge is to collect scan data for
	#     0 if the badge is to scan and collect indefinetely.
	#   duration_seconds is how many seconds each scan operation is to last for (0 for firmware default)
	#   period_seconds is how often the badge should scan (0 for firmware default)
	#   window_miliseconds and interval_miliseconds controls radio duty cycle during scanning (0 for firmware default)
	#     radio is active for [window_miliseconds] every [interval_miliseconds]
	# Returns a StartScanningResponse() representing the badge's response.
	def start_scanning(self, t=None,
	   timeout_minutes=0, window_miliseconds=DEFAULT_SCAN_WINDOW, interval_miliseconds=DEFAULT_SCAN_INTERVAL,
	    duration_seconds=DEFAULT_SCAN_DURATION, period_seconds=DEFAULT_SCAN_PERIOD):
		if t is None:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps_from_time(t)
		
		start_scanning_request = StartScanningRequest(timestamp_seconds, timestamp_miliseconds, timeout_minutes, 
			window_miliseconds, interval_miliseconds, duration_seconds, period_seconds)

		return self.send_command(start_scanning_request, StartScanningResponse)

	# Sends a request to the badge to stop scanning.
	# Returns True if request was successfuly sent.
	def stop_scanning(self):
		stop_scanning_request = StopScanningRequest()
		return self.send_command(stop_scanning_request, None)

	# Send a request to the badge to light an led to identify its self.
	#   If duration_seconds == 0, badge will turn off LED if currently lit.
	# Returns True if request was successfuly sent. 
	def identify(self, duration_seconds=10):
		identify_request = IdentifyRequest(duration_seconds)
		return self.send_command(identify_request, None)

	# Send a request to the badge for recorded microphone data starting at the given timestamp.
	# Returns a list of tuples of (MicrophoneDataHeader(), microphone_sample_chunk_data), where each tuple
	#   contains one chunk of microphone data. 
	def get_mic_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps_from_time(t)


		mic_data_request = MicrophoneDataRequest(timestamp_seconds, timestamp_miliseconds)
		header = self.send_command(mic_data_request, MicrophoneDataHeader)

		chunks_and_headers = []

		while not is_end_header(header):
			bytes_awaited = header.num_samples_in_chunk
			logger.debug("Awaiting {} bytes.".format(bytes_awaited))
			data = self.connection.await_data(bytes_awaited)

			# TODO: better to use a deserializer here to ensure we are using the correct data type
			chunks_and_headers.append((header, map(ord, data)))

			serialized_header = self.connection.await_data(MicrophoneDataHeader.length())
			header = MicrophoneDataHeader.deserialize_message(serialized_header)

		return chunks_and_headers

	# Sends a request to the badge for recorded scan data starting at the the given timestamp.
	# Returns a list of tuples of (ScanDataHeader(), [ScanDataDevice(), ScanDataDevice(), ...])
	#   where each tuple contains a header and a list of devices seen from one scan. 
	def get_scan_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_miliseconds) = get_timestamps_from_time(t)


		scan_data_request = ScanDataRequest(timestamp_seconds)
		header = self.send_command(scan_data_request, ScanDataHeader)

		scan_headers_and_devices_seen = []

		while not is_end_header(header):
			bytes_awaited = header.num_devices_seen * ScanDataDevice.length()
			logger.debug("Awaiting {} bytes".format(bytes_awaited))
			data = self.connection.await_data(bytes_awaited)

			devices = []
			if data:
				for i in range(0, len(data), ScanDataDevice.length()):
					devices.append(ScanDataDevice.deserialize_message(data[i:i+ScanDataDevice.length()]))

			scan_headers_and_devices_seen.append((header, devices))

			serialized_header = self.connection.await_data(ScanDataHeader.length())
			logger.debug("Attempting to deserialize header {} of length {}".format(serialized_header.encode("hex"), len(serialized_header)))
			header = ScanDataHeader.deserialize_message(serialized_header)

		return scan_headers_and_devices_seen

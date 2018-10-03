from __future__ import division, absolute_import, print_function
import time
import logging
import sys
import struct
import Queue

DEFAULT_MICROPHONE_SAMPLING_PERIOD_MS = 50

DEFAULT_SCAN_WINDOW = 100
DEFAULT_SCAN_INTERVAL = 300
DEFAULT_SCAN_DURATION = 5
DEFAULT_SCAN_PERIOD = 60
DEFAULT_SCAN_AGGREGATION_TYPE = 0

DEFAULT_ACCELEROMETER_OPERATING_MODE = 1
DEFAULT_ACCELEROMETER_FULL_SCALE = 4
DEFAULT_ACCELEROMETER_DATARATE = 10
DEFAULT_ACCELEROMETER_FIFO_SAMPLING_PERIOD_MS = 1000

DEFAULT_ACCELEROMETER_INTERRUPT_THRESHOLD_MG = 250
DEFAULT_ACCELEROMETER_INTERRUPT_MINIMAL_DURATION_MS = 10
DEFAULT_ACCELEROMETER_INTERRUPT_IGNORE_DURATION_MS = 5000

DEFAULT_BATTERY_SAMPLING_PERIOD_MS = 15000



DEFAULT_MICROPHONE_STREAM_SAMPLING_PERIOD_MS = 50

DEFAULT_SCAN_STREAM_WINDOW = 100
DEFAULT_SCAN_STREAM_INTERVAL = 300
DEFAULT_SCAN_STREAM_DURATION = 5
DEFAULT_SCAN_STREAM_PERIOD = 6
DEFAULT_SCAN_STREAM_AGGREGATION_TYPE = 0

DEFAULT_ACCELEROMETER_STREAM_OPERATING_MODE = 1
DEFAULT_ACCELEROMETER_STREAM_FULL_SCALE = 4
DEFAULT_ACCELEROMETER_STREAM_DATARATE = 25
DEFAULT_ACCELEROMETER_STREAM_FIFO_SAMPLING_PERIOD_MS = 50

DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_THRESHOLD_MG = 250
DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_MINIMAL_DURATION_MS = 10
DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_IGNORE_DURATION_MS = 1000

DEFAULT_BATTERY_STREAM_SAMPLING_PERIOD_MS = 5000

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
	timestamp_ms = int(1000 * timestamp_fraction_of_second)
	return (timestamp_seconds, timestamp_ms)
	

# Convert badge timestamp representation to python representation
def timestamps_to_time(timestamp_seconds, timestamp_miliseconds):
	return float(timestamp_seconds) + (float(timestamp_miliseconds) / 1000.0)


# Represents an OpenBadge currently connected via the BadgeConnection 'connection'.
#    The 'connection' should already be connected when it is used to initialize this class.
# Implements methods that allow for interaction with that badge. 
class OpenBadge(object):
	def __init__(self, connection):
		self.connection = connection
		self.status_response_queue = Queue.Queue()
		self.start_microphone_response_queue = Queue.Queue()
		self.start_scan_response_queue = Queue.Queue()
		self.start_accelerometer_response_queue = Queue.Queue()
		self.start_accelerometer_interrupt_response_queue = Queue.Queue()
		self.start_battery_response_queue = Queue.Queue()
		self.microphone_data_response_queue = Queue.Queue()
		self.scan_data_response_queue = Queue.Queue()
		self.accelerometer_data_response_queue = Queue.Queue()
		self.accelerometer_interrupt_data_response_queue = Queue.Queue()
		self.battery_data_response_queue = Queue.Queue()
		self.test_response_queue = Queue.Queue()
		self.stream_response_queue = Queue.Queue()

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
	
	
	def send_request(self, request_message):
		serialized_request = request_message.encode()
		
		# Adding length header:
		serialized_request_len = struct.pack('>H', len(serialized_request))
		serialized_request = serialized_request_len + serialized_request
	
		logger.debug("Sending: {}, Raw: {}".format(request_message, serialized_request.encode("hex")))
		
		self.connection.send(serialized_request, response_len = 0)
		
		
	
	def receive_response(self):
		response_len = struct.unpack('>H', self.connection.await_data(2))[0]
		print("Wait response len: " + str(response_len))
		serialized_response = self.connection.await_data(response_len)
		
		response_message = Response.decode(serialized_response)
		
		queue_options = {
			Response_status_response_tag: self.status_response_queue,
			Response_start_microphone_response_tag: self.start_microphone_response_queue,
			Response_start_scan_response_tag: self.start_scan_response_queue,
			Response_start_accelerometer_response_tag: self.start_accelerometer_response_queue,
			Response_start_accelerometer_interrupt_response_tag: self.start_accelerometer_interrupt_response_queue,
			Response_start_battery_response_tag: self.start_battery_response_queue,
			Response_microphone_data_response_tag: self.microphone_data_response_queue,
			Response_scan_data_response_tag: self.scan_data_response_queue,
			Response_accelerometer_data_response_tag: self.accelerometer_data_response_queue,
			Response_accelerometer_interrupt_data_response_tag: self.accelerometer_interrupt_data_response_queue,
			Response_battery_data_response_tag: self.battery_data_response_queue,
			Response_test_response_tag: self.test_response_queue,
			Response_stream_response_tag: self.stream_response_queue,
		}
		response_options = {
			Response_status_response_tag: response_message.type.status_response,
			Response_start_microphone_response_tag: response_message.type.start_microphone_response,
			Response_start_scan_response_tag: response_message.type.start_scan_response,
			Response_start_accelerometer_response_tag: response_message.type.start_accelerometer_response,
			Response_start_accelerometer_interrupt_response_tag: response_message.type.start_accelerometer_interrupt_response,
			Response_start_battery_response_tag: response_message.type.start_battery_response,
			Response_microphone_data_response_tag: response_message.type.microphone_data_response,
			Response_scan_data_response_tag: response_message.type.scan_data_response,
			Response_accelerometer_data_response_tag: response_message.type.accelerometer_data_response,
			Response_accelerometer_interrupt_data_response_tag: response_message.type.accelerometer_interrupt_data_response,
			Response_battery_data_response_tag: response_message.type.battery_data_response,
			Response_test_response_tag: response_message.type.test_response,
			Response_stream_response_tag: response_message.type.stream_response,
		}
		queue_options[response_message.type.which].put(response_options[response_message.type.which])
		
		
		
	
	# Sends a status request to this Badge.
	#   Optional fields new_id and new_group number will set the badge's id
	#     and group number. They must be sent together. 
	# Returns a StatusResponse() representing badge's response.
	def get_status(self, t=None, new_id=None, new_group_number=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
	
		request = Request()
		request.type.which = Request_status_request_tag
		request.type.status_request = StatusRequest()
		request.type.status_request.timestamp = Timestamp()
		request.type.status_request.timestamp.seconds = timestamp_seconds
		request.type.status_request.timestamp.ms = timestamp_ms
		if(not ((new_id is None) or (new_group_number is None)) ):
			request.type.status_request.badge_assignement = BadgeAssignement()
			request.type.status_request.badge_assignement.ID = new_id
			request.type.status_request.badge_assignement.group = new_group_number
			request.type.status_request.has_badge_assignement = True
			
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.status_response_queue.mutex:
			self.status_response_queue.queue.clear()
			
		while(self.status_response_queue.empty()):
			self.receive_response()
			
		return self.status_response_queue.get()
		
		

	# Sends a request to the badge to start recording microphone data.
	#  timeout_minutes is the number of minutes the badge is to record, 
	#    or 0 if the badge is to scan indefinetely.
	# Returns a StartRecordResponse() representing the badges response.
	def start_microphone(self, t=None, timeout_minutes=0, sampling_period_ms=DEFAULT_MICROPHONE_SAMPLING_PERIOD_MS):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
			
		request = Request()
		request.type.which = Request_start_microphone_request_tag
		request.type.start_microphone_request = StartMicrophoneRequest()
		request.type.start_microphone_request.timestamp = Timestamp()
		request.type.start_microphone_request.timestamp.seconds = timestamp_seconds
		request.type.start_microphone_request.timestamp.ms = timestamp_ms
		request.type.start_microphone_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_microphone_request.period_ms = sampling_period_ms
		
		self.send_request(request)
		
		
		with self.start_microphone_response_queue.mutex:
			self.start_microphone_response_queue.queue.clear()
			
		while(self.start_microphone_response_queue.empty()):
			self.receive_response()
			
		return self.start_microphone_response_queue.get()

	# Sends a request to the badge to stop recording.
	# Returns True if request was successfuly sent.
	def stop_microphone(self):
	
		request = Request()
		request.type.which = Request_stop_microphone_request_tag
		request.type.stop_microphone_request = StopMicrophoneRequest()
		
		self.send_request(request)
		
		

	# Sends a request to the badge to start performing scans and collecting scan data.
	#   timeout_minutes is the number of minutes the badge is to collect scan data for
	#     0 if the badge is to scan and collect indefinetely.
	#   duration_seconds is how many seconds each scan operation is to last for (0 for firmware default)
	#   period_seconds is how often the badge should scan (0 for firmware default)
	#   window_miliseconds and interval_miliseconds controls radio duty cycle during scanning (0 for firmware default)
	#     radio is active for [window_miliseconds] every [interval_miliseconds]
	# Returns a StartScanningResponse() representing the badge's response.
	def start_scan(self, t=None,
	   timeout_minutes=0, window_ms=DEFAULT_SCAN_WINDOW, interval_ms=DEFAULT_SCAN_INTERVAL,
	    duration_seconds=DEFAULT_SCAN_DURATION, period_seconds=DEFAULT_SCAN_PERIOD, aggregation_type=DEFAULT_SCAN_AGGREGATION_TYPE):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
		request = Request()
		request.type.which = Request_start_scan_request_tag
		request.type.start_scan_request = StartScanRequest()
		request.type.start_scan_request.timestamp = Timestamp()
		request.type.start_scan_request.timestamp.seconds = timestamp_seconds
		request.type.start_scan_request.timestamp.ms = timestamp_ms
		request.type.start_scan_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_scan_request.window = window_ms
		request.type.start_scan_request.interval = interval_ms
		request.type.start_scan_request.duration = duration_seconds
		request.type.start_scan_request.period = period_seconds
		request.type.start_scan_request.aggregation_type = aggregation_type
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.start_scan_response_queue.mutex:
			self.start_scan_response_queue.queue.clear()
			
		while(self.start_scan_response_queue.empty()):
			self.receive_response()
			
		return self.start_scan_response_queue.get()
		
		

	# Sends a request to the badge to stop scanning.
	# Returns True if request was successfuly sent.
	def stop_scan(self):
	
		request = Request()
		request.type.which = Request_stop_scan_request_tag
		request.type.stop_scan_request = StopScanRequest()
		
		self.send_request(request)
		
	
	
	def start_accelerometer(self, t=None, 
		timeout_minutes=0, operating_mode=DEFAULT_ACCELEROMETER_OPERATING_MODE, full_scale=DEFAULT_ACCELEROMETER_FULL_SCALE,
		datarate=DEFAULT_ACCELEROMETER_DATARATE, fifo_sampling_period_ms=DEFAULT_ACCELEROMETER_FIFO_SAMPLING_PERIOD_MS):
		
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
			
		request = Request()
		request.type.which = Request_start_accelerometer_request_tag
		request.type.start_accelerometer_request = StartAccelerometerRequest()
		request.type.start_accelerometer_request.timestamp = Timestamp()
		request.type.start_accelerometer_request.timestamp.seconds = timestamp_seconds
		request.type.start_accelerometer_request.timestamp.ms = timestamp_ms
		request.type.start_accelerometer_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_accelerometer_request.operating_mode = operating_mode
		request.type.start_accelerometer_request.full_scale = full_scale
		request.type.start_accelerometer_request.datarate = datarate
		request.type.start_accelerometer_request.fifo_sampling_period_ms = fifo_sampling_period_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.start_accelerometer_response_queue.mutex:
			self.start_accelerometer_response_queue.queue.clear()
			
		while(self.start_accelerometer_response_queue.empty()):
			self.receive_response()
			
		return self.start_accelerometer_response_queue.get()
		
	def stop_accelerometer(self):
	
		request = Request()
		request.type.which = Request_stop_accelerometer_request_tag
		request.type.stop_accelerometer_request = StopAccelerometerRequest()
		
		self.send_request(request)
		

		
	def start_accelerometer_interrupt(self, t=None, 
		timeout_minutes=0, threshold_mg=DEFAULT_ACCELEROMETER_INTERRUPT_THRESHOLD_MG, 
		minimal_duration_ms=DEFAULT_ACCELEROMETER_INTERRUPT_MINIMAL_DURATION_MS,
		ignore_duration_ms=DEFAULT_ACCELEROMETER_INTERRUPT_IGNORE_DURATION_MS):
		
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
			
			
		
		request = Request()
		request.type.which = Request_start_accelerometer_interrupt_request_tag
		request.type.start_accelerometer_interrupt_request = StartAccelerometerInterruptRequest()
		request.type.start_accelerometer_interrupt_request.timestamp = Timestamp()
		request.type.start_accelerometer_interrupt_request.timestamp.seconds = timestamp_seconds
		request.type.start_accelerometer_interrupt_request.timestamp.ms = timestamp_ms
		request.type.start_accelerometer_interrupt_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_accelerometer_interrupt_request.threshold_mg = threshold_mg
		request.type.start_accelerometer_interrupt_request.minimal_duration_ms = minimal_duration_ms
		request.type.start_accelerometer_interrupt_request.ignore_duration_ms = ignore_duration_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.start_accelerometer_interrupt_response_queue.mutex:
			self.start_accelerometer_interrupt_response_queue.queue.clear()
			
		while(self.start_accelerometer_interrupt_response_queue.empty()):
			self.receive_response()
			
		return self.start_accelerometer_interrupt_response_queue.get()
		
	def stop_accelerometer_interrupt(self):
	
		request = Request()
		request.type.which = Request_stop_accelerometer_interrupt_request_tag
		request.type.stop_accelerometer_interrupt_request = StopAccelerometerInterruptRequest()
		
		self.send_request(request)
		

		
	
	def start_battery(self, t=None, timeout_minutes=0, sampling_period_ms=DEFAULT_BATTERY_SAMPLING_PERIOD_MS):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
		
		
		request = Request()
		request.type.which = Request_start_battery_request_tag
		request.type.start_battery_request = StartBatteryRequest()
		request.type.start_battery_request.timestamp = Timestamp()
		request.type.start_battery_request.timestamp.seconds = timestamp_seconds
		request.type.start_battery_request.timestamp.ms = timestamp_ms
		request.type.start_battery_request.timeout = int(timeout_minutes*60*1000)
		request.type.start_battery_request.period_ms = sampling_period_ms
		
		self.send_request(request)
		
		
		with self.start_battery_response_queue.mutex:
			self.start_battery_response_queue.queue.clear()
			
		while(self.start_battery_response_queue.empty()):
			self.receive_response()
			
		return self.start_battery_response_queue.get()

	
	
	def stop_battery(self):
	
		request = Request()
		request.type.which = Request_stop_battery_request_tag
		request.type.stop_battery_request = StopBatteryRequest()
		
		self.send_request(request)
		
		return True

	
	
	
	
	# Send a request to the badge to light an led to identify its self.
	#   If duration_seconds == 0, badge will turn off LED if currently lit.
	# Returns True if request was successfuly sent. 
	def identify(self, duration_seconds=10):
	
		request = Request()
		request.type.which = Request_identify_request_tag
		request.type.identify_request = IdentifyRequest()
		request.type.identify_request.timeout = duration_seconds
		
		self.send_request(request)
		
		return True
		
	def test(self):
	
		request = Request()
		request.type.which = Request_test_request_tag
		request.type.test_request = TestRequest()

		
		self.send_request(request)
		
		
		with self.test_response_queue.mutex:
			self.test_response_queue.queue.clear()
			
		while(self.test_response_queue.empty()):
			self.receive_response()
			
		return self.test_response_queue.get()
		

	# Send a request to the badge for recorded microphone data starting at the given timestamp.
	# Returns a list of tuples of (MicrophoneDataHeader(), microphone_sample_chunk_data), where each tuple
	#   contains one chunk of microphone data. 
	def get_microphone_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
			
		request = Request()
		request.type.which = Request_microphone_data_request_tag
		request.type.microphone_data_request = MicrophoneDataRequest()
		request.type.microphone_data_request.timestamp = Timestamp()
		request.type.microphone_data_request.timestamp.seconds = timestamp_seconds
		request.type.microphone_data_request.timestamp.ms = timestamp_ms
		
		self.send_request(request)
	
		# Clear the queue before receiving
		with self.microphone_data_response_queue.mutex:
			self.microphone_data_response_queue.queue.clear()
		
		microphone_chunks = []
		
		while True:
			self.receive_response()
			if(not self.microphone_data_response_queue.empty()):
				microphone_data_response = self.microphone_data_response_queue.get()
				if(microphone_data_response.last_response):
					break;
				microphone_chunks.append(microphone_data_response)
		
	
		return microphone_chunks
		
	# Sends a request to the badge for recorded scan data starting at the the given timestamp.
	# Returns a list of tuples of (ScanDataHeader(), [ScanDataDevice(), ScanDataDevice(), ...])
	#   where each tuple contains a header and a list of devices seen from one scan. 
	def get_scan_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

		request = Request()
		request.type.which = Request_scan_data_request_tag
		request.type.scan_data_request = ScanDataRequest()
		request.type.scan_data_request.timestamp = Timestamp()
		request.type.scan_data_request.timestamp.seconds = timestamp_seconds
		request.type.scan_data_request.timestamp.ms = timestamp_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.scan_data_response_queue.mutex:
			self.scan_data_response_queue.queue.clear()
		
		scan_chunks = []
		
		while True:
			self.receive_response()
			if(not self.scan_data_response_queue.empty()):
				scan_data_response = self.scan_data_response_queue.get()
				if(scan_data_response.last_response):
					break;
				scan_chunks.append(scan_data_response)
		
	
		return scan_chunks
		
		
		
	def get_accelerometer_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

		request = Request()
		request.type.which = Request_accelerometer_data_request_tag
		request.type.accelerometer_data_request = AccelerometerDataRequest()
		request.type.accelerometer_data_request.timestamp = Timestamp()
		request.type.accelerometer_data_request.timestamp.seconds = timestamp_seconds
		request.type.accelerometer_data_request.timestamp.ms = timestamp_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.accelerometer_data_response_queue.mutex:
			self.accelerometer_data_response_queue.queue.clear()
		
		accelerometer_chunks = []
		
		while True:
			self.receive_response()
			if(not self.accelerometer_data_response_queue.empty()):
				accelerometer_data_response = self.accelerometer_data_response_queue.get()
				if(accelerometer_data_response.last_response):
					break;
				accelerometer_chunks.append(accelerometer_data_response)
		
	
		return accelerometer_chunks
		
		
		
	def get_accelerometer_interrupt_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

		request = Request()
		request.type.which = Request_accelerometer_interrupt_data_request_tag
		request.type.accelerometer_interrupt_data_request = AccelerometerInterruptDataRequest()
		request.type.accelerometer_interrupt_data_request.timestamp = Timestamp()
		request.type.accelerometer_interrupt_data_request.timestamp.seconds = timestamp_seconds
		request.type.accelerometer_interrupt_data_request.timestamp.ms = timestamp_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.accelerometer_interrupt_data_response_queue.mutex:
			self.accelerometer_interrupt_data_response_queue.queue.clear()
		
		accelerometer_interrupt_chunks = []
		
		while True:
			self.receive_response()
			if(not self.accelerometer_interrupt_data_response_queue.empty()):
				accelerometer_interrupt_data_response = self.accelerometer_interrupt_data_response_queue.get()
				if(accelerometer_interrupt_data_response.last_response):
					break;
				accelerometer_interrupt_chunks.append(accelerometer_interrupt_data_response)
		
	
		return accelerometer_interrupt_chunks
		
		
	def get_battery_data(self, t=None):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)

		request = Request()
		request.type.which = Request_battery_data_request_tag
		request.type.battery_data_request = BatteryDataRequest()
		request.type.battery_data_request.timestamp = Timestamp()
		request.type.battery_data_request.timestamp.seconds = timestamp_seconds
		request.type.battery_data_request.timestamp.ms = timestamp_ms
		
		self.send_request(request)
		
		# Clear the queue before receiving
		with self.battery_data_response_queue.mutex:
			self.battery_data_response_queue.queue.clear()
		
		battery_chunks = []
		
		while True:
			self.receive_response()
			if(not self.battery_data_response_queue.empty()):
				battery_data_response = self.battery_data_response_queue.get()
				if(battery_data_response.last_response):
					break;
				battery_chunks.append(battery_data_response)
		
	
		return battery_chunks
	
	
	
	
	
	
	
	
################# Streaming ################	
	def start_microphone_stream(self, t=None, timeout_minutes=0, sampling_period_ms=DEFAULT_MICROPHONE_STREAM_SAMPLING_PERIOD_MS):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
		
		request = Request()
		request.type.which = Request_start_microphone_stream_request_tag
		request.type.start_microphone_stream_request = StartMicrophoneStreamRequest()
		request.type.start_microphone_stream_request.timestamp = Timestamp()
		request.type.start_microphone_stream_request.timestamp.seconds = timestamp_seconds
		request.type.start_microphone_stream_request.timestamp.ms = timestamp_ms
		request.type.start_microphone_stream_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_microphone_stream_request.period_ms = sampling_period_ms
		
		self.send_request(request)
		

	def stop_microphone_stream(self):
	
		request = Request()
		request.type.which = Request_stop_microphone_stream_request_tag
		request.type.stop_microphone_stream_request = StopMicrophoneStreamRequest()
		
		self.send_request(request)
		
		

	def start_scan_stream(self, t=None,
	   timeout_minutes=0, window_ms=DEFAULT_SCAN_STREAM_WINDOW, interval_ms=DEFAULT_SCAN_STREAM_INTERVAL,
	    duration_seconds=DEFAULT_SCAN_STREAM_DURATION, period_seconds=DEFAULT_SCAN_STREAM_PERIOD, aggregation_type=DEFAULT_SCAN_STREAM_AGGREGATION_TYPE):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
		
		
		request = Request()
		request.type.which = Request_start_scan_stream_request_tag
		request.type.start_scan_stream_request = StartScanStreamRequest()
		request.type.start_scan_stream_request.timestamp = Timestamp()
		request.type.start_scan_stream_request.timestamp.seconds = timestamp_seconds
		request.type.start_scan_stream_request.timestamp.ms = timestamp_ms
		request.type.start_scan_stream_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_scan_stream_request.window = window_ms
		request.type.start_scan_stream_request.interval = interval_ms
		request.type.start_scan_stream_request.duration = duration_seconds
		request.type.start_scan_stream_request.period = period_seconds
		request.type.start_scan_stream_request.aggregation_type = aggregation_type
		
		self.send_request(request)
		
		


	def stop_scan_stream(self):
	
		request = Request()
		request.type.which = Request_stop_scan_stream_request_tag
		request.type.stop_scan_stream_request = StopScanStreamRequest()
		
		self.send_request(request)
		
	
	
	def start_accelerometer_stream(self, t=None, 
		timeout_minutes=0, operating_mode=DEFAULT_ACCELEROMETER_STREAM_OPERATING_MODE, full_scale=DEFAULT_ACCELEROMETER_STREAM_FULL_SCALE,
		datarate=DEFAULT_ACCELEROMETER_STREAM_DATARATE, fifo_sampling_period_ms=DEFAULT_ACCELEROMETER_STREAM_FIFO_SAMPLING_PERIOD_MS):
		
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
			
		
		
		request = Request()
		request.type.which = Request_start_accelerometer_stream_request_tag
		request.type.start_accelerometer_stream_request = StartAccelerometerStreamRequest()
		request.type.start_accelerometer_stream_request.timestamp = Timestamp()
		request.type.start_accelerometer_stream_request.timestamp.seconds = timestamp_seconds
		request.type.start_accelerometer_stream_request.timestamp.ms = timestamp_ms
		request.type.start_accelerometer_stream_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_accelerometer_stream_request.operating_mode = operating_mode
		request.type.start_accelerometer_stream_request.full_scale = full_scale
		request.type.start_accelerometer_stream_request.datarate = datarate
		request.type.start_accelerometer_stream_request.fifo_sampling_period_ms = fifo_sampling_period_ms
		
		self.send_request(request)
		
		
		
	def stop_accelerometer_stream(self):
	
		request = Request()
		request.type.which = Request_stop_accelerometer_stream_request_tag
		request.type.stop_accelerometer_stream_request = StopAccelerometerStreamRequest()
		
		self.send_request(request)
		

		
	def start_accelerometer_interrupt_stream(self, t=None, 
		timeout_minutes=0, threshold_mg=DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_THRESHOLD_MG, 
		minimal_duration_ms=DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_MINIMAL_DURATION_MS,
		ignore_duration_ms=DEFAULT_ACCELEROMETER_INTERRUPT_STREAM_IGNORE_DURATION_MS):
		
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
		
		
		request = Request()
		request.type.which = Request_start_accelerometer_interrupt_stream_request_tag
		request.type.start_accelerometer_interrupt_stream_request = StartAccelerometerInterruptStreamRequest()
		request.type.start_accelerometer_interrupt_stream_request.timestamp = Timestamp()
		request.type.start_accelerometer_interrupt_stream_request.timestamp.seconds = timestamp_seconds
		request.type.start_accelerometer_interrupt_stream_request.timestamp.ms = timestamp_ms
		request.type.start_accelerometer_interrupt_stream_request.timeout =  int(timeout_minutes*60*1000)
		request.type.start_accelerometer_interrupt_stream_request.threshold_mg = threshold_mg
		request.type.start_accelerometer_interrupt_stream_request.minimal_duration_ms = minimal_duration_ms
		request.type.start_accelerometer_interrupt_stream_request.ignore_duration_ms = ignore_duration_ms
		
		self.send_request(request)
		
		
		
	def stop_accelerometer_interrupt_stream(self):
	
		request = Request()
		request.type.which = Request_stop_accelerometer_interrupt_stream_request_tag
		request.type.stop_accelerometer_interrupt_stream_request = StopAccelerometerInterruptStreamRequest()
		
		self.send_request(request)
		

		
	
	def start_battery_stream(self, t=None, sampling_period_ms=DEFAULT_BATTERY_STREAM_SAMPLING_PERIOD_MS):
		if t is None:
			(timestamp_seconds, timestamp_ms) = get_timestamps()
		else:
			(timestamp_seconds, timestamp_ms) = get_timestamps_from_time(t)
			
			
			
		
		request = Request()
		request.type.which = Request_start_battery_stream_request_tag
		request.type.start_battery_stream_request = StartBatteryStreamRequest()
		request.type.start_battery_stream_request.timestamp = Timestamp()
		request.type.start_battery_stream_request.timestamp.seconds = timestamp_seconds
		request.type.start_battery_stream_request.timestamp.ms = timestamp_ms
		request.type.start_battery_stream_request.timeout = int(timeout_minutes*60*1000)
		request.type.start_battery_stream_request.period_ms = sampling_period_ms
		
		self.send_request(request)
		
		
		
	
	def stop_battery_stream(self):
	
		request = Request()
		request.type.which = Request_stop_battery_stream_request_tag
		request.type.stop_battery_stream_request = StopBatteryStreamRequest()
		
		self.send_request(request)

	
	def stream_clear(self):
		# Clear the queue
		with self.stream_response_queue.mutex:
			self.stream_response_queue.queue.clear()
	
	def get_stream(self):
		self.receive_response()
		if(not self.stream_response_queue.empty()):
			return self.stream_response_queue.get()
		else:
			return []
			
	
from __future__ import division, absolute_import, print_function
from struct import *

# This file defines much of the structure needed to carry out our communication protocol in Python.
# You can find more information about our communications protocol here:
#   https://github.com/HumanDynamics/OpenBadge/wiki/Communication-protocol

STATUS_REQUEST_HEADER = "s"
START_RECORDING_HEADER = "1"
STOP_RECORDING_HEADER = "0"
START_SCANNING_HEADER = "p"
STOP_SCANNING_HEADER = "q"
REQUEST_MIC_DATA_HEADER = "r"
REQUEST_SCAN_DATA_HEADER = "b"
IDENITFY_HEADER = "i"

# These fields are used by BadgeMessage objects to serialize and deserialize binary messages to Python objects.
# Fields are composed of (attribute, length [in bytes], optional, serializer, parser)
def char_field(attribute, optional=False):
	return (attribute, 1, optional, lambda x: chr(x) if type(x) is int else x, lambda x: chr(x) if type (x) is int else x)

def long_field(attribute, optional=False):
	return (attribute, 4, optional, lambda x: pack("<l", x), lambda x: unpack("<l", x)[0])

def short_field(attribute, optional=False):
	return (attribute, 2, optional, lambda x: pack("<h", x), lambda x: unpack("<h", x)[0])

def float_field(attribute, optional=False):
	return (attribute, 4, optional, lambda x: pack("<f", x), lambda x: unpack("<f", x)[0])

def bool_field(attribute, optional=False):
	return (attribute, 1, optional, lambda x: chr(x), lambda x: not x == chr(False))

def uint8_field(attribute, optional=False):
	return (attribute, 1, optional, lambda x: chr(x), lambda x: ord(x))

# BadgeMessage represents a message sent to/recieved from the badge.
#  The badge communicates by sending messages in a special binary format. 
#  BadgeMessage allows us to manipulate these messages as Python objects and then serialize them into 
#   the binary message strings the badge can understand using serialize_message().
#  We can also deserialize a message from the badge into an instance of a BadgeMessage object by calling 
#   deserialize_message() on that binary message string. 
#   deserialize_message() is a class method, so we must call it on a specific type of BadgeMessage so that
#     we know the structure of the message we are trying to deserialize (i.e. StatusMessage.deserialize_message(some_bytes))
#  All subclasses of BadgeMessage should include a class attribute 'message_fields'.
#    This 'message_fields' attribute defines the structure of the binary message represented by that subclass.
#    The attribute 'message_fields' is a list of 'fields' (see functions above) in the order they are to be
#      transmitted in a binary message to/from the badge according to our communication protocol.
class BadgeMessage(object):
	def __init__(self):
		pass

	# Override Python's __repr__() for maximum prettiness. 
	def __repr__(self):
		fields_dict = {}
		for attribute, length, optional, serializer, parser in self.message_fields:
			if hasattr(self, attribute):
				fields_dict[attribute] = getattr(self, attribute)

		string_representation = "[BadgeMessage (" + self.__class__.__name__ + ") " + str(fields_dict) + "]"
		return string_representation

	# Returns the length of the binary representation of this type of BadgeMessage.
	@classmethod
	def length(cls):
		return reduce(lambda x, y: x + y[1], cls.message_fields, 0)

	# Returns the binary representation of this BadgeMessage as a string according to the
	# badge communication protocol.
	def serialize_message(self):
		serialized_message = ""
		for attribute, length, optional, serializer, parser in self.message_fields:
			if hasattr(self, attribute):
				serialized_message += serializer(getattr(self, attribute))

		return serialized_message

	# Returns an instance of the given derived class 'cls' of BadgeMessage initialized from the
	# given string 'serialized_message' according to the badge communication protocol.
	@classmethod
	def deserialize_message(cls, serialized_message):
		message_attributes = {}

		pos = 0
		for attribute, length, optional, serializer, parser in cls.message_fields:
			if pos < len(serialized_message):
				raw_field = serialized_message[pos:pos + length]
				message_attributes[attribute] = parser(raw_field)
				pos += length
			else:
				if optional != True: raise ValueError("Serialized messsage is malformed. {}".format(serialized_message.encode("hex")))

		return cls(**message_attributes)

# The propoer values of the fields needed to initialize these objects are not documented here.
# They are documented in the communications protocol documentation on the GitHub Wiki instead.
#   https://github.com/HumanDynamics/OpenBadge/wiki/Communication-protocol

class StatusRequest(BadgeMessage):
	message_fields = [char_field("header"), long_field("timestamp_seconds"), short_field("timestamp_miliseconds"),
	 short_field("badge_id", optional=True), char_field("group_number", optional=True)]

	def __init__(self, timestamp_seconds, timestamp_miliseconds, header=STATUS_REQUEST_HEADER, badge_id=None, group_number=None):
		self.header = header
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds
		if badge_id: self.badge_id = badge_id
		if group_number: self.group_number = group_number

		BadgeMessage.__init__(self)

class StatusResponse(BadgeMessage):
	message_fields = [char_field("clock_status"), bool_field("scanner_status"), bool_field("collector_status"), 
	long_field("timestamp_seconds"), short_field("timestamp_miliseconds"), float_field("battery_voltage")]

	def __init__(self, clock_status, scanner_status, collector_status, timestamp_seconds, 
		timestamp_miliseconds, battery_voltage):
		self.clock_status = clock_status
		self.scanner_status = scanner_status
		self.collector_status = collector_status
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds
		self.battery_voltage = battery_voltage

		BadgeMessage.__init__(self)

class StartRecordRequest(BadgeMessage):
	message_fields = [char_field("header"), long_field("timestamp_seconds"), short_field("timestamp_miliseconds"), short_field("timeout_minutes")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds, timeout_minutes, header=START_RECORDING_HEADER):
		self.header = header
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds
		self.timeout_minutes = timeout_minutes

		BadgeMessage.__init__(self)

class StartRecordResponse(BadgeMessage):
	message_fields = [long_field("timestamp_seconds"), short_field("timestamp_miliseconds")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds):
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds

		BadgeMessage.__init__(self)

class StopRecordRequest(BadgeMessage):
	message_fields = [char_field("header")]

	def __init__(self, header=STOP_RECORDING_HEADER):
		self.header = header

		BadgeMessage.__init__(self)

class StartScanningRequest(BadgeMessage):
	message_fields = [char_field("header"), long_field("timestamp_seconds"), short_field("timestamp_miliseconds"),
	 short_field("timeout_minutes"), short_field("window_miliseconds"), short_field("interval_miliseconds"),
	  short_field("duration_seconds"), short_field("period_seconds")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds, timeout_minutes, window_miliseconds, interval_miliseconds, 
		duration_seconds, period_seconds, header=START_SCANNING_HEADER):
		self.header = header
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds
		self.timeout_minutes = timeout_minutes
		self.window_miliseconds = window_miliseconds
		self.interval_miliseconds = interval_miliseconds
		self.duration_seconds = duration_seconds
		self.period_seconds = period_seconds

		BadgeMessage.__init__(self)

class StartScanningResponse(BadgeMessage):
	message_fields = [long_field("timestamp_seconds"), short_field("timestamp_miliseconds")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds):
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds

		BadgeMessage.__init__(self)

class StopScanningRequest(BadgeMessage):
	message_fields = [char_field("header")]

	def __init__(self, header=STOP_SCANNING_HEADER):
		self.header = header

		BadgeMessage.__init__(self)

class IdentifyRequest(BadgeMessage):
	message_fields = [char_field("header"), short_field("duration_seconds")]

	def __init__(self, duration_seconds, header=IDENITFY_HEADER):
		self.header = header
		self.duration_seconds = duration_seconds

		BadgeMessage.__init__(self)

class MicrophoneDataRequest(BadgeMessage):
	message_fields = [char_field("header"), long_field("timestamp_seconds"), short_field("timestamp_miliseconds")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds, header=REQUEST_MIC_DATA_HEADER):
		self.header = header
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds

		BadgeMessage.__init__(self)

class MicrophoneDataHeader(BadgeMessage):
	message_fields = [long_field("timestamp_seconds"), short_field("timestamp_miliseconds"), 
	   float_field("battery_voltage"), short_field("sample_period_miliseconds"), 
	   uint8_field("num_samples_in_chunk")]

	def __init__(self, timestamp_seconds, timestamp_miliseconds, battery_voltage, 
		sample_period_miliseconds, num_samples_in_chunk):
		self.timestamp_seconds = timestamp_seconds
		self.timestamp_miliseconds = timestamp_miliseconds
		self.battery_voltage = battery_voltage
		self.sample_period_miliseconds = sample_period_miliseconds
		self.num_samples_in_chunk = num_samples_in_chunk

		BadgeMessage.__init__(self)

class ScanDataRequest(BadgeMessage):
	message_fields = [char_field("header"), long_field("timestamp_seconds")]

	def __init__(self, timestamp_seconds, header=REQUEST_SCAN_DATA_HEADER):
		self.header = header
		self.timestamp_seconds = timestamp_seconds

		BadgeMessage.__init__(self)

class ScanDataHeader(BadgeMessage):
	message_fields = [long_field("timestamp_seconds"), float_field("battery_voltage"), uint8_field("num_devices_seen")]

	def __init__(self, timestamp_seconds, battery_voltage, num_devices_seen):
		self.timestamp_seconds = timestamp_seconds
		self.battery_voltage = battery_voltage
		self.num_devices_seen = num_devices_seen

		BadgeMessage.__init__(self)

class ScanDataDevice(BadgeMessage):
	message_fields = [short_field("device_id"), char_field("average_rssi"), uint8_field("num_times_seen")]

	def __init__(self, device_id, average_rssi, num_times_seen):
		self.device_id = device_id
		self.average_rssi = average_rssi
		self.num_times_seen = num_times_seen

		BadgeMessage.__init__(self)
import struct



STREAM_BATTERY_FIFO_SIZE = 10
STREAM_MICROPHONE_FIFO_SIZE = 100
STREAM_SCAN_FIFO_SIZE = 100
STREAM_ACCELEROMETER_FIFO_SIZE = 100
STREAM_ACCELEROMETER_INTERRUPT_FIFO_SIZE = 10

PROTOCOL_MICROPHONE_DATA_SIZE = 114
PROTOCOL_SCAN_DATA_SIZE = 29
PROTOCOL_ACCELEROMETER_DATA_SIZE = 100
PROTOCOL_MICROPHONE_STREAM_SIZE = 10
PROTOCOL_SCAN_STREAM_SIZE = 10
PROTOCOL_ACCELEROMETER_STREAM_SIZE = 10
PROTOCOL_ACCELEROMETER_INTERRUPT_STREAM_SIZE = 10
PROTOCOL_BATTERY_STREAM_SIZE = 10

Request_status_request_tag = 1
Request_start_microphone_request_tag = 2
Request_stop_microphone_request_tag = 3
Request_start_scan_request_tag = 4
Request_stop_scan_request_tag = 5
Request_start_accelerometer_request_tag = 6
Request_stop_accelerometer_request_tag = 7
Request_start_accelerometer_interrupt_request_tag = 8
Request_stop_accelerometer_interrupt_request_tag = 9
Request_start_battery_request_tag = 10
Request_stop_battery_request_tag = 11
Request_microphone_data_request_tag = 12
Request_scan_data_request_tag = 13
Request_accelerometer_data_request_tag = 14
Request_accelerometer_interrupt_data_request_tag = 15
Request_battery_data_request_tag = 16
Request_start_microphone_stream_request_tag = 17
Request_stop_microphone_stream_request_tag = 18
Request_start_scan_stream_request_tag = 19
Request_stop_scan_stream_request_tag = 20
Request_start_accelerometer_stream_request_tag = 21
Request_stop_accelerometer_stream_request_tag = 22
Request_start_accelerometer_interrupt_stream_request_tag = 23
Request_stop_accelerometer_interrupt_stream_request_tag = 24
Request_start_battery_stream_request_tag = 25
Request_stop_battery_stream_request_tag = 26
Request_identify_request_tag = 27
Request_test_request_tag = 28
Response_status_response_tag = 1
Response_start_microphone_response_tag = 2
Response_start_scan_response_tag = 3
Response_start_accelerometer_response_tag = 4
Response_start_accelerometer_interrupt_response_tag = 5
Response_start_battery_response_tag = 6
Response_microphone_data_response_tag = 7
Response_scan_data_response_tag = 8
Response_accelerometer_data_response_tag = 9
Response_accelerometer_interrupt_data_response_tag = 10
Response_battery_data_response_tag = 11
Response_stream_response_tag = 12
Response_test_response_tag = 13




class _Ostream:
	def __init__(self):
		self.buf = b''
	def write(self, data):
		self.buf += data

class _Istream:
	def __init__(self, buf):
		self.buf = buf
	def read(self, l):
		if(l > len(self.buf)):
			raise Exception("Not enough bytes in Istream to read")
		ret = self.buf[0:l]
		self.buf = self.buf[l:]
		return ret


class Timestamp:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.seconds = 0
		self.ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_seconds(ostream)
		self.encode_ms(ostream)
		pass

	def encode_seconds(self, ostream):
		ostream.write(struct.pack('>I', self.seconds))

	def encode_ms(self, ostream):
		ostream.write(struct.pack('>H', self.ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_seconds(istream)
		self.decode_ms(istream)
		pass

	def decode_seconds(self, istream):
		self.seconds= struct.unpack('>I', istream.read(4))[0]

	def decode_ms(self, istream):
		self.ms= struct.unpack('>H', istream.read(2))[0]


class BadgeAssignement:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.ID = 0
		self.group = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_ID(ostream)
		self.encode_group(ostream)
		pass

	def encode_ID(self, ostream):
		ostream.write(struct.pack('>H', self.ID))

	def encode_group(self, ostream):
		ostream.write(struct.pack('>B', self.group))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_ID(istream)
		self.decode_group(istream)
		pass

	def decode_ID(self, istream):
		self.ID= struct.unpack('>H', istream.read(2))[0]

	def decode_group(self, istream):
		self.group= struct.unpack('>B', istream.read(1))[0]


class BatteryData:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.voltage = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_voltage(ostream)
		pass

	def encode_voltage(self, ostream):
		ostream.write(struct.pack('>f', self.voltage))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_voltage(istream)
		pass

	def decode_voltage(self, istream):
		self.voltage= struct.unpack('>f', istream.read(4))[0]


class MicrophoneData:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.value = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_value(ostream)
		pass

	def encode_value(self, ostream):
		ostream.write(struct.pack('>B', self.value))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_value(istream)
		pass

	def decode_value(self, istream):
		self.value= struct.unpack('>B', istream.read(1))[0]


class ScanDevice:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.ID = 0
		self.rssi = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_ID(ostream)
		self.encode_rssi(ostream)
		pass

	def encode_ID(self, ostream):
		ostream.write(struct.pack('>H', self.ID))

	def encode_rssi(self, ostream):
		ostream.write(struct.pack('>b', self.rssi))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_ID(istream)
		self.decode_rssi(istream)
		pass

	def decode_ID(self, istream):
		self.ID= struct.unpack('>H', istream.read(2))[0]

	def decode_rssi(self, istream):
		self.rssi= struct.unpack('>b', istream.read(1))[0]


class ScanResultData:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.scan_device = None
		self.count = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_scan_device(ostream)
		self.encode_count(ostream)
		pass

	def encode_scan_device(self, ostream):
		self.scan_device.encode_internal(ostream)

	def encode_count(self, ostream):
		ostream.write(struct.pack('>B', self.count))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_scan_device(istream)
		self.decode_count(istream)
		pass

	def decode_scan_device(self, istream):
		self.scan_device = ScanDevice()
		self.scan_device.decode_internal(istream)

	def decode_count(self, istream):
		self.count= struct.unpack('>B', istream.read(1))[0]


class AccelerometerData:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.acceleration = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_acceleration(ostream)
		pass

	def encode_acceleration(self, ostream):
		ostream.write(struct.pack('>H', self.acceleration))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_acceleration(istream)
		pass

	def decode_acceleration(self, istream):
		self.acceleration= struct.unpack('>H', istream.read(2))[0]


class AccelerometerRawData:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.raw_acceleration = []
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_raw_acceleration(ostream)
		pass

	def encode_raw_acceleration(self, ostream):
		count = 3
		for i in range(0, count):
			ostream.write(struct.pack('>h', self.raw_acceleration[i]))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_raw_acceleration(istream)
		pass

	def decode_raw_acceleration(self, istream):
		count = 3
		for i in range(0, count):
			self.raw_acceleration.append(struct.unpack('>h', istream.read(2))[0])

			
			
			
class BatteryStream:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.battery_data = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_battery_data(ostream)
		pass

	def encode_battery_data(self, ostream):
		self.battery_data.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_battery_data(istream)
		pass

	def decode_battery_data(self, istream):
		self.battery_data = BatteryData()
		self.battery_data.decode_internal(istream)


class MicrophoneStream:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.microphone_data = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_microphone_data(ostream)
		pass

	def encode_microphone_data(self, ostream):
		self.microphone_data.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_microphone_data(istream)
		pass

	def decode_microphone_data(self, istream):
		self.microphone_data = MicrophoneData()
		self.microphone_data.decode_internal(istream)


class ScanStream:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.scan_device = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_scan_device(ostream)
		pass

	def encode_scan_device(self, ostream):
		self.scan_device.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_scan_device(istream)
		pass

	def decode_scan_device(self, istream):
		self.scan_device = ScanDevice()
		self.scan_device.decode_internal(istream)


class AccelerometerStream:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.accelerometer_raw_data = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_accelerometer_raw_data(ostream)
		pass

	def encode_accelerometer_raw_data(self, ostream):
		self.accelerometer_raw_data.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_accelerometer_raw_data(istream)
		pass

	def decode_accelerometer_raw_data(self, istream):
		self.accelerometer_raw_data = AccelerometerRawData()
		self.accelerometer_raw_data.decode_internal(istream)


class AccelerometerInterruptStream:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)









		
		
class StatusRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.has_badge_assignement = 0
		self.badge_assignement = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_badge_assignement(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_badge_assignement(self, ostream):
		ostream.write(struct.pack('>B', self.has_badge_assignement))
		if self.has_badge_assignement:
			self.badge_assignement.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_badge_assignement(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_badge_assignement(self, istream):
		self.has_badge_assignement= struct.unpack('>B', istream.read(1))[0]
		if self.has_badge_assignement:
			self.badge_assignement = BadgeAssignement()
			self.badge_assignement.decode_internal(istream)


class StartMicrophoneRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_period_ms(self, ostream):
		ostream.write(struct.pack('>H', self.period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_period_ms(self, istream):
		self.period_ms= struct.unpack('>H', istream.read(2))[0]


class StopMicrophoneRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartScanRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.window = 0
		self.interval = 0
		self.duration = 0
		self.period = 0
		self.aggregation_type = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_window(ostream)
		self.encode_interval(ostream)
		self.encode_duration(ostream)
		self.encode_period(ostream)
		self.encode_aggregation_type(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_window(self, ostream):
		ostream.write(struct.pack('>H', self.window))

	def encode_interval(self, ostream):
		ostream.write(struct.pack('>H', self.interval))

	def encode_duration(self, ostream):
		ostream.write(struct.pack('>H', self.duration))

	def encode_period(self, ostream):
		ostream.write(struct.pack('>H', self.period))

	def encode_aggregation_type(self, ostream):
		ostream.write(struct.pack('>B', self.aggregation_type))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_window(istream)
		self.decode_interval(istream)
		self.decode_duration(istream)
		self.decode_period(istream)
		self.decode_aggregation_type(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_window(self, istream):
		self.window= struct.unpack('>H', istream.read(2))[0]

	def decode_interval(self, istream):
		self.interval= struct.unpack('>H', istream.read(2))[0]

	def decode_duration(self, istream):
		self.duration= struct.unpack('>H', istream.read(2))[0]

	def decode_period(self, istream):
		self.period= struct.unpack('>H', istream.read(2))[0]

	def decode_aggregation_type(self, istream):
		self.aggregation_type= struct.unpack('>B', istream.read(1))[0]


class StopScanRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartAccelerometerRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.operating_mode = 0
		self.full_scale = 0
		self.datarate = 0
		self.fifo_sampling_period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_operating_mode(ostream)
		self.encode_full_scale(ostream)
		self.encode_datarate(ostream)
		self.encode_fifo_sampling_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_operating_mode(self, ostream):
		ostream.write(struct.pack('>B', self.operating_mode))

	def encode_full_scale(self, ostream):
		ostream.write(struct.pack('>B', self.full_scale))

	def encode_datarate(self, ostream):
		ostream.write(struct.pack('>H', self.datarate))

	def encode_fifo_sampling_period_ms(self, ostream):
		ostream.write(struct.pack('>H', self.fifo_sampling_period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_operating_mode(istream)
		self.decode_full_scale(istream)
		self.decode_datarate(istream)
		self.decode_fifo_sampling_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_operating_mode(self, istream):
		self.operating_mode= struct.unpack('>B', istream.read(1))[0]

	def decode_full_scale(self, istream):
		self.full_scale= struct.unpack('>B', istream.read(1))[0]

	def decode_datarate(self, istream):
		self.datarate= struct.unpack('>H', istream.read(2))[0]

	def decode_fifo_sampling_period_ms(self, istream):
		self.fifo_sampling_period_ms= struct.unpack('>H', istream.read(2))[0]


class StopAccelerometerRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartAccelerometerInterruptRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.threshold_mg = 0
		self.minimal_duration_ms = 0
		self.ignore_duration_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_threshold_mg(ostream)
		self.encode_minimal_duration_ms(ostream)
		self.encode_ignore_duration_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_threshold_mg(self, ostream):
		ostream.write(struct.pack('>H', self.threshold_mg))

	def encode_minimal_duration_ms(self, ostream):
		ostream.write(struct.pack('>H', self.minimal_duration_ms))

	def encode_ignore_duration_ms(self, ostream):
		ostream.write(struct.pack('>I', self.ignore_duration_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_threshold_mg(istream)
		self.decode_minimal_duration_ms(istream)
		self.decode_ignore_duration_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_threshold_mg(self, istream):
		self.threshold_mg= struct.unpack('>H', istream.read(2))[0]

	def decode_minimal_duration_ms(self, istream):
		self.minimal_duration_ms= struct.unpack('>H', istream.read(2))[0]

	def decode_ignore_duration_ms(self, istream):
		self.ignore_duration_ms= struct.unpack('>I', istream.read(4))[0]


class StopAccelerometerInterruptRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartBatteryRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_period_ms(self, ostream):
		ostream.write(struct.pack('>I', self.period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_period_ms(self, istream):
		self.period_ms= struct.unpack('>I', istream.read(4))[0]


class StopBatteryRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class MicrophoneDataRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class ScanDataRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class AccelerometerDataRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class AccelerometerInterruptDataRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class BatteryDataRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartMicrophoneStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_period_ms(self, ostream):
		ostream.write(struct.pack('>H', self.period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_period_ms(self, istream):
		self.period_ms= struct.unpack('>H', istream.read(2))[0]


class StopMicrophoneStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartScanStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.window = 0
		self.interval = 0
		self.duration = 0
		self.period = 0
		self.aggregation_type = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_window(ostream)
		self.encode_interval(ostream)
		self.encode_duration(ostream)
		self.encode_period(ostream)
		self.encode_aggregation_type(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_window(self, ostream):
		ostream.write(struct.pack('>H', self.window))

	def encode_interval(self, ostream):
		ostream.write(struct.pack('>H', self.interval))

	def encode_duration(self, ostream):
		ostream.write(struct.pack('>H', self.duration))

	def encode_period(self, ostream):
		ostream.write(struct.pack('>H', self.period))

	def encode_aggregation_type(self, ostream):
		ostream.write(struct.pack('>B', self.aggregation_type))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_window(istream)
		self.decode_interval(istream)
		self.decode_duration(istream)
		self.decode_period(istream)
		self.decode_aggregation_type(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_window(self, istream):
		self.window= struct.unpack('>H', istream.read(2))[0]

	def decode_interval(self, istream):
		self.interval= struct.unpack('>H', istream.read(2))[0]

	def decode_duration(self, istream):
		self.duration= struct.unpack('>H', istream.read(2))[0]

	def decode_period(self, istream):
		self.period= struct.unpack('>H', istream.read(2))[0]

	def decode_aggregation_type(self, istream):
		self.aggregation_type= struct.unpack('>B', istream.read(1))[0]


class StopScanStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartAccelerometerStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.operating_mode = 0
		self.full_scale = 0
		self.datarate = 0
		self.fifo_sampling_period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_operating_mode(ostream)
		self.encode_full_scale(ostream)
		self.encode_datarate(ostream)
		self.encode_fifo_sampling_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_operating_mode(self, ostream):
		ostream.write(struct.pack('>B', self.operating_mode))

	def encode_full_scale(self, ostream):
		ostream.write(struct.pack('>B', self.full_scale))

	def encode_datarate(self, ostream):
		ostream.write(struct.pack('>H', self.datarate))

	def encode_fifo_sampling_period_ms(self, ostream):
		ostream.write(struct.pack('>H', self.fifo_sampling_period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_operating_mode(istream)
		self.decode_full_scale(istream)
		self.decode_datarate(istream)
		self.decode_fifo_sampling_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_operating_mode(self, istream):
		self.operating_mode= struct.unpack('>B', istream.read(1))[0]

	def decode_full_scale(self, istream):
		self.full_scale= struct.unpack('>B', istream.read(1))[0]

	def decode_datarate(self, istream):
		self.datarate= struct.unpack('>H', istream.read(2))[0]

	def decode_fifo_sampling_period_ms(self, istream):
		self.fifo_sampling_period_ms= struct.unpack('>H', istream.read(2))[0]


class StopAccelerometerStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartAccelerometerInterruptStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.threshold_mg = 0
		self.minimal_duration_ms = 0
		self.ignore_duration_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_threshold_mg(ostream)
		self.encode_minimal_duration_ms(ostream)
		self.encode_ignore_duration_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_threshold_mg(self, ostream):
		ostream.write(struct.pack('>H', self.threshold_mg))

	def encode_minimal_duration_ms(self, ostream):
		ostream.write(struct.pack('>H', self.minimal_duration_ms))

	def encode_ignore_duration_ms(self, ostream):
		ostream.write(struct.pack('>I', self.ignore_duration_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_threshold_mg(istream)
		self.decode_minimal_duration_ms(istream)
		self.decode_ignore_duration_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_threshold_mg(self, istream):
		self.threshold_mg= struct.unpack('>H', istream.read(2))[0]

	def decode_minimal_duration_ms(self, istream):
		self.minimal_duration_ms= struct.unpack('>H', istream.read(2))[0]

	def decode_ignore_duration_ms(self, istream):
		self.ignore_duration_ms= struct.unpack('>I', istream.read(4))[0]


class StopAccelerometerInterruptStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class StartBatteryStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.timeout = 0
		self.period_ms = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_timeout(ostream)
		self.encode_period_ms(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))

	def encode_period_ms(self, ostream):
		ostream.write(struct.pack('>I', self.period_ms))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_timeout(istream)
		self.decode_period_ms(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]

	def decode_period_ms(self, istream):
		self.period_ms= struct.unpack('>I', istream.read(4))[0]


class StopBatteryStreamRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class IdentifyRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timeout = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timeout(ostream)
		pass

	def encode_timeout(self, ostream):
		ostream.write(struct.pack('>H', self.timeout))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timeout(istream)
		pass

	def decode_timeout(self, istream):
		self.timeout= struct.unpack('>H', istream.read(2))[0]


class TestRequest:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		pass


class Request:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.type = self._type()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.type.encode_internal(ostream)
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.type.decode_internal(istream)
		pass

	class _type:

		def __init__(self):
			self.reset()

		def __repr__(self):
			return str(self.__dict__)

		def reset(self):
			self.which = 0
			self.status_request = None
			self.start_microphone_request = None
			self.stop_microphone_request = None
			self.start_scan_request = None
			self.stop_scan_request = None
			self.start_accelerometer_request = None
			self.stop_accelerometer_request = None
			self.start_accelerometer_interrupt_request = None
			self.stop_accelerometer_interrupt_request = None
			self.start_battery_request = None
			self.stop_battery_request = None
			self.microphone_data_request = None
			self.scan_data_request = None
			self.accelerometer_data_request = None
			self.accelerometer_interrupt_data_request = None
			self.battery_data_request = None
			self.start_microphone_stream_request = None
			self.stop_microphone_stream_request = None
			self.start_scan_stream_request = None
			self.stop_scan_stream_request = None
			self.start_accelerometer_stream_request = None
			self.stop_accelerometer_stream_request = None
			self.start_accelerometer_interrupt_stream_request = None
			self.stop_accelerometer_interrupt_stream_request = None
			self.start_battery_stream_request = None
			self.stop_battery_stream_request = None
			self.identify_request = None
			self.test_request = None
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('>B', self.which))
			options = {
				1: self.encode_status_request,
				2: self.encode_start_microphone_request,
				3: self.encode_stop_microphone_request,
				4: self.encode_start_scan_request,
				5: self.encode_stop_scan_request,
				6: self.encode_start_accelerometer_request,
				7: self.encode_stop_accelerometer_request,
				8: self.encode_start_accelerometer_interrupt_request,
				9: self.encode_stop_accelerometer_interrupt_request,
				10: self.encode_start_battery_request,
				11: self.encode_stop_battery_request,
				12: self.encode_microphone_data_request,
				13: self.encode_scan_data_request,
				14: self.encode_accelerometer_data_request,
				15: self.encode_accelerometer_interrupt_data_request,
				16: self.encode_battery_data_request,
				17: self.encode_start_microphone_stream_request,
				18: self.encode_stop_microphone_stream_request,
				19: self.encode_start_scan_stream_request,
				20: self.encode_stop_scan_stream_request,
				21: self.encode_start_accelerometer_stream_request,
				22: self.encode_stop_accelerometer_stream_request,
				23: self.encode_start_accelerometer_interrupt_stream_request,
				24: self.encode_stop_accelerometer_interrupt_stream_request,
				25: self.encode_start_battery_stream_request,
				26: self.encode_stop_battery_stream_request,
				27: self.encode_identify_request,
				28: self.encode_test_request,
			}
			options[self.which](ostream)
			pass

		def encode_status_request(self, ostream):
			self.status_request.encode_internal(ostream)

		def encode_start_microphone_request(self, ostream):
			self.start_microphone_request.encode_internal(ostream)

		def encode_stop_microphone_request(self, ostream):
			self.stop_microphone_request.encode_internal(ostream)

		def encode_start_scan_request(self, ostream):
			self.start_scan_request.encode_internal(ostream)

		def encode_stop_scan_request(self, ostream):
			self.stop_scan_request.encode_internal(ostream)

		def encode_start_accelerometer_request(self, ostream):
			self.start_accelerometer_request.encode_internal(ostream)

		def encode_stop_accelerometer_request(self, ostream):
			self.stop_accelerometer_request.encode_internal(ostream)

		def encode_start_accelerometer_interrupt_request(self, ostream):
			self.start_accelerometer_interrupt_request.encode_internal(ostream)

		def encode_stop_accelerometer_interrupt_request(self, ostream):
			self.stop_accelerometer_interrupt_request.encode_internal(ostream)

		def encode_start_battery_request(self, ostream):
			self.start_battery_request.encode_internal(ostream)

		def encode_stop_battery_request(self, ostream):
			self.stop_battery_request.encode_internal(ostream)

		def encode_microphone_data_request(self, ostream):
			self.microphone_data_request.encode_internal(ostream)

		def encode_scan_data_request(self, ostream):
			self.scan_data_request.encode_internal(ostream)

		def encode_accelerometer_data_request(self, ostream):
			self.accelerometer_data_request.encode_internal(ostream)

		def encode_accelerometer_interrupt_data_request(self, ostream):
			self.accelerometer_interrupt_data_request.encode_internal(ostream)

		def encode_battery_data_request(self, ostream):
			self.battery_data_request.encode_internal(ostream)

		def encode_start_microphone_stream_request(self, ostream):
			self.start_microphone_stream_request.encode_internal(ostream)

		def encode_stop_microphone_stream_request(self, ostream):
			self.stop_microphone_stream_request.encode_internal(ostream)

		def encode_start_scan_stream_request(self, ostream):
			self.start_scan_stream_request.encode_internal(ostream)

		def encode_stop_scan_stream_request(self, ostream):
			self.stop_scan_stream_request.encode_internal(ostream)

		def encode_start_accelerometer_stream_request(self, ostream):
			self.start_accelerometer_stream_request.encode_internal(ostream)

		def encode_stop_accelerometer_stream_request(self, ostream):
			self.stop_accelerometer_stream_request.encode_internal(ostream)

		def encode_start_accelerometer_interrupt_stream_request(self, ostream):
			self.start_accelerometer_interrupt_stream_request.encode_internal(ostream)

		def encode_stop_accelerometer_interrupt_stream_request(self, ostream):
			self.stop_accelerometer_interrupt_stream_request.encode_internal(ostream)

		def encode_start_battery_stream_request(self, ostream):
			self.start_battery_stream_request.encode_internal(ostream)

		def encode_stop_battery_stream_request(self, ostream):
			self.stop_battery_stream_request.encode_internal(ostream)

		def encode_identify_request(self, ostream):
			self.identify_request.encode_internal(ostream)

		def encode_test_request(self, ostream):
			self.test_request.encode_internal(ostream)


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('>B', istream.read(1))[0]
			options = {
				1: self.decode_status_request,
				2: self.decode_start_microphone_request,
				3: self.decode_stop_microphone_request,
				4: self.decode_start_scan_request,
				5: self.decode_stop_scan_request,
				6: self.decode_start_accelerometer_request,
				7: self.decode_stop_accelerometer_request,
				8: self.decode_start_accelerometer_interrupt_request,
				9: self.decode_stop_accelerometer_interrupt_request,
				10: self.decode_start_battery_request,
				11: self.decode_stop_battery_request,
				12: self.decode_microphone_data_request,
				13: self.decode_scan_data_request,
				14: self.decode_accelerometer_data_request,
				15: self.decode_accelerometer_interrupt_data_request,
				16: self.decode_battery_data_request,
				17: self.decode_start_microphone_stream_request,
				18: self.decode_stop_microphone_stream_request,
				19: self.decode_start_scan_stream_request,
				20: self.decode_stop_scan_stream_request,
				21: self.decode_start_accelerometer_stream_request,
				22: self.decode_stop_accelerometer_stream_request,
				23: self.decode_start_accelerometer_interrupt_stream_request,
				24: self.decode_stop_accelerometer_interrupt_stream_request,
				25: self.decode_start_battery_stream_request,
				26: self.decode_stop_battery_stream_request,
				27: self.decode_identify_request,
				28: self.decode_test_request,
			}
			options[self.which](istream)
			pass

		def decode_status_request(self, istream):
			self.status_request = StatusRequest()
			self.status_request.decode_internal(istream)

		def decode_start_microphone_request(self, istream):
			self.start_microphone_request = StartMicrophoneRequest()
			self.start_microphone_request.decode_internal(istream)

		def decode_stop_microphone_request(self, istream):
			self.stop_microphone_request = StopMicrophoneRequest()
			self.stop_microphone_request.decode_internal(istream)

		def decode_start_scan_request(self, istream):
			self.start_scan_request = StartScanRequest()
			self.start_scan_request.decode_internal(istream)

		def decode_stop_scan_request(self, istream):
			self.stop_scan_request = StopScanRequest()
			self.stop_scan_request.decode_internal(istream)

		def decode_start_accelerometer_request(self, istream):
			self.start_accelerometer_request = StartAccelerometerRequest()
			self.start_accelerometer_request.decode_internal(istream)

		def decode_stop_accelerometer_request(self, istream):
			self.stop_accelerometer_request = StopAccelerometerRequest()
			self.stop_accelerometer_request.decode_internal(istream)

		def decode_start_accelerometer_interrupt_request(self, istream):
			self.start_accelerometer_interrupt_request = StartAccelerometerInterruptRequest()
			self.start_accelerometer_interrupt_request.decode_internal(istream)

		def decode_stop_accelerometer_interrupt_request(self, istream):
			self.stop_accelerometer_interrupt_request = StopAccelerometerInterruptRequest()
			self.stop_accelerometer_interrupt_request.decode_internal(istream)

		def decode_start_battery_request(self, istream):
			self.start_battery_request = StartBatteryRequest()
			self.start_battery_request.decode_internal(istream)

		def decode_stop_battery_request(self, istream):
			self.stop_battery_request = StopBatteryRequest()
			self.stop_battery_request.decode_internal(istream)

		def decode_microphone_data_request(self, istream):
			self.microphone_data_request = MicrophoneDataRequest()
			self.microphone_data_request.decode_internal(istream)

		def decode_scan_data_request(self, istream):
			self.scan_data_request = ScanDataRequest()
			self.scan_data_request.decode_internal(istream)

		def decode_accelerometer_data_request(self, istream):
			self.accelerometer_data_request = AccelerometerDataRequest()
			self.accelerometer_data_request.decode_internal(istream)

		def decode_accelerometer_interrupt_data_request(self, istream):
			self.accelerometer_interrupt_data_request = AccelerometerInterruptDataRequest()
			self.accelerometer_interrupt_data_request.decode_internal(istream)

		def decode_battery_data_request(self, istream):
			self.battery_data_request = BatteryDataRequest()
			self.battery_data_request.decode_internal(istream)

		def decode_start_microphone_stream_request(self, istream):
			self.start_microphone_stream_request = StartMicrophoneStreamRequest()
			self.start_microphone_stream_request.decode_internal(istream)

		def decode_stop_microphone_stream_request(self, istream):
			self.stop_microphone_stream_request = StopMicrophoneStreamRequest()
			self.stop_microphone_stream_request.decode_internal(istream)

		def decode_start_scan_stream_request(self, istream):
			self.start_scan_stream_request = StartScanStreamRequest()
			self.start_scan_stream_request.decode_internal(istream)

		def decode_stop_scan_stream_request(self, istream):
			self.stop_scan_stream_request = StopScanStreamRequest()
			self.stop_scan_stream_request.decode_internal(istream)

		def decode_start_accelerometer_stream_request(self, istream):
			self.start_accelerometer_stream_request = StartAccelerometerStreamRequest()
			self.start_accelerometer_stream_request.decode_internal(istream)

		def decode_stop_accelerometer_stream_request(self, istream):
			self.stop_accelerometer_stream_request = StopAccelerometerStreamRequest()
			self.stop_accelerometer_stream_request.decode_internal(istream)

		def decode_start_accelerometer_interrupt_stream_request(self, istream):
			self.start_accelerometer_interrupt_stream_request = StartAccelerometerInterruptStreamRequest()
			self.start_accelerometer_interrupt_stream_request.decode_internal(istream)

		def decode_stop_accelerometer_interrupt_stream_request(self, istream):
			self.stop_accelerometer_interrupt_stream_request = StopAccelerometerInterruptStreamRequest()
			self.stop_accelerometer_interrupt_stream_request.decode_internal(istream)

		def decode_start_battery_stream_request(self, istream):
			self.start_battery_stream_request = StartBatteryStreamRequest()
			self.start_battery_stream_request.decode_internal(istream)

		def decode_stop_battery_stream_request(self, istream):
			self.stop_battery_stream_request = StopBatteryStreamRequest()
			self.stop_battery_stream_request.decode_internal(istream)

		def decode_identify_request(self, istream):
			self.identify_request = IdentifyRequest()
			self.identify_request.decode_internal(istream)

		def decode_test_request(self, istream):
			self.test_request = TestRequest()
			self.test_request.decode_internal(istream)


class StatusResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.clock_status = 0
		self.microphone_status = 0
		self.scan_status = 0
		self.accelerometer_status = 0
		self.accelerometer_interrupt_status = 0
		self.battery_status = 0
		self.timestamp = None
		self.battery_data = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_clock_status(ostream)
		self.encode_microphone_status(ostream)
		self.encode_scan_status(ostream)
		self.encode_accelerometer_status(ostream)
		self.encode_accelerometer_interrupt_status(ostream)
		self.encode_battery_status(ostream)
		self.encode_timestamp(ostream)
		self.encode_battery_data(ostream)
		pass

	def encode_clock_status(self, ostream):
		ostream.write(struct.pack('>B', self.clock_status))

	def encode_microphone_status(self, ostream):
		ostream.write(struct.pack('>B', self.microphone_status))

	def encode_scan_status(self, ostream):
		ostream.write(struct.pack('>B', self.scan_status))

	def encode_accelerometer_status(self, ostream):
		ostream.write(struct.pack('>B', self.accelerometer_status))

	def encode_accelerometer_interrupt_status(self, ostream):
		ostream.write(struct.pack('>B', self.accelerometer_interrupt_status))

	def encode_battery_status(self, ostream):
		ostream.write(struct.pack('>B', self.battery_status))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_battery_data(self, ostream):
		self.battery_data.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_clock_status(istream)
		self.decode_microphone_status(istream)
		self.decode_scan_status(istream)
		self.decode_accelerometer_status(istream)
		self.decode_accelerometer_interrupt_status(istream)
		self.decode_battery_status(istream)
		self.decode_timestamp(istream)
		self.decode_battery_data(istream)
		pass

	def decode_clock_status(self, istream):
		self.clock_status= struct.unpack('>B', istream.read(1))[0]

	def decode_microphone_status(self, istream):
		self.microphone_status= struct.unpack('>B', istream.read(1))[0]

	def decode_scan_status(self, istream):
		self.scan_status= struct.unpack('>B', istream.read(1))[0]

	def decode_accelerometer_status(self, istream):
		self.accelerometer_status= struct.unpack('>B', istream.read(1))[0]

	def decode_accelerometer_interrupt_status(self, istream):
		self.accelerometer_interrupt_status= struct.unpack('>B', istream.read(1))[0]

	def decode_battery_status(self, istream):
		self.battery_status= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_battery_data(self, istream):
		self.battery_data = BatteryData()
		self.battery_data.decode_internal(istream)


class StartMicrophoneResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartScanResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartAccelerometerResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartAccelerometerInterruptResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class StartBatteryResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class MicrophoneDataResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.last_response = 0
		self.timestamp = None
		self.sample_period_ms = 0
		self.microphone_data = []
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_last_response(ostream)
		self.encode_timestamp(ostream)
		self.encode_sample_period_ms(ostream)
		self.encode_microphone_data(ostream)
		pass

	def encode_last_response(self, ostream):
		ostream.write(struct.pack('>B', self.last_response))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_sample_period_ms(self, ostream):
		ostream.write(struct.pack('>H', self.sample_period_ms))

	def encode_microphone_data(self, ostream):
		count = len(self.microphone_data)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.microphone_data[i].encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_last_response(istream)
		self.decode_timestamp(istream)
		self.decode_sample_period_ms(istream)
		self.decode_microphone_data(istream)
		pass

	def decode_last_response(self, istream):
		self.last_response= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_sample_period_ms(self, istream):
		self.sample_period_ms= struct.unpack('>H', istream.read(2))[0]

	def decode_microphone_data(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = MicrophoneData()
			tmp.decode_internal(istream)
			self.microphone_data.append(tmp)


class ScanDataResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.last_response = 0
		self.timestamp = None
		self.scan_result_data = []
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_last_response(ostream)
		self.encode_timestamp(ostream)
		self.encode_scan_result_data(ostream)
		pass

	def encode_last_response(self, ostream):
		ostream.write(struct.pack('>B', self.last_response))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_scan_result_data(self, ostream):
		count = len(self.scan_result_data)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.scan_result_data[i].encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_last_response(istream)
		self.decode_timestamp(istream)
		self.decode_scan_result_data(istream)
		pass

	def decode_last_response(self, istream):
		self.last_response= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_scan_result_data(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = ScanResultData()
			tmp.decode_internal(istream)
			self.scan_result_data.append(tmp)


class AccelerometerDataResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.last_response = 0
		self.timestamp = None
		self.accelerometer_data = []
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_last_response(ostream)
		self.encode_timestamp(ostream)
		self.encode_accelerometer_data(ostream)
		pass

	def encode_last_response(self, ostream):
		ostream.write(struct.pack('>B', self.last_response))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_accelerometer_data(self, ostream):
		count = len(self.accelerometer_data)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.accelerometer_data[i].encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_last_response(istream)
		self.decode_timestamp(istream)
		self.decode_accelerometer_data(istream)
		pass

	def decode_last_response(self, istream):
		self.last_response= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_accelerometer_data(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = AccelerometerData()
			tmp.decode_internal(istream)
			self.accelerometer_data.append(tmp)


class AccelerometerInterruptDataResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.last_response = 0
		self.timestamp = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_last_response(ostream)
		self.encode_timestamp(ostream)
		pass

	def encode_last_response(self, ostream):
		ostream.write(struct.pack('>B', self.last_response))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_last_response(istream)
		self.decode_timestamp(istream)
		pass

	def decode_last_response(self, istream):
		self.last_response= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)


class BatteryDataResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.last_response = 0
		self.timestamp = None
		self.battery_data = None
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_last_response(ostream)
		self.encode_timestamp(ostream)
		self.encode_battery_data(ostream)
		pass

	def encode_last_response(self, ostream):
		ostream.write(struct.pack('>B', self.last_response))

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_battery_data(self, ostream):
		self.battery_data.encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_last_response(istream)
		self.decode_timestamp(istream)
		self.decode_battery_data(istream)
		pass

	def decode_last_response(self, istream):
		self.last_response= struct.unpack('>B', istream.read(1))[0]

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_battery_data(self, istream):
		self.battery_data = BatteryData()
		self.battery_data.decode_internal(istream)


class StreamResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.timestamp = None
		self.battery_stream = []
		self.microphone_stream = []
		self.scan_stream = []
		self.accelerometer_stream = []
		self.accelerometer_interrupt_stream = []
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_timestamp(ostream)
		self.encode_battery_stream(ostream)
		self.encode_microphone_stream(ostream)
		self.encode_scan_stream(ostream)
		self.encode_accelerometer_stream(ostream)
		self.encode_accelerometer_interrupt_stream(ostream)
		pass

	def encode_timestamp(self, ostream):
		self.timestamp.encode_internal(ostream)

	def encode_battery_stream(self, ostream):
		count = len(self.battery_stream)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.battery_stream[i].encode_internal(ostream)

	def encode_microphone_stream(self, ostream):
		count = len(self.microphone_stream)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.microphone_stream[i].encode_internal(ostream)

	def encode_scan_stream(self, ostream):
		count = len(self.scan_stream)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.scan_stream[i].encode_internal(ostream)

	def encode_accelerometer_stream(self, ostream):
		count = len(self.accelerometer_stream)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.accelerometer_stream[i].encode_internal(ostream)

	def encode_accelerometer_interrupt_stream(self, ostream):
		count = len(self.accelerometer_interrupt_stream)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.accelerometer_interrupt_stream[i].encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_timestamp(istream)
		self.decode_battery_stream(istream)
		self.decode_microphone_stream(istream)
		self.decode_scan_stream(istream)
		self.decode_accelerometer_stream(istream)
		self.decode_accelerometer_interrupt_stream(istream)
		pass

	def decode_timestamp(self, istream):
		self.timestamp = Timestamp()
		self.timestamp.decode_internal(istream)

	def decode_battery_stream(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = BatteryStream()
			tmp.decode_internal(istream)
			self.battery_stream.append(tmp)

	def decode_microphone_stream(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = MicrophoneStream()
			tmp.decode_internal(istream)
			self.microphone_stream.append(tmp)

	def decode_scan_stream(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = ScanStream()
			tmp.decode_internal(istream)
			self.scan_stream.append(tmp)

	def decode_accelerometer_stream(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = AccelerometerStream()
			tmp.decode_internal(istream)
			self.accelerometer_stream.append(tmp)

	def decode_accelerometer_interrupt_stream(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = AccelerometerInterruptStream()
			tmp.decode_internal(istream)
			self.accelerometer_interrupt_stream.append(tmp)


class TestResponse:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.test_passed = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_test_passed(ostream)
		pass

	def encode_test_passed(self, ostream):
		ostream.write(struct.pack('>B', self.test_passed))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_test_passed(istream)
		pass

	def decode_test_passed(self, istream):
		self.test_passed= struct.unpack('>B', istream.read(1))[0]


class Response:

	def __init__(self):
		self.reset()

	def __repr__(self):
		return str(self.__dict__)

	def reset(self):
		self.type = self._type()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.type.encode_internal(ostream)
		pass


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.type.decode_internal(istream)
		pass

	class _type:

		def __init__(self):
			self.reset()

		def __repr__(self):
			return str(self.__dict__)

		def reset(self):
			self.which = 0
			self.status_response = None
			self.start_microphone_response = None
			self.start_scan_response = None
			self.start_accelerometer_response = None
			self.start_accelerometer_interrupt_response = None
			self.start_battery_response = None
			self.microphone_data_response = None
			self.scan_data_response = None
			self.accelerometer_data_response = None
			self.accelerometer_interrupt_data_response = None
			self.battery_data_response = None
			self.stream_response = None
			self.test_response = None
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('>B', self.which))
			options = {
				1: self.encode_status_response,
				2: self.encode_start_microphone_response,
				3: self.encode_start_scan_response,
				4: self.encode_start_accelerometer_response,
				5: self.encode_start_accelerometer_interrupt_response,
				6: self.encode_start_battery_response,
				7: self.encode_microphone_data_response,
				8: self.encode_scan_data_response,
				9: self.encode_accelerometer_data_response,
				10: self.encode_accelerometer_interrupt_data_response,
				11: self.encode_battery_data_response,
				12: self.encode_stream_response,
				13: self.encode_test_response,
			}
			options[self.which](ostream)
			pass

		def encode_status_response(self, ostream):
			self.status_response.encode_internal(ostream)

		def encode_start_microphone_response(self, ostream):
			self.start_microphone_response.encode_internal(ostream)

		def encode_start_scan_response(self, ostream):
			self.start_scan_response.encode_internal(ostream)

		def encode_start_accelerometer_response(self, ostream):
			self.start_accelerometer_response.encode_internal(ostream)

		def encode_start_accelerometer_interrupt_response(self, ostream):
			self.start_accelerometer_interrupt_response.encode_internal(ostream)

		def encode_start_battery_response(self, ostream):
			self.start_battery_response.encode_internal(ostream)

		def encode_microphone_data_response(self, ostream):
			self.microphone_data_response.encode_internal(ostream)

		def encode_scan_data_response(self, ostream):
			self.scan_data_response.encode_internal(ostream)

		def encode_accelerometer_data_response(self, ostream):
			self.accelerometer_data_response.encode_internal(ostream)

		def encode_accelerometer_interrupt_data_response(self, ostream):
			self.accelerometer_interrupt_data_response.encode_internal(ostream)

		def encode_battery_data_response(self, ostream):
			self.battery_data_response.encode_internal(ostream)

		def encode_stream_response(self, ostream):
			self.stream_response.encode_internal(ostream)

		def encode_test_response(self, ostream):
			self.test_response.encode_internal(ostream)


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('>B', istream.read(1))[0]
			options = {
				1: self.decode_status_response,
				2: self.decode_start_microphone_response,
				3: self.decode_start_scan_response,
				4: self.decode_start_accelerometer_response,
				5: self.decode_start_accelerometer_interrupt_response,
				6: self.decode_start_battery_response,
				7: self.decode_microphone_data_response,
				8: self.decode_scan_data_response,
				9: self.decode_accelerometer_data_response,
				10: self.decode_accelerometer_interrupt_data_response,
				11: self.decode_battery_data_response,
				12: self.decode_stream_response,
				13: self.decode_test_response,
			}
			options[self.which](istream)
			pass

		def decode_status_response(self, istream):
			self.status_response = StatusResponse()
			self.status_response.decode_internal(istream)

		def decode_start_microphone_response(self, istream):
			self.start_microphone_response = StartMicrophoneResponse()
			self.start_microphone_response.decode_internal(istream)

		def decode_start_scan_response(self, istream):
			self.start_scan_response = StartScanResponse()
			self.start_scan_response.decode_internal(istream)

		def decode_start_accelerometer_response(self, istream):
			self.start_accelerometer_response = StartAccelerometerResponse()
			self.start_accelerometer_response.decode_internal(istream)

		def decode_start_accelerometer_interrupt_response(self, istream):
			self.start_accelerometer_interrupt_response = StartAccelerometerInterruptResponse()
			self.start_accelerometer_interrupt_response.decode_internal(istream)

		def decode_start_battery_response(self, istream):
			self.start_battery_response = StartBatteryResponse()
			self.start_battery_response.decode_internal(istream)

		def decode_microphone_data_response(self, istream):
			self.microphone_data_response = MicrophoneDataResponse()
			self.microphone_data_response.decode_internal(istream)

		def decode_scan_data_response(self, istream):
			self.scan_data_response = ScanDataResponse()
			self.scan_data_response.decode_internal(istream)

		def decode_accelerometer_data_response(self, istream):
			self.accelerometer_data_response = AccelerometerDataResponse()
			self.accelerometer_data_response.decode_internal(istream)

		def decode_accelerometer_interrupt_data_response(self, istream):
			self.accelerometer_interrupt_data_response = AccelerometerInterruptDataResponse()
			self.accelerometer_interrupt_data_response.decode_internal(istream)

		def decode_battery_data_response(self, istream):
			self.battery_data_response = BatteryDataResponse()
			self.battery_data_response.decode_internal(istream)

		def decode_stream_response(self, istream):
			self.stream_response = StreamResponse()
			self.stream_response.decode_internal(istream)

		def decode_test_response(self, istream):
			self.test_response = TestResponse()
			self.test_response.decode_internal(istream)


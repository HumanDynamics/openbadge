import struct
from test_parent_protocol import *

TEST1 = 10
TEST2 = 12
TEST3 = 1000

Embedded_message_g_tag = 100
Test_message_x_tag = 1
Test_message_embedded_message_oneof_tag = 2

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

class Embedded_message1:

	def __init__(self):
		self.reset()

	def reset(self):
		self.has_e = 0
		self.e = 0
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_e(ostream)
		pass

	def encode_e(self, ostream):
		ostream.write(struct.pack('>B', self.has_e))
		if self.has_e:
			ostream.write(struct.pack('>Q', self.e))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_e(istream)
		pass

	def decode_e(self, istream):
		self.has_e= struct.unpack('>B', istream.read(1))[0]
		if self.has_e:
			self.e= struct.unpack('>Q', istream.read(8))[0]


class Embedded_message:

	def __init__(self):
		self.reset()

	def reset(self):
		self.f = 0
		self.embedded_message1 = []
		self.embedded_payload = self._embedded_payload()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_f(ostream)
		self.encode_embedded_message1(ostream)
		self.embedded_payload.encode_internal(ostream)
		pass

	def encode_f(self, ostream):
		ostream.write(struct.pack('>B', self.f))

	def encode_embedded_message1(self, ostream):
		count = 2
		for i in range(0, count):
			self.embedded_message1[i].encode_internal(ostream)


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_f(istream)
		self.decode_embedded_message1(istream)
		self.embedded_payload.decode_internal(istream)
		pass

	def decode_f(self, istream):
		self.f= struct.unpack('>B', istream.read(1))[0]

	def decode_embedded_message1(self, istream):
		count = 2
		for i in range(0, count):
			tmp = Embedded_message1()
			tmp.decode_internal(istream)
			self.embedded_message1.append(tmp)

	class _embedded_payload:

		def __init__(self):
			self.reset()

		def reset(self):
			self.which = 0
			self.g = 0
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('>B', self.which))
			options = {
				100: self.encode_g,
			}
			options[self.which](ostream)
			pass

		def encode_g(self, ostream):
			ostream.write(struct.pack('>B', self.g))


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('>B', istream.read(1))[0]
			options = {
				100: self.decode_g,
			}
			options[self.which](istream)
			pass

		def decode_g(self, istream):
			self.g= struct.unpack('>B', istream.read(1))[0]


class Test_message:

	def __init__(self):
		self.reset()

	def reset(self):
		self.fixed_array = []
		self.has_a = 0
		self.a = 0
		self.b = 0
		self.uint16_array = []
		self.embedded_messages = []
		self.has_embedded_message1 = 0
		self.embedded_message1 = None
		self.empty_message = None
		self.uint8_array = []
		self.has_c = 0
		self.c = 0
		self.d = 0
		self.payload = self._payload()
		pass

	def encode(self):
		ostream = _Ostream()
		self.encode_internal(ostream)
		return ostream.buf

	def encode_internal(self, ostream):
		self.encode_fixed_array(ostream)
		self.encode_a(ostream)
		self.encode_b(ostream)
		self.encode_uint16_array(ostream)
		self.encode_embedded_messages(ostream)
		self.encode_embedded_message1(ostream)
		self.encode_empty_message(ostream)
		self.encode_uint8_array(ostream)
		self.encode_c(ostream)
		self.encode_d(ostream)
		self.payload.encode_internal(ostream)
		pass

	def encode_fixed_array(self, ostream):
		count = 4
		for i in range(0, count):
			ostream.write(struct.pack('>I', self.fixed_array[i]))

	def encode_a(self, ostream):
		ostream.write(struct.pack('>B', self.has_a))
		if self.has_a:
			ostream.write(struct.pack('>H', self.a))

	def encode_b(self, ostream):
		ostream.write(struct.pack('>i', self.b))

	def encode_uint16_array(self, ostream):
		count = len(self.uint16_array)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			ostream.write(struct.pack('>H', self.uint16_array[i]))

	def encode_embedded_messages(self, ostream):
		count = len(self.embedded_messages)
		ostream.write(struct.pack('>B', count))
		for i in range(0, count):
			self.embedded_messages[i].encode_internal(ostream)

	def encode_embedded_message1(self, ostream):
		ostream.write(struct.pack('>B', self.has_embedded_message1))
		if self.has_embedded_message1:
			self.embedded_message1.encode_internal(ostream)

	def encode_empty_message(self, ostream):
		self.empty_message.encode_internal(ostream)

	def encode_uint8_array(self, ostream):
		count = len(self.uint8_array)
		ostream.write(struct.pack('>H', count))
		for i in range(0, count):
			ostream.write(struct.pack('>B', self.uint8_array[i]))

	def encode_c(self, ostream):
		ostream.write(struct.pack('>B', self.has_c))
		if self.has_c:
			ostream.write(struct.pack('>d', self.c))

	def encode_d(self, ostream):
		ostream.write(struct.pack('>f', self.d))


	@classmethod
	def decode(cls, buf):
		obj = cls()
		obj.decode_internal(_Istream(buf))
		return obj

	def decode_internal(self, istream):
		self.reset()
		self.decode_fixed_array(istream)
		self.decode_a(istream)
		self.decode_b(istream)
		self.decode_uint16_array(istream)
		self.decode_embedded_messages(istream)
		self.decode_embedded_message1(istream)
		self.decode_empty_message(istream)
		self.decode_uint8_array(istream)
		self.decode_c(istream)
		self.decode_d(istream)
		self.payload.decode_internal(istream)
		pass

	def decode_fixed_array(self, istream):
		count = 4
		for i in range(0, count):
			self.fixed_array.append(struct.unpack('>I', istream.read(4))[0])

	def decode_a(self, istream):
		self.has_a= struct.unpack('>B', istream.read(1))[0]
		if self.has_a:
			self.a= struct.unpack('>H', istream.read(2))[0]

	def decode_b(self, istream):
		self.b= struct.unpack('>i', istream.read(4))[0]

	def decode_uint16_array(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			self.uint16_array.append(struct.unpack('>H', istream.read(2))[0])

	def decode_embedded_messages(self, istream):
		count = struct.unpack('>B', istream.read(1))[0]
		for i in range(0, count):
			tmp = Embedded_message()
			tmp.decode_internal(istream)
			self.embedded_messages.append(tmp)

	def decode_embedded_message1(self, istream):
		self.has_embedded_message1= struct.unpack('>B', istream.read(1))[0]
		if self.has_embedded_message1:
			self.embedded_message1 = Embedded_message1()
			self.embedded_message1.decode_internal(istream)

	def decode_empty_message(self, istream):
		self.empty_message = Empty_message()
		self.empty_message.decode_internal(istream)

	def decode_uint8_array(self, istream):
		count = struct.unpack('>H', istream.read(2))[0]
		for i in range(0, count):
			self.uint8_array.append(struct.unpack('>B', istream.read(1))[0])

	def decode_c(self, istream):
		self.has_c= struct.unpack('>B', istream.read(1))[0]
		if self.has_c:
			self.c= struct.unpack('>d', istream.read(8))[0]

	def decode_d(self, istream):
		self.d= struct.unpack('>f', istream.read(4))[0]

	def get_uint8_array_as_bytestring(self):
		bytestring = b''
		for b in self.uint8_array:
			bytestring += struct.pack('>B', b)
		return bytestring

	def set_uint8_array_as_bytestring(self, bytestring):
		self.uint8_array = []
		for b in bytestring:
			self.uint8_array.append(struct.unpack('>B', b)[0])

	class _payload:

		def __init__(self):
			self.reset()

		def reset(self):
			self.which = 0
			self.x = 0
			self.embedded_message_oneof = None
			pass

		def encode_internal(self, ostream):
			ostream.write(struct.pack('>B', self.which))
			options = {
				1: self.encode_x,
				2: self.encode_embedded_message_oneof,
			}
			options[self.which](ostream)
			pass

		def encode_x(self, ostream):
			ostream.write(struct.pack('>B', self.x))

		def encode_embedded_message_oneof(self, ostream):
			self.embedded_message_oneof.encode_internal(ostream)


		def decode_internal(self, istream):
			self.reset()
			self.which= struct.unpack('>B', istream.read(1))[0]
			options = {
				1: self.decode_x,
				2: self.decode_embedded_message_oneof,
			}
			options[self.which](istream)
			pass

		def decode_x(self, istream):
			self.x= struct.unpack('>B', istream.read(1))[0]

		def decode_embedded_message_oneof(self, istream):
			self.embedded_message_oneof = Embedded_message()
			self.embedded_message_oneof.decode_internal(istream)



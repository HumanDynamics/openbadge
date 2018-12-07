from __future__ import division, absolute_import, print_function
from badge_connection import *

import logging
import time
import uuid
import sys

from bluepy import *
from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers ,Scanner
from bluepy.btle import BTLEException
import struct	
import Queue

logger = logging.getLogger(__name__)

# Enable debug output.
# logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)

# Define service and characteristic UUIDs used by the UART service.
UART_SERVICE_UUID = uuid.UUID('6E400001-B5A3-F393-E0A9-E50E24DCCA9E')
TX_CHAR_UUID      = uuid.UUID('6E400002-B5A3-F393-E0A9-E50E24DCCA9E')
RX_CHAR_UUID      = uuid.UUID('6E400003-B5A3-F393-E0A9-E50E24DCCA9E')


class SimpleDelegate(DefaultDelegate):

    def __init__(self, bleconn):
        DefaultDelegate.__init__(self)
        self.bleconn = bleconn

    def handleNotification(self, cHandle, data):
        self.bleconn.received(data)



# BLEBadgeConnection represents a connection to 'ble_device', a badge connected over BLE.
#    ('ble_device' should be a device from the Bluefruit Library)
# This class implements the BadgeConnection interface to communicate with
#   a badge over the BLE UART service using the Adafruit BlueFruit Library.
# This class implements an additional class method, BLEBadgeConnection.get_connection_to_badge(), 
#   that can be used to retrieve an instance of this class that represents a connection to a badge with
#   the given ID. This class method should be the main way clients instantiate new BLEBadgeConnections.
class BLEBadgeConnection(BadgeConnection):
	
	def __init__(self, ble_device):
		self.ble_device = ble_device

		# We will set these on connection to be the correct objects from the
		#   Adafruit library.
		self.uart = None
		self.rx = None
		self.tx = None
		self.conn = None
		

		# Contains the bytes recieved from the device. Held here until an entire message is recieved.
		self.rx_queue = Queue.Queue()
		
		
		BadgeConnection.__init__(self)

	# Returns a BLEBadgeConnection() to the first badge it sees, or none if a badge
	#   could not be found in timeout_seconds seconds. (Default is 10)
	@classmethod
	def get_connection_to_badge(cls, device_addr, timeout_seconds=10.0):
		
		return cls(device_addr)


	# Function to receive RX characteristic changes.  Note that this will
	# be called on a different thread so be careful to make sure state that
	# the function changes is thread safe.  Use Queue or other thread-safe
	# primitives to send data to other threads.

	def received(self,data):
		logger.debug("Recieved {}".format(data.encode("hex")))
		
		for b in data:
			self.rx_queue.put(b)
			
		

	# Implements BadgeConnection's connect() spec.	
	def connect(self):
		logger.debug("Connecting...")
		self.conn = btle.Peripheral(self.ble_device, btle.ADDR_TYPE_RANDOM)
		
		logger.debug("Connected.")

		# Find the UART service and its characteristics.
		self.uart = self.conn.getServiceByUUID(UART_SERVICE_UUID)
		self.rx = self.uart.getCharacteristics(RX_CHAR_UUID)[0]
		self.tx = self.uart.getCharacteristics(TX_CHAR_UUID)[0]

		# Turn on notification of RX characteristics
		logger.debug('Subscribing to RX characteristic changes...')
		CONFIG_HANDLE = 0x000c;
		self.conn.writeCharacteristic(handle=CONFIG_HANDLE,val=struct.pack('<bb', 0x01, 0x00))
		self.conn.setDelegate(SimpleDelegate(bleconn=self))


	# Implements BadgeConnections's disconnect() spec.
	def disconnect(self):
		
		self.uart = None
		self.tx = None
		self.rx = None
		
		with self.rx_queue.mutex:
			self.rx_queue.queue.clear()
		
		#self.ble_device.disconnect()
		self.conn.disconnect()
		self.ble_device = None


	# Implements BadgeConnection's is_connected() spec.
	def is_connected(self):

		if (self.ble_device == None):
			return False
		else:
			return True


	# Implements BadgeConnection's await_data() spec.
	def await_data(self, data_len):
		if not self.is_connected():
			raise RuntimeError("BLEBadgeConnection not connected before await_data()!")

		rx_message = ""
		rx_bytes_expected = data_len
		
		if rx_bytes_expected > 0:
			while True:
				while(not self.rx_queue.empty()):
					rx_message += self.rx_queue.get()
					if(len(rx_message) == rx_bytes_expected):
						return rx_message
						
				self.conn.waitForNotifications(5.0)
		
			

	# Implements BadgeConnection's send() spec.
	def send(self, message, response_len=0):
		if not self.is_connected():
			raise RuntimeError("BLEBadgeConnection not connected before send()!")

		rx_message = ""
		rx_bytes_expected = response_len
		
		self.tx.write(message,withResponse=True)
	
		
		if rx_bytes_expected > 0:
			while True:
				while(not self.rx_queue.empty()):
					rx_message += self.rx_queue.get()
					if(len(rx_message) == rx_bytes_expected):
						return rx_message
						
				self.conn.waitForNotifications(5.0)
				
		
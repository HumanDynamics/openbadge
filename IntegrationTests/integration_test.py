import unittest
import Adafruit_BluefruitLE
import logging
import serial
import serial.tools.list_ports
import threading

from BadgeFramework.ble_badge_connection import BLEBadgeConnection
from BadgeFramework.badge import OpenBadge

logging.basicConfig(filename="integration_test.log", level=logging.INFO)
logger = logging.getLogger(__name__)

class IntegrationTest(unittest.TestCase):
	def __init__(self):
		self.test_running = False
		unittest.TestCase.__init__(self)

	def onUartRx(self, data):
		logger.debug(data)

	def runTest(self):
		if self.test_running: return
		self.test_running = True

		uartPort = list(serial.tools.list_ports.grep("cu.usbmodem"))[0]
		uartSerial = serial.Serial(uartPort.device, 115200)

		def uartRXTarget():
			while self.test_running:
				self.onUartRx(uartSerial.readline())

		uartRXThread = threading.Thread(target=uartRXTarget)
		uartRXThread.start()

		# AdaFruit has this really handy helper function, but we should probably write our own, so that
		# we don't have to propogate the AdaFruit dependency everywhere.
		Adafruit_BluefruitLE.get_provider().run_mainloop_with(self.runTest_MainLoop)

	def runTest_MainLoop(self):
		connection = BLEBadgeConnection.get_connection_to_badge()
		connection.connect()
		badge = OpenBadge(connection)
		self.testCase(badge)
		self.test_running = False

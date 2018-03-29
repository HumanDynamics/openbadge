import unittest
import sys
import Adafruit_BluefruitLE
import logging
import time
import serial
import serial.tools.list_ports
import threading

from BadgeFramework.ble_badge_connection import BLEBadgeConnection
from BadgeFramework.badge import OpenBadge

logging.basicConfig(filename="integration_test.log", level=logging.DEBUG)
logger = logging.getLogger(__name__)

# Enable log output to terminal
stdout_handler = logging.StreamHandler(sys.stdout)
stdout_handler.setLevel(logging.DEBUG)
logger.addHandler(stdout_handler)

# Uncomment this line to make logging very verbose.
# logging.getLogger().addHandler(stdout_handler)

# Special badge restart command only used for testing purposes
def restart_badge(serial):
	serial.write("restart\n")
	time.sleep(5)

class IntegrationTest(unittest.TestCase):
	def __init__(self):
		unittest.TestCase.__init__(self)

	def runTest(self):
		self.runTest_startUART()
		# AdaFruit has this really handy helper function, but we should probably write our own, so that
		# we don't have to propogate the AdaFruit dependency everywhere.
		Adafruit_BluefruitLE.get_provider().run_mainloop_with(self.runTest_MainLoop)

	def runTest_startUART(self):
		uartPort = list(serial.tools.list_ports.grep("cu.usbmodem"))[0]
		self.uartSerial = serial.Serial(uartPort.device, 115200, timeout=1)

		def uartRXTarget():
			while True:
				# Some slight implicit control flow going on here:
				#   uartSerial.readline() will sometimes timeout, and then we'll just loop around.
				rx_data = self.uartSerial.readline()
				if rx_data:
					# We truncate the ending newline. 
					self.onUartLineRx(rx_data[:-1])

		uartRXThread = threading.Thread(target=uartRXTarget)
		uartRXThread.setDaemon(True)
		uartRXThread.start()

	def onUartLineRx(self, data):
		logger.info("UART:" + data)

	def runTest_MainLoop(self):
		restart_badge(self.uartSerial)

		connection = BLEBadgeConnection.get_connection_to_badge()
		connection.connect()
		badge = OpenBadge(connection)

		try:
			self.testCase(badge, logger)
			print "Test Passed! :)"
		except Exception as e:
			self.onTestFailure(badge, logger)
			raise AssertionError("Test Failure")

	def onTestFailure(self, badge, logger):
		logger.exception("Exception during test!")
		logger.info("Badge Status after Failure: {}".format(badge.get_status()))

from __future__ import division, absolute_import, print_function
# This BadgeConnection interface represents a connection to an OpenBadge.
#   This connection is used by the Badge object to communicate with a physical badge.
#      This acts to bridge our abstraction of a badge to our implementation of 
#      how we communicate with that badge.
#   This connection can be implemented by different subclasses using any 
#     number of different libraries (Adafruit, BluePY, PYObjC) or communications protocols. (UART, BLE)
#    As long as a given implementation (subclass) implements these methods and follows their specs,
#    Badge() will be able to use that BadgeConnection to communicate with a physical badge. 
class BadgeConnection(object):
	def __init__(self):
		pass

	# Connect to the physical device.
	# Must be called before any other methods communicating with the badge
	#   can be called. 
	def connect(self):
		raise NotImplementedError

	# Disconnect from this badge and reset any internal state.
	def disconnect(self):
		raise NotImplementedError

	# Returns true if this BadgeConnection is currently connected to a physical badge.
	def is_connected(self):
		raise NotImplementedError

	# Send the `message` byte string to the badge over this BadgeConnection. 
	# Await a response of length response_len.
	#   Blocks until response recieved. 
	#   Returns None immediately after sending if response_len == 0
	# This method should throw a RuntimeError if this BadgeConnection is not currently connected.
	def send(self, message, response_len=0):
		raise NotImplementedError

	# Await data_len bytes to be recieved from the badge over this connection
	#  and return them after they have been recieved.
	# Returns None immediately if data_len == 0.
	# This method blocks until data_len bytes have been recieved.
	# This method should throw a RuntimeError if this BadgeConnection is not currently connected.
	def await_data(self, data_len):
		raise NotImplementedError

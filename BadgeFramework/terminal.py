from __future__ import division, absolute_import, print_function
import logging
import sys
import threading


from badge import *
from ble_badge_connection import *
from bluepy import *
from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers ,Scanner
from bluepy.btle import BTLEException

# Enable debug output.
logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)
# Main Loop of Badge Terminal

def main():
	num_args = len(sys.argv) # Get the arguments list
	if num_args != 2:
		print("Please enter badge MAC address")
		return

	device_addr = sys.argv[1]
	print("Connecting to badge", device_addr)	
	connection = BLEBadgeConnection.get_connection_to_badge(device_addr)

	if not connection:
		print("Could not find nearby OpenBadge. :( Please try again!")
		return

	connection.connect()
	badge = OpenBadge(connection)

	print("Connected!")

	def print_help(args):
		print("Available commands: [optional arguments]")
		print("  status [new badge id] [group number] (id + group must be set together)")
		print("  start_microphone")
		print("  stop_microphone")
		print("  start_scan")
		print("  stop_scan")
		print("  start_accelerometer")
		print("  stop_accelerometer")
		print("  start_accelerometer_interrupt")
		print("  stop_accelerometer_interrupt")
		print("  start_battery")
		print("  stop_battery")
		print("  get_microphone_data [seconds of mic data to request]")
		print("  get_scan_data [seconds of scan data to request]")
		print("  get_accelerometer_data [seconds of accelerometer data to request]")
		print("  get_accelerometer_interrupt_data [seconds of accelerometer interrupt data to request]")
		print("  get_battery_data [seconds of battery data to request]")
		print("  identify [led duration seconds | 'off']")
		print("  test")
		print("  restart")
		print("  help")
		print("  start_microphone_stream")
		print("  stop_microphone_stream")
		print("  start_scan_stream")
		print("  stop_scan_stream")
		print("  start_accelerometer_stream")
		print("  stop_accelerometer_stream")
		print("  start_accelerometer_interrupt_stream")
		print("  stop_accelerometer_interrupt_stream")
		print("  start_battery_stream")
		print("  stop_battery_stream")
		print("  stream")
		print("All commands use current system time as transmitted time.")
		print("Default arguments used where not specified.")

	def handle_status_request(args):
		if len(args) == 1:
			print(badge.get_status())
		elif len(args) == 2:
			print("Badge ID and Group Number Must Be Set Simultaneously")
		elif len(args) == 3:
			new_id = int(args[1])
			group_number = int(args[2])
			print(badge.get_status(new_id=new_id, new_group_number=group_number))
		else:
			print("Invalid Syntax: status [new badge id] [group number]")

	def handle_start_microphone_request(args):
		print(badge.start_microphone())


	def handle_stop_microphone_request(args):
		badge.stop_microphone()
	

	def handle_start_scan_request(args):
		print(badge.start_scan())

	def handle_stop_scan_request(args):
		badge.stop_scan()
		
			
	def handle_start_accelerometer_request(args):
		print(badge.start_accelerometer())

	def handle_stop_accelerometer_request(args):
		badge.stop_accelerometer()
		
		
	def handle_start_accelerometer_interrupt_request(args):
		print(badge.start_accelerometer_interrupt())

	def handle_stop_accelerometer_interrupt_request(args):
		badge.stop_accelerometer_interrupt()
		
	def handle_start_battery_request(args):
		print(badge.start_battery())

	def handle_stop_battery_request(args):
		badge.stop_battery()
		

	def handle_get_microphone_data(args):
		if len(args) == 1:
			print(badge.get_microphone_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_microphone_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_microphone_data [seconds of microphone data to request]")


	def handle_get_scan_data(args):
		if len(args) == 1:
			print(badge.get_scan_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_scan_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_scan_data [seconds of scan data to request]")
			
	def handle_get_accelerometer_data(args):
		if len(args) == 1:
			print(badge.get_accelerometer_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_accelerometer_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_accelerometer_data [seconds of accelerometer data to request]")
			
	def handle_get_accelerometer_interrupt_data(args):
		if len(args) == 1:
			print(badge.get_accelerometer_interrupt_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_accelerometer_interrupt_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_accelerometer_interrupt_data [seconds of accelerometer interrupt data to request]")
			
	def handle_get_battery_data(args):
		if len(args) == 1:
			print(badge.get_battery_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_battery_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_battery_data [seconds of battery data to request]")
			
		

	def handle_identify_request(args):
		if len(args) == 1:
			badge.identify()
		elif len(args) == 2:
			if args[1] == "off":
				badge.identify(duration_seconds=0)
			else:
				badge.identify(duration_seconds=int(args[1]))
		else:
			print("Invalid Syntax: identify [led duration seconds | 'off']")
			return

	def handle_test_request(args):
		print(badge.test())
	
	def handle_restart_request(args):
		print(badge.restart())
		
		
		
	def handle_start_microphone_stream_request(args):
		badge.start_microphone_stream()


	def handle_stop_microphone_stream_request(args):
		badge.stop_microphone_stream()
	

	def handle_start_scan_stream_request(args):
		badge.start_scan_stream()

	def handle_stop_scan_stream_request(args):
		badge.stop_scan_stream()
		
			
	def handle_start_accelerometer_stream_request(args):
		badge.start_accelerometer_stream()

	def handle_stop_accelerometer_stream_request(args):
		badge.stop_accelerometer_stream()
		
		
	def handle_start_accelerometer_interrupt_stream_request(args):
		badge.start_accelerometer_interrupt_stream()

	def handle_stop_accelerometer_interrupt_stream_request(args):
		badge.stop_accelerometer_interrupt_stream()
		
	def handle_start_battery_stream_request(args):
		badge.start_battery_stream()

	def handle_stop_battery_stream_request(args):
		badge.stop_battery_stream()
		
	def handle_stream_request(args):
		badge.stream_clear()
		stream = badge.get_stream()
		while(not stream == []):
			print(stream)
			stream = badge.get_stream()

	command_handlers = {
		"help": print_help,
		"status": handle_status_request,
		"start_microphone": handle_start_microphone_request,
		"stop_microphone": handle_stop_microphone_request,
		"start_scan": handle_start_scan_request,
		"stop_scan": handle_stop_scan_request,
		"start_accelerometer": handle_start_accelerometer_request,
		"stop_accelerometer": handle_stop_accelerometer_request,
		"start_accelerometer_interrupt": handle_start_accelerometer_interrupt_request,
		"stop_accelerometer_interrupt": handle_stop_accelerometer_interrupt_request,
		"start_battery": handle_start_battery_request,
		"stop_battery": handle_stop_battery_request,
		"get_microphone_data": handle_get_microphone_data,
		"get_scan_data": handle_get_scan_data,
		"get_accelerometer_data": handle_get_accelerometer_data,
		"get_accelerometer_interrupt_data": handle_get_accelerometer_interrupt_data,
		"get_battery_data": handle_get_battery_data,
		"identify": handle_identify_request,
		"test": handle_test_request,
		"restart": handle_restart_request,
		"start_microphone_stream": handle_start_microphone_stream_request,
		"stop_microphone_stream": handle_stop_microphone_stream_request,
		"start_scan_stream": handle_start_scan_stream_request,
		"stop_scan_stream": handle_stop_scan_stream_request,
		"start_accelerometer_stream": handle_start_accelerometer_stream_request,
		"stop_accelerometer_stream": handle_stop_accelerometer_stream_request,
		"start_accelerometer_interrupt_stream": handle_start_accelerometer_interrupt_stream_request,
		"stop_accelerometer_interrupt_stream": handle_stop_accelerometer_interrupt_stream_request,
		"start_battery_stream": handle_start_battery_stream_request,
		"stop_battery_stream": handle_stop_battery_stream_request,
		"stream": handle_stream_request,		
	}

	while True:
		sys.stdout.write("> ")
		# [:-1] removes newline character
		command = sys.stdin.readline()[:-1]
		if command == "exit":
			connection.disconnect()
			break

		command_args = command.split(" ")
		if command_args[0] in command_handlers:
			command_handlers[command_args[0]](command_args)
		else:
			print("Command Not Found!")
			print_help({})




if __name__ == "__main__":
	main()
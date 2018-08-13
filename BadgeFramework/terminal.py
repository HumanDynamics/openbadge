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
		print("  start_record [timeout in minutes]")
		print("  stop_record")
		print("  start_scan")
		print("  stop_scan")
		print("  get_mic_data [seconds of mic data to request]")
		print("  get_audio_stream [seconds of mic data to request]")
		print("  get_scan_data [seconds of scan data to request]")
		print("  identify [led duration seconds | 'off']")
		print("  help")
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

	def handle_start_record_request(args):
		if len(args) == 1:
			print(badge.start_recording())
		elif len(args) == 2:
			print(badge.start_recording(timeout_minutes=int(args[1])))
		else:
			print("Invalid Syntax: start_record [timeout in minutes]")

	def handle_stop_record_request(args):
		if badge.stop_recording():
			print("Stop request request sent!")
		else:
			print("Stop record request failed. :(")

	def handle_start_scanning_request(args):
		print(badge.start_scanning())

	def handle_stop_scanning_request(args):
		if badge.stop_scanning():
			print("Stop scanning request sent!")
		else:
			print("Stop scanning request failed. :(")

	def handle_get_mic_data(args):
		if len(args) == 1:
			print(badge.get_mic_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_mic_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_mic_data [seconds of mic data to request]")

	def handle_get_audio_stream(args):
		mic_data = None
		if len(args) == 1:
			mic_data = badge.get_mic_data()
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			mic_data = badge.get_mic_data(start_time_to_request)
		else:
			print("Invalid Syntax: get_audio_stream [seconds of audio to request]")
			return

		audio_stream = []
		last_chunk_end_time = None
		for header, samples in mic_data:
			this_chunk_start_time = timestamps_to_time(header.timestamp_seconds, header.timestamp_miliseconds)
			if last_chunk_end_time and this_chunk_start_time - last_chunk_end_time > 0.1:
				audio_stream.append(("GAP", this_chunk_start_time - last_chunk_end_time))

			audio_stream.extend(samples)

			chunk_duration = (header.num_samples_in_chunk * (float(header.sample_period_miliseconds) / 1000.0))
			last_chunk_end_time = this_chunk_start_time + chunk_duration

		print(audio_stream)

	def handle_get_scan_data(args):
		if len(args) == 1:
			print(badge.get_scan_data())
		elif len(args) == 2:
			start_time_to_request = int(time.time()) - int(args[1])
			print(badge.get_scan_data(start_time_to_request))
		else:
			print("Invalid Syntax: get_scan_data [seconds of scan data to request]")

	def handle_identify_request(args):
		if len(args) == 1:
			request_success = badge.identify()
		elif len(args) == 2:
			if args[1] == "off":
				request_success = badge.identify(duration_seconds=0)
			else:
				request_success = badge.identify(duration_seconds=int(args[1]))
		else:
			print("Invalid Syntax: identify [led duration seconds | 'off']")
			return

		if request_success:
			print("Identify request sent!")
		else:
			print("Identify request failed. :(")

	command_handlers = {
		"help": print_help,
		"status": handle_status_request,
		"start_record": handle_start_record_request,
		"stop_record": handle_stop_record_request,
		"start_scan": handle_start_scanning_request,
		"stop_scan": handle_stop_scanning_request,
		"get_mic_data": handle_get_mic_data,
		"get_audio_stream": handle_get_audio_stream,
		"get_scan_data": handle_get_scan_data,
		"identify": handle_identify_request,
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
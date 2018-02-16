#	serial_test.py
#	@author Zachary Pitcher
# 	@version 2/16/2018
#
#	***Intended for use with badge_03v4. Not guaranteed to work with other versions***
#
#	Connect the badge to the devkit, and connect the device via USB 
#	Ensure \dev\ttyACM0 exists and run the program 
#	Click the button on the badge to start capturing data
#	Log output will appear in BLE_range_test.log

import serial
import time
import os.path
import sys

# only accept messages of the form: XX:XX:XX:XX:XX:XX,-XX
def valid_msg(s):
	try:
		mac, rssi = s.split(",")
		for m in mac.split(":"):
			if not all([c.isalpha() or c.isdigit() for c in m]):	# expect each mac field to be hexadecimal
				return False
		int(rssi)	# expect rssi to be a negative number
	except ValueError:
		return False
	return True


# open log_file in append mode
with open("./BLE_range_test.log", "a") as log_file:		
	
	started = False

	try:
		# print to both user and log in file
		def all_streams(s):
			print s				
			log_file.write(s + "\r\n")

		# ensure that the device is connected
		assert os.path.exists("/dev/ttyACM0"), "/dev/ttyACM0 not found. Connect badge via USB and try again"	

		# open the device connection
		with serial.Serial("/dev/ttyACM0", 115200, timeout=1) as ser:

			# wait for user to start test
			line = ser.readline()
			if line == "":
				print "Press the button on the badge to begin test..."	

				while line == "":
					try:
			 			line = ser.readline()

					# occurs when devkit is removed or when attaching badge to devkit
					except serial.serialutil.SerialException:
						print "Connection to /dev/ttyACM0 lost. Reconnect and try again"
						sys.exit()

			# ignore first 10 scan results (they're just debug messages / garbage usually)
			for i in range(10):	
				ser.readline()	
			line = ser.readline()
			
			# start reading badge messages
			all_streams("-----------------------------\n" + str("%.3f," % time.time()) + "starting test")
			started = True
						
			while line != "":
				if valid_msg(line):	
					all_streams(str("%.3f," % time.time()) + line[:-2])
				try:
					line = ser.readline()
				except serial.serialutil.SerialException:
					print "Disconnected devkit"
					break
			else:
				print "Disconnected badge from devkit"
				#print "Invalid message. Try again with a fresh connection"
	except KeyboardInterrupt:
		# catch keyboard interupt to end test cleanly		
		print " Caught keyboard interrupt"
	finally:
		if started:
			all_streams(str("%.3f," % time.time()) + "ending test")


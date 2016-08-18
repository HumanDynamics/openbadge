#!/usr/bin/python
from bluepy import btle
from bluepy.btle import BTLEException
from badge import Badge, BadgeDelegate
from badge_db import badgeDB
from badge_discoverer import BadgeDiscoverer

import datetime
import struct
import logging
import logging.handlers
import os
import re
import subprocess
import shlex
import sys
import time
import csv

import random

from functools import wraps
import errno
import signal


log_file_name = 'out.log'
scans_file_name = 'rssi_scan.txt'

WAIT_FOR = 1.0  # timeout for WaitForNotification calls.  Must be > samplePeriod of badge
PULL_WAIT = 2
SCAN_DURATION = 3 # seconds
# create logger with 'badge_server'
logger = logging.getLogger('badge_server')
logger.setLevel(logging.DEBUG)

# create file handler which logs even debug messages
fh = logging.FileHandler(log_file_name)
fh.setLevel(logging.DEBUG)

# create console handler with a higher log level
ch = logging.StreamHandler()
ch.setLevel(logging.DEBUG)

# create formatter and add it to the handlers
#formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(mac)s %(message)s')
formatter = logging.Formatter('%(asctime)s - %(levelname)s - [%(mac)s] %(message)s')
fh.setFormatter(formatter)
ch.setFormatter(formatter)

# add the handlers to the logger
logger.addHandler(fh)
logger.addHandler(ch)

# global logging variable
mac = None

# adding a context filter that adds the MAC to the log
class ContextFilter(logging.Filter):
	def filter(self,record):
		record.mac=mac
		return True

f = ContextFilter()
logger.addFilter(f)

# Raises a timeout exception if a function takes too long
# http://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
class TimeoutError(Exception):
	pass

# Or, to use with a "with" statement
class timeout:
	def __init__(self, seconds=1, error_message='Timeout'):
		self.seconds = seconds
		self.error_message = error_message
	def handle_timeout(self, signum, frame):
		raise TimeoutError(self.error_message)
	def __enter__(self):
		signal.signal(signal.SIGALRM, self.handle_timeout)
		signal.alarm(self.seconds)
	def __exit__(self, type, value, traceback):
		signal.alarm(0)

'''
Returns a list of devices included in device_macs.txt 
Format is device_mac<space>device_name
'''
def get_devices(device_file="device_macs.txt"):
	if not os.path.isfile(device_file):
		logger.error("Cannot find devices file: {}".format(device_file))
		exit(1)
	logger.info("Reading whitelist:")
	devices=[]

	with open(device_file, 'r') as csvfile:
		fil = filter(lambda row: row[0]!='#', csvfile)
		fil = filter(lambda x: not re.match(r'^\s*$', x), fil)
		rdr = csv.reader(fil, delimiter=' ')
		for row in rdr:
			device = row[0]
			devices.append(device)

		csvfile.close()

	for device in devices:
		logger.info("    {}".format(device))
	return devices

'''
Attempts to read data from the device specified by the address. Reading is handled by gatttool.
'''
lastScanTimestamp = datetime.datetime.fromtimestamp(1471539036)

def dialogue(addr=""):
	global lastScanTimestamp

	logger.info("Connecting to {}".format(addr))
	retcode = 0
	bdg = None
	try:
		with timeout(seconds=5, error_message="Connect timeout"):
			bdg = Badge(addr)

		logger.info("Connected")

		with timeout(seconds=5, error_message="Dialogue timeout (wrong firmware version?)"):
			while not bdg.dlg.gotStatus:
				bdg.sendStatusRequest()  # ask for status
				bdg.waitForNotifications(WAIT_FOR)  # waiting for status report
			
			logger.info("Got status")
						
			#while not bdg.dlg.gotDateTime:
			#	bdg.NrfReadWrite.write("t")  # ask for time
			#	bdg.waitForNotifications(WAIT_FOR)

			logger.info("Badge datetime: {},{}, Voltage: {}".format(bdg.dlg.timestamp_sec,bdg.dlg.timestamp_ms,bdg.dlg.voltage))

		
		# data request using the "r" command - data since time X
		logger.info("Requesting data...")
		lastChunkDate = None
		with badgeDB() as db:
			lastChunkDate = db.getLastChunkDate(addr)

		if (lastChunkDate == None):
			lastChunkDate = datetime.datetime.now()
			logger.info("Cannot find saved chunks. Setting last chunk date to: {}".format(lastChunkDate))
		else:
			logger.info("Last chunk date: {}".format(lastChunkDate))

		bdg.sendDataRequest(lastChunkDate) # ask for data
		wait_count = 0
		while True:
			if bdg.dlg.gotEndOfData == True:
				break
			if bdg.waitForNotifications(WAIT_FOR):
				# if got data, don't inrease the wait counter
				continue
			logger.info("Waiting for more data...")
			wait_count = wait_count+1
			if wait_count >= PULL_WAIT: break
		logger.info("finished reading data")
		
		
		# data request using the "r" command - data since time X
		logger.info("Requesting scans...")


		bdg.sendScanRequest(lastScanTimestamp)

		#rand = random.randint(0,300)
		#bdg.sendScanRequest(lastScanTimestamp-datetime.timedelta(seconds=rand)) # ask for data
		
		
		wait_count = 0
		while True:
			if bdg.dlg.gotEndOfScans == True:
				break
			if bdg.waitForNotifications(WAIT_FOR):
				# if got data, don't inrease the wait counter
				continue
			logger.info("Waiting for more data...")
			wait_count = wait_count+1
			if wait_count >= PULL_WAIT: break
		logger.info("finished reading data")
		
		#time.sleep(60)
		

	except BTLEException, e:
		retcode=-1
		logger.error("failed pulling data")
		logger.error(e.code)
		logger.error(e.message)
	except TimeoutError, te:
		retcode=-1
		logger.error("TimeoutError: "+te.message)
	except Exception as e:
		#retcode=-1
		#e = sys.exc_info()[0]
		logger.error("unexpected failure, {}".format(e))
	finally:
		if bdg:
			bdg.disconnect()
			logger.info("disconnected")

			if bdg.dlg.chunks: 
				logger.info("Chunks received: {}".format(len(bdg.dlg.chunks)))
				logger.info("saving chunks to file")
				outfile = addr.replace(":","_") + ".scn"
				i = 0

				# store in CSV file
				fout = open(outfile, "a")
				for chunk in bdg.dlg.chunks:
					logger.info("CSV: Chunk timestamp: {}, Voltage: {}, Delay: {}, Samples in chunk: {}".format(chunk.ts,chunk.voltage,chunk.sampleDelay,len(chunk.samples)))
					fout.write("{},{},{}".format(chunk.ts,chunk.voltage,chunk.sampleDelay))
					for sample in chunk.samples:
						fout.write(",{}".format(sample))
		
					fout.write("\n")
				fout.close()

				# store in DB
				with badgeDB() as db:
					for chunk in bdg.dlg.chunks:
						logger.info("DB: Chunk timestamp: {}, Voltage: {}, Delay: {}, Samples in chunk: {}".format(chunk.ts,chunk.voltage,chunk.sampleDelay,len(chunk.samples)))
						db.insertChunk(addr,chunk.ts,chunk.voltage)
						db.insertSamples(addr,chunk)
						#for sample in chunk.samples:
						#	fout.write(",{}".format(sample))			

				logger.info("done writing")
			
			else:
				logger.info("No mic data ready")
			
			if bdg.dlg.scans:
				for scan in bdg.dlg.scans:
					logger.info("SCAN: scan timestamp: {}, number: {}".format(scan.ts,scan.numDevices))
					if scan.devices:
						deviceList = ''
						for dev in scan.devices:
							deviceList += "[#{:x},{},{}]".format(dev.ID,dev.rssi,dev.count)
						logger.info('  >  ' + deviceList)
					#lastScanTimestamp = bdg.dlg.scans[-1].ts
			
			else:
				logger.info("No scans ready")

	return retcode
	
'''
Attempts to get status of badge specified by address. Reading is handled by gatttool.
'''
def getStatus(addr=""):
	logger.info("Connecting to {}".format(addr))
	retcode = 0
	bdg = None
	try:
		with timeout(seconds=5, error_message="Connect timeout"):
			bdg = Badge(addr)

		logger.info("Connected")

		with timeout(seconds=5, error_message="Status request timeout (wrong firmware version?)"):
			while not bdg.dlg.gotStatus:
				bdg.sendStatusRequest()  # ask for status
				bdg.waitForNotifications(WAIT_FOR)  # waiting for status report
			
			logger.info("Got status")

			bdg.sendIdentifyReq(10)

			if bdg.dlg.timestamp_sec != 0:
				logger.info("Badge datetime: {},{}".format(bdg.dlg.timestamp_sec,bdg.dlg.timestamp_ms))
			else:
				logger.info("Badge previously unsynced.")

	except BTLEException, e:
		retcode=-1
		logger.error("failed pulling data")
		logger.error(e.code)
		logger.error(e.message)
	except TimeoutError, te:
		retcode=-1
		logger.error("TimeoutError: "+te.message)
	except Exception as e:
		#retcode=-1
		#e = sys.exc_info()[0]
		logger.error("unexpected failure, {}".format(e))
	finally:
		if bdg:
			bdg.disconnect()
			logger.info("disconnected")

	return retcode
	
'''
Attempts to start badge recording.
'''
def startRec(addr=""):
	logger.info("Connecting to {}".format(addr))
	retcode = 0
	bdg = None
	try:
		with timeout(seconds=5, error_message="Connect timeout"):
			bdg = Badge(addr)

		logger.info("Connected")

		with timeout(seconds=5, error_message="StartRec timeout (wrong firmware version?)"):
			while not bdg.dlg.gotTimestamp:
				bdg.sendStartRecRequest(10)  # start recording
				#bdg.sendStartScanRequest(10,0,0,0,0)  # start recording
				bdg.waitForNotifications(WAIT_FOR)  # waiting for time acknowledgement
			
			logger.info("Got time ack")
						

			if bdg.dlg.timestamp_sec != 0:
				logger.info("Badge datetime: {},{}".format(bdg.dlg.timestamp_sec,bdg.dlg.timestamp_ms))
			else:
				logger.info("Badge previously unsynced.")
		
		bdg.dlg.gotTimestamp = False
				
		with timeout(seconds=5, error_message="StartScan timeout (wrong firmware version?)"):
			while not bdg.dlg.gotTimestamp:
				#bdg.sendStartRecRequest(10)  # start recording
				bdg.sendStartScanRequest(10,0,0,0,0)  # start recording
				bdg.waitForNotifications(WAIT_FOR)  # waiting for time acknowledgement
			
			logger.info("Got time ack")
						

			if bdg.dlg.timestamp_sec != 0:
				logger.info("Badge datetime: {},{}".format(bdg.dlg.timestamp_sec,bdg.dlg.timestamp_ms))
			else:
				logger.info("Badge previously unsynced.")

	except BTLEException, e:
		retcode=-1
		logger.error("failed starting recording")
		logger.error(e.code)
		logger.error(e.message)
	except TimeoutError, te:
		retcode=-1
		logger.error("TimeoutError: "+te.message)
	except Exception as e:
		#retcode=-1
		#e = sys.exc_info()[0]
		logger.error("unexpected failure, {}".format(e))
	finally:
		if bdg:
			bdg.disconnect()
			logger.info("disconnected")

	return retcode

'''
def send_time(addr=""):
	logger.info("Sending date to {}".format(addr))
	retcode = 0
	bdg = None
	try:
		bdg = Badge(addr)
		logger.info("Connected")
		bdg.sendDateTime()
		logger.info("Sent date")
	except BTLEException, e:
		retcode=-1
		logger.error("failed sending time,{},{}".format(e.code,e.message))
	finally:
		if bdg:
			bdg.disconnect()
			logger.info("disconnected")
			del bdg
	return retcode
'''

def scan_for_devices(devices_whitelist):
	bd = BadgeDiscoverer()
	try:
		all_devices = bd.discover(scanDuration=SCAN_DURATION)
	except: # catch *all* exceptions
		e = sys.exc_info()[0]
		logger.error("Scan failed: {}".format(e))
		all_devices = {}

	scanned_devices = []
	for addr,device_info in all_devices.iteritems():
		if addr in devices_whitelist:
			logger.debug("Found {}, added".format(addr))
			scanned_devices.append({'mac':addr,'device_info':device_info})
		else:
			logger.debug("Found {}, but not on whitelist".format(addr))
	return scanned_devices

def reset():
	reset_command = "hciconfig hci0 reset"
	args = shlex.split(reset_command)
	p = subprocess.Popen(args)

def add_pull_command_options(subparsers):
	pull_parser = subparsers.add_parser('pull', help='Continuously pull data from badges')
	pull_parser.add_argument('-w','--use_whitelist', action='store_true', default=False, help="Use whitelist instead of continuously scanning for badges")

def add_scan_command_options(subparsers):
	pull_parser = subparsers.add_parser('scan', help='Continuously scan for badges')
	
def add_sync_all_command_options(subparsers):
	sa_parser = subparsers.add_parser('sync_all', help='Send date to all devices in whitelist')
	sa_parser.add_argument('-w','--use_whitelist', action='store_true', default=False, help="Use whitelist instead of continuously scanning for badges")

def add_start_all_command_options(subparsers):
	st_parser = subparsers.add_parser('start_all', help='Start recording on all devices in whitelist')
	st_parser.add_argument('-w','--use_whitelist', action='store_true', default=False, help="Use whitelist instead of continuously scanning for badges")


def add_sync_device_command_options(subparsers):
	sd_parser = subparsers.add_parser('sync_device', help='Send date to a given device')
	sd_parser.add_argument('-d',
		'--device',
		required=True,
		action='store',
		dest='device',
		help='device to sync')
	
if __name__ == "__main__":
	import time
	import sys
	import argparse

	parser = argparse.ArgumentParser(description="Run scans, send dates, or continuously pull data")
	parser.add_argument('-dr','--disable_reset_ble', action='store_true', default=False, help="Do not reset BLE")
	
	subparsers = parser.add_subparsers(help='Program mode (e.g. Scan, send dates, pull, scan etc.)', dest='mode')
	add_pull_command_options(subparsers)
	add_scan_command_options(subparsers)	
	add_sync_all_command_options(subparsers)
	add_start_all_command_options(subparsers)
	add_sync_device_command_options(subparsers)

	args = parser.parse_args()

	if not args.disable_reset_ble:
		logger.info("Resetting BLE")
		reset()
		time.sleep(2)  # requires sleep after reset
		logger.info("Done resetting BLE")
	
	if args.mode == "sync_device":
		status = send_time(args.device)
		if status != 0:
			logger.error("error sending time to {}".format(args.device))
		
		exit(0)

	if args.mode == "sync_all":  
	    logger.info('Syncing all badges.')
        whitelist_devices = get_devices()
        synced_devices = []
        unsynced_devices = []
        if not args.use_whitelist:
            logger.info("Scanning for devices...")
            scanned_devices = scan_for_devices(whitelist_devices)
            logger.info("Found: {} devices".format(len(scanned_devices)))
            synced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==True]
            unsynced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==False]
        else:
            logger.info("Scan is disabled. Using whitelist.")
            synced_devices = [{'mac':x} for x in whitelist_devices]

        time.sleep(2) 

        if len(unsynced_devices)>0:
            logger.info("Sending dates to unsynced devices...")
            for device in unsynced_devices:
                mac=device['mac']
                status = send_time(mac)
                if status != 0:
                    logger.error("error sending time to {}".format(mac))

                time.sleep(2)  # requires sleep between devices
                mac=None
        if len(synced_devices)>0:            
            logger.info("Communicating with synced devices...")
            for device in synced_devices:
                mac=device['mac']
                getStatus(mac)
                time.sleep(2)  # requires sleep between devices
                mac=None
					
		time.sleep(2) # allow BLE time to disconnect 
	
	if args.mode == "start_all":  
	    logger.info('Starting all badges recording.')
        whitelist_devices = get_devices()
        synced_devices = []
        unsynced_devices = []
        if not args.use_whitelist:
            logger.info("Scanning for devices...")
            scanned_devices = scan_for_devices(whitelist_devices)
            logger.info("Found: {} devices".format(len(scanned_devices)))
            synced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==True]
            unsynced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==False]
        else:
            logger.info("Scan is disabled. Using whitelist.")
            synced_devices = [{'mac':x} for x in whitelist_devices]

        time.sleep(2) 

        if len(unsynced_devices)>0:
            logger.info("Sending startRec to unsynced devices...")
            for device in unsynced_devices:
                mac=device['mac']
                status = send_time(mac)
                if status != 0:
                    logger.error("error sending time to {}".format(mac))

                time.sleep(2)  # requires sleep between devices
                mac=None
        if len(synced_devices)>0:            
            logger.info("Sending startRec to alldevices...")
            for device in synced_devices:
                mac=device['mac']
                startRec(mac)
                time.sleep(2)  # requires sleep between devices
                mac=None
					
		time.sleep(2) # allow BLE time to disconnect 

	# scan for devices
	if args.mode == "scan":
		logger.info('Scanning for badges')
		while True:
			whitelist_devices = get_devices()
			logger.info("Scanning for devices...")
			scanned_devices = scan_for_devices(whitelist_devices)
			fout = open(scans_file_name, "a")			
			for device in scanned_devices:
					mac=device['mac']
					scan_date=device['device_info']['scan_date']
					rssi=device['device_info']['rssi']
					logger.debug("{},{},{:.2f}".format(scan_date,mac,rssi))
					fout.write("{},{},{:.2f}\n".format(scan_date,mac,rssi))
			fout.close()
		
	# pull data from all devices
	if args.mode == "pull":
		logger.info('Started')
		while True:
		
			whitelist_devices = get_devices()
			scanned_devices = []
			
			if not args.use_whitelist:
				logger.info("Scanning for devices...")
				scanned_devices = scan_for_devices(whitelist_devices)
				logger.info("Found: {} devices".format(len(scanned_devices)))
			else:
				logger.info("Scan is disabled. Using whitelist.")
				scanned_devices = [{'mac':x} for x in whitelist_devices]

			time.sleep(0.5) 
			
			if len(scanned_devices)>0:            
				logger.info("Communicating with devices...")
				for device in scanned_devices:
					mac=device['mac']
					dialogue(mac)
					time.sleep(0.5)  # requires sleep between devices
					mac=None

			logger.info("Sleeping...")
			time.sleep(10);
			
			'''
			whitelist_devices = get_devices()
			synced_devices = []
			unsynced_devices = []
			if not args.use_whitelist:
				logger.info("Scanning for devices...")
				scanned_devices = scan_for_devices(whitelist_devices)
				logger.info("Found: {} devices".format(len(scanned_devices)))
				synced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==True]
				unsynced_devices = [x for x in scanned_devices if x['device_info']['is_sync']==False]
			else:
				logger.info("Scan is disabled. Using whitelist.")
				synced_devices = [{'mac':x} for x in whitelist_devices]

			time.sleep(1) 
			
			if len(unsynced_devices)>0:
				logger.info("Sending dates to unsynced devices...")
				for device in unsynced_devices:
					mac=device['mac']
					status = send_time(mac)
					if status != 0:
						logger.error("error sending time to {}".format(mac))

					time.sleep(1)  # requires sleep between devices
					mac=None
			if len(synced_devices)>0:            
				logger.info("Communicating with synced devices...")
				for device in synced_devices:
					mac=device['mac']
					dialogue(mac)
					time.sleep(1)  # requires sleep between devices
					mac=None

			logger.info("Sleeping...")
			time.sleep(10);
			'''

exit(0)

# BLE scanner, based on https://code.google.com/p/pybluez/source/browse/trunk/examples/advanced/inquiry-with-rssi.py
# and on https://github.com/adamf/BLE/blob/master/ble-scanner.py

# hcitool.c for lescan
# hci.h for opcodes
# hci.c for functions used by lescan

import binascii
import os
import sys
import struct
import bluetooth._bluetooth as bluez

DEBUG = False

class BleScanner:
	LE_META_EVENT = 0x3e
	LE_PUBLIC_ADDRESS=0x00
	LE_RANDOM_ADDRESS=0x01
	LE_SET_SCAN_PARAMETERS_CP_SIZE=7
	OGF_LE_CTL=0x08
	OCF_LE_SET_SCAN_PARAMETERS=0x000B
	OCF_LE_SET_SCAN_ENABLE=0x000C
	OCF_LE_CREATE_CONN=0x000D

	# these are actually subevents of LE_META_EVENT
	EVT_LE_CONN_COMPLETE=0x01
	EVT_LE_ADVERTISING_REPORT=0x02
	EVT_LE_CONN_UPDATE_COMPLETE=0x03
	EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE=0x04

	# name types
	EIR_NAME_SHORT=0x08  	# shortened local name
	EIR_NAME_COMPLETE=0x09	# complete local name

	def __init__(self):
		dev_id = 0
		try:
			self.sock = bluez.hci_open_dev(dev_id)
		except:
			print "error accessing bluetooth device..."
			sys.exit(1)
	
	def packed_bdaddr_to_string(self,bdaddr_packed):
		return ':'.join('%02x'%i for i in struct.unpack("<BBBBBB", bdaddr_packed[::-1]))

	def hci_enable_le_scan(self):
		# hci_le_set_scan_enable(dd, 0x01, filter_dup, 1000);
		self.hci_toggle_le_scan(0x01)

	def hci_disable_le_scan(self):
		self.hci_toggle_le_scan(0x00)

	def hci_toggle_le_scan(self,enable):
		if DEBUG: print "toggle scan: ", enable
		cmd_pkt = struct.pack("<BB", enable, 0x00) #0x00 seems to be the filter_dup flag. 00 = allow duplicates
		bluez.hci_send_cmd(self.sock, self.OGF_LE_CTL, self.OCF_LE_SET_SCAN_ENABLE, cmd_pkt)
		if DEBUG: print "sent toggle enable"


	# For reference, see cmd_lescan in hcitool.c
	def hci_le_set_scan_parameters(self):
		if DEBUG: print "setting up scan"
		old_filter = self.sock.getsockopt( bluez.SOL_HCI, bluez.HCI_FILTER, 14)
		if DEBUG: print "got old filter"

		OWN_TYPE = 0x00 #SCAN_PUBLIC. Random doesn't seem to work
		SCAN_TYPE = 0x01 # Active

		INTERVAL = 0x10
		WINDOW = 0x10
		FILTER = 0x00 # 
		
		# interval and window are uint_16, so we pad them with 0x0
		cmd_pkt = struct.pack("<BBBBBBB", SCAN_TYPE, 0x0, INTERVAL, 0x0, WINDOW, OWN_TYPE, FILTER)
		if DEBUG: print "packed up: ", cmd_pkt
		if DEBUG: print repr(cmd_pkt)
		
		#r = bluez.hci_send_cmd(sock, OGF_LE_CTL, OCF_LE_SET_SCAN_PARAMETERS, cmd_pkt)
		r = bluez.hci_send_req(self.sock, self.OGF_LE_CTL, self.OCF_LE_SET_SCAN_PARAMETERS,bluez.EVT_CMD_COMPLETE,self.LE_SET_SCAN_PARAMETERS_CP_SIZE, cmd_pkt)	
		if DEBUG: print "status for set scan: {}".format(r)
		if DEBUG: print "sent scan parameters command"


	# parse events until it finds an advertisement event
	# returns null if it couldn't find anyhting
	def parse_events(self,max_attempts=100):
		old_filter = self.sock.getsockopt( bluez.SOL_HCI, bluez.HCI_FILTER, 14)

		# Setting up filter
		flt = bluez.hci_filter_new()
		#bluez.hci_filter_all_events(flt)
		# we are only interested in LE_META_EVENT
		bluez.hci_filter_set_event(flt,self.LE_META_EVENT);
		bluez.hci_filter_set_ptype(flt, bluez.HCI_EVENT_PKT)
		self.sock.setsockopt( bluez.SOL_HCI, bluez.HCI_FILTER, flt )
		
		event_info = None
		for i in range(0, max_attempts):
			pkt = self.sock.recv(255)
			ptype, event, plen = struct.unpack("BBB", pkt[:3])
			if DEBUG: print "-------------- ptype: 0x%02x event: 0x%02x plen: 0x%02x" % (ptype, event, plen)
			if event == self.LE_META_EVENT:
				subevent, = struct.unpack("B", pkt[3]) 	#sub event is the 4th byte of the complete package
				pkt = pkt[4:]							
				if DEBUG: print "LE META EVENT subevent: 0x%02x" %(subevent,)
				if subevent == self.EVT_LE_ADVERTISING_REPORT:
					event_info = {}
					# data starts from the 5th byte. There will be a byte indicating the number of reports, and 
					# then a EVT_LE_ADVERTISING_REPORT structure for each report. We only take the first report
					if DEBUG: print "advertising report"
					num_reports = struct.unpack("B", pkt[0])[0]

					# report (ignore other reports if there are more than 1)
					rpt = pkt[1:]
					event_info["data"]=rpt
					report_event_type = struct.unpack("B", rpt[0])[0]
					bdaddr_type = struct.unpack("B", rpt[1])[0]
					event_info["mac"]=self.packed_bdaddr_to_string(rpt[2:8]).upper()
					report_data_length, = struct.unpack("B", rpt[8])
					if DEBUG: print "\tadvertising report event type: 0x%02x" % report_event_type
					if DEBUG: print "\tbdaddr type: 0x%02x" % (bdaddr_type,)
					if DEBUG: print "\tadvertising packet metadata length: ", report_data_length

					rssi, = struct.unpack("b", pkt[-1])
					event_info["rssi"] = rssi
					
					name = None
					if report_data_length > 0:
						rpt_data = rpt[9:]
					
						# Look for a name
						name = self.parse_name(rpt_data)
					event_info["name"]=name
					#print "\tName:", name
					if DEBUG: print repr(event_info)
		
					break
				if subevent == self.EVT_LE_CONN_COMPLETE:
					if DEBUG: print "connection complete"
					break
				elif subevent == self.EVT_LE_CONN_UPDATE_COMPLETE:
					if DEBUG: print "connection updated"
					break
				elif subevent == self.EVT_LE_READ_REMOTE_USED_FEATURES_COMPLETE:
					if DEBUG: print "read remote used features complete"
					break
				else:
					if DEBUG: print "unknown LE_META_EVENT subevent"
					break
			else:
				if DEBUG: print "unsupported packet, event 0x%02x " % event
				if DEBUG: print "unsupported packet type 0x%02x" % ptype
				continue # try again

		self.sock.setsockopt( bluez.SOL_HCI, bluez.HCI_FILTER, old_filter )
		return event_info
		
	# see static void eir_parse_name in hcitool.c
	# this method will try to extract the name only from the first segment
	def parse_name(self,rpt_data):
		offset = 0
		len_rpt_data=len(rpt_data)

		field_len = struct.unpack("B", rpt_data[0])[0]

		if field_len == 0:
			return None;

		if field_len > len_rpt_data -1:
			return None
						
		name_type = struct.unpack("B", rpt_data[1])[0]
		#print "Name type: {0:d},0x{0:x}".format(name_type)
			
		if name_type == self.EIR_NAME_COMPLETE:
			name = rpt_data[2:field_len+1]
			#print "Name: --{}--,{}".format(name,binascii.hexlify(name))
			return name
			
		return None

if __name__ == "__main__":
	bleScanner = BleScanner()
	# stop if there is any running scan
	bleScanner.hci_disable_le_scan()
	
	bleScanner.hci_le_set_scan_parameters()
		
	bleScanner.hci_enable_le_scan()
	
	#for cnt in range(0,10):
	while(1):
		device = bleScanner.parse_events()
		if device is not None:
			print "{}|{}|{}|{}".format(device['name'],device['mac'],device['rssi'],repr(device['data']))
		else:
			print "Scan completed, quitting"
			break
	
	# stop scan when done
	bleScanner.hci_disable_le_scan()
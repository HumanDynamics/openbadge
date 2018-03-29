# log_to_csv.py
#
# @author Zachary Pitcher
# @version 2/19/2018
#
# ***Only guaranteed to work with serial_test.py version matching date above***
#
# convert a LOG file output from serial_test.py as a readable CSV file for PANDAS
# since there may be multiple tests for a given LOG file, 
# creates a folder with all tests in separate output CSV files

import sys, os
import datetime

if __name__ == "__main__":
	args = sys.argv[1:]
	if type(args) != list:
		print "specify a logfile to convert"
	
	filename = args[0]
	if len(args) == 3:
		dirname = args[1]
	else:
		dirname = filename[:filename.rindex('.')] + "_CSV/"
		
	if not os.path.exists(dirname):
		os.makedirs(dirname)
	
	with open(filename, "r") as logfile:
		numfiles = -1
		numlines = 0
		outfile = None
		raw = logfile.readline()

		while raw != "":
			if "end" in raw:
				raw = logfile.readline()
				if raw == "":
					break
			if "----" in raw:
				if outfile:
					outfile.close()
				numfiles += 1
				if numfiles > 999:
					print "Too many logs in one file. Stopping at line", str(numlines)
					if logfile:
						logfile.close()
				outfile = open(dirname + str("%03d" % numfiles) + ".csv", "w")
				outfile.write("DATETIME,MAC,RSSI\n")
				raw = logfile.readline()
				raw = logfile.readline()
			
			try:
				_sec, _mac, _rssi = raw.split(",")
				sec = int(_sec[:-4])
				date = '\"' + datetime.datetime.fromtimestamp(int(sec)).strftime('%Y-%m-%d %H:%M:%S') + _sec[-4:] + '\"'
				mac = '\"' + _mac + '\"'
				rssi = int(_rssi)
				numlines += 1
			except ValueError:
				print "Format error in input logfile at line", numlines
				if outfile:
					outfile.close()
				if logfile:
					logfile.close()
				sys.exit(1)
				
			outfile.write(date + "," + mac + "," + str(rssi) + "\n")	
			raw = logfile.readline()
		if outfile:
			outfile.close()
		if logfile:
			logfile.close()
		numfiles += 1
	
	print "Converted", numfiles, "files in", dirname

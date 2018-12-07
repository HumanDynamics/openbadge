from __future__ import division, absolute_import, print_function
import sys
import threading
import datetime
import random
import time

from badge import *
from ble_badge_connection import *
from bluepy import *
from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers ,Scanner
from bluepy.btle import BTLEException

import matplotlib
matplotlib.use("TkAgg")
from matplotlib.backends.backend_tkagg import (FigureCanvasTkAgg, NavigationToolbar2TkAgg)
from matplotlib.figure import Figure
from Tkinter import Tk, Label, Button, Entry, LEFT, BOTH
import numpy as np

# It is important to enable the access to the host-display with "xhost +" before starting the docker-container
# You have to configure the ID's and group number of the badges via the terminal correctly to "find" them in a scan --> same group number
# Please enter the MAC-Address of the Badge during the call of this python script


MAC_ADDRESS = ""
DISPLAY_COUNTER_MIC 	= 200
DISPLAY_TIME_MIC_SEC	= 10
DISPLAY_COUNTER_ACC 	= 200
DISPLAY_TIME_ACC_SEC    = 10





t_mic = []
y_mic = []
t_acc = []
y_acc = []
y_scan = []
scan_alpha = 0.2

start_time = time.time()

scan_color_map=['r', 'g', 'b', 'cyan', 'magenta']


def get_distance(rssi):
	n = 2
	p_ref = -65
	d = 10**((p_ref - rssi)/(10*n))

	return d
	
	

class StreamThread(threading.Thread):
	def __init__(self): 
		threading.Thread.__init__(self) 
		self.running = False
	
		
	
	def run(self): 
		global t_mic
		global y_mic
		global t_acc
		global y_acc
		global y_scan
		
		self.running = True
		self.connect()
		while(self.running):
			try:
				tmp = self.badge.get_stream()
				while(not (tmp == [])):
					#print(tmp)
					if(not (tmp.microphone_stream == [])):
						mic = tmp.microphone_stream[0].microphone_data.value
						t_mic.append(time.time() - start_time)
						y_mic.append(mic)
						
					if(not (tmp.accelerometer_stream == [])):
						acc = tmp.accelerometer_stream[0].accelerometer_raw_data.raw_acceleration
						t_acc.append(time.time() - start_time)
						y_acc.append(acc)
						
			
					
					if(not (tmp.scan_stream == [])):
						
						scan = tmp.scan_stream[0].scan_device
						ID = scan.ID
						RSSI = scan.rssi
						IDs =  [row[0] for row in y_scan]
						idx = []
						try:
							idx = IDs.index(ID)
						except Exception as e:
							print(e)
							
						
						d = get_distance(RSSI)
							
							
						if(idx == []):
							y_scan.append([ID, d])
							# sort ID
							y_scan = sorted(y_scan, key=lambda x: x[0])
							
						else:
							former_d = y_scan[idx][1]
							new_d = scan_alpha*d + (1-scan_alpha)* former_d
							y_scan[idx]= [ID, new_d]
							
						print(y_scan)
						
						#t_scan.append(time.time() - start_time)
						# sort to greatest
						#y_scan.append(scan)
						
					
					
					# TODO: Add here the other data-source (accel...)
					tmp = self.badge.get_stream()
			except Exception as e:
				print(e)			
				self.connection.disconnect()
				self.connect()
				
		self.connection.disconnect()	
	
	def stop(self):
		self.running = False
				
	def connect(self):
		print("Connecting to badge", MAC_ADDRESS)	
		self.connection = BLEBadgeConnection.get_connection_to_badge(MAC_ADDRESS)
		self.connection.connect()
		self.badge = OpenBadge(self.connection)
		print("Connected!")		
		print(self.badge.get_status())
		print("Start streaming..")
		self.badge.start_microphone_stream(sampling_period_ms=50)
		
		self.badge.start_accelerometer_stream(timeout_minutes=0, operating_mode=2, full_scale=4, datarate=25, fifo_sampling_period_ms=50)
		self.badge.start_scan_stream(timeout_minutes=0, window_ms=100, interval_ms=300,duration_seconds=5, period_seconds=6, aggregation_type=0)
		
		############ START other Datasources #############
		
		self.clear()
		
	def clear(self):
		global t_mic
		global y_mic
		global t_acc
		global y_acc
		global y_scan
		print("Clear stream queue")
		self.badge.stream_clear()
		print("Cleared stream queue")
		t_mic = []
		y_mic = []
		t_acc = []
		y_acc = []
		y_scan = []


class App:
    def __init__(self):		
		self.stream_thread_running = 0		
		self.plot_thread_running = 0
		
		self.f = Figure(figsize=(5,4), dpi=100)
		self.axis_mic = self.f.add_subplot(131)
		self.axis_mic.set_ylim(0,100)
		self.axis_mic.grid(b=True)
		
		self.axis_acc = self.f.add_subplot(132)
		self.axis_acc.set_ylim(-500,500)
		self.axis_acc.grid(b=True)
		
		
		self.axis_scan = self.f.add_subplot(133)
		self.axis_scan.set_ylim(0,4)
		self.axis_scan.grid(b=True)		
		

		
		########### Add other axis ##########
		
		self.window = Tk()
		self.canvas = FigureCanvasTkAgg(self.f, master = self.window)
		self.canvas.draw()
		self.canvas.get_tk_widget().pack(fill=BOTH, expand=True)
		
		
		
		self.start_button = Button(master=self.window, text="Start", command = self.start_button_pressed)
		self.start_button.pack(side=LEFT)
		self.stop_button = Button(master=self.window, text="Stop", command = self.stop_button_pressed)
		self.stop_button.pack(side=LEFT)
		self.window.mainloop()
		
		
    def reset(self):
		global t_mic
		global y_mic
		global t_acc
		global y_acc
		global y_scan
		
		t_mic = []
		y_mic = []
		t_acc = []
		y_acc = []
		t_acc_int = []
		y_acc_int = []
		t_bat = []
		y_bat = []
		t_scan = []
		y_scan = []
	

    def start_button_pressed(self):
		self.reset()
		global start_time
		start_time = time.time()
		
		if(not self.plot_thread_running):
			self.plot_thread_running = 1
			self.plot_thread = PlotThread(self.canvas, self.axis_mic, self.axis_acc, self.axis_scan)
			self.plot_thread.start()
	
		
		if(not self.stream_thread_running):	
			self.stream_thread_running = 1
			self.stream_thread = StreamThread()
			self.stream_thread.start()
			
    def stop_button_pressed(self):
		self.reset()
		
		if(self.plot_thread_running):
			self.plot_thread_running = 0
			self.plot_thread.stop()
		
		if(self.stream_thread_running):	
			self.stream_thread_running = 0
			self.stream_thread.stop()
		
		
class PlotThread(threading.Thread):
	def __init__(self, canvas, axis_mic, axis_acc, axis_scan): 
		threading.Thread.__init__(self) 
		self.canvas = canvas
		self.axis_mic = axis_mic
		self.axis_acc = axis_acc
		self.axis_scan = axis_scan
		self.running = False
		
	def run(self):
		
		global t_mic
		global y_mic
		global t_acc
		global y_acc
		global y_scan
		self.running = True
		while(self.running):
			################# MIC ###################		
			x_tmp = []
			y_tmp = []
			start_index, end_index =  self.get_list_indizes(t_mic, DISPLAY_COUNTER_MIC)
				
			x_tmp = t_mic[start_index:end_index]
			y_tmp = y_mic[start_index:end_index]
			t_mic = t_mic[start_index:]
			y_mic = y_mic[start_index:]
			
			
			self.axis_mic.clear()
			self.axis_mic.plot(x_tmp, y_tmp, "-", label='Mic')
			#self.axis_mic.legend(loc="upper right")
			y_lim_min, y_lim_max = self.get_y_lim(y_tmp, 0, 90)
			self.axis_mic.set_ylim(y_lim_min, y_lim_max + 10)
			x_lim_min, x_lim_max = self.get_x_lim_time(DISPLAY_TIME_MIC_SEC, start_time)
			self.axis_mic.set_xlim(x_lim_min, x_lim_max)
			self.axis_mic.grid(b=True)
			self.axis_mic.set_title("Microphone")
			self.axis_mic.set_xlabel("Time (s)")
			
			
			##### ACC #####			
			x_tmp = []
			y_tmp = []
			start_index, end_index =  self.get_list_indizes(t_acc, DISPLAY_COUNTER_ACC)
				
			x_tmp = t_acc[start_index:end_index]
			y_tmp = y_acc[start_index:end_index]
			t_acc = t_acc[start_index:]
			y_acc = y_acc[start_index:]
			
			
			self.axis_acc.clear()
			self.axis_acc.plot(x_tmp, [row[0] for row in y_tmp] , "-", label='x')
			self.axis_acc.plot(x_tmp, [row[1] for row in y_tmp], "-", label='y')
			self.axis_acc.plot(x_tmp, [row[2] for row in y_tmp], "-", label='z')
			self.axis_acc.legend(loc="upper right")
			y_lim_min, y_lim_max = self.get_y_lim(y_tmp, -750, 750)
			self.axis_acc.set_ylim(y_lim_min - 10, y_lim_max + 10)
			x_lim_min, x_lim_max = self.get_x_lim_time(DISPLAY_TIME_ACC_SEC, start_time)
			self.axis_acc.set_xlim(x_lim_min, x_lim_max)
			self.axis_acc.grid(b=True)		
			self.axis_acc.set_title("Acceleration (mg)")
			self.axis_acc.set_xlabel("Time (s)")
			
			
			##### Scan #####
			x_label = []
			x_pos = []
			y = []
			colors = []
			for i in range(0, len(y_scan)):
				colors.append(scan_color_map[i%len(scan_color_map)])				
				x_label.append("ID: " + str(y_scan[i][0]))
				y.append(y_scan[i][1])
				x_pos.append(i)
				
				
			barwidth = 0.5
			self.axis_scan.clear()
			self.axis_scan.set_xticks(x_pos)
			self.axis_scan.set_xticklabels(x_label)
			self.axis_scan.bar(x_pos, y, barwidth, align='center', alpha=0.5, color = colors)
			self.axis_scan.set_title("Approximate distance (m)")
			
			self.canvas.draw()
			print("Draw")
			
	def stop(self):
		self.running = False
	
	def get_list_indizes(self, x, max_values):
		l = len(x)
		start_index = 0
		end_index = 0
		if(l < max_values):
			start_index = 	0
			end_index = 	l
		else:
			start_index = 	l-max_values
			end_index = 	l
		
		return start_index, end_index
	
	def get_y_lim(self, y, default_min, default_max):
		l = len(y)
		y_lim_max = default_max
		y_lim_min = default_min
		if(l == 0):
			return y_lim_min, y_lim_max
			
		maximum = np.max(y)
		minimum = np.min(y)
		
		if(maximum > default_max):
			y_lim_max = maximum
		if(minimum < default_min):
			y_lim_min = minimum
		return y_lim_min, y_lim_max
	
	def get_x_lim_time(self, display_time, start_time):
		cur_time = time.time()
		x_lim_min = 0
		x_lim_max = cur_time - start_time
		
		if(x_lim_max < display_time):
			x_lim_min = 0
		else:
			x_lim_min = x_lim_max - display_time

		return x_lim_min, x_lim_max

		
if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Please enter badge MAC address")
        exit(1)
    device_addr = sys.argv[1]
    MAC_ADDRESS = device_addr
    app = App()


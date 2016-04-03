#!/usr/bin/env python
from __future__ import absolute_import, division, print_function, unicode_literals
import sys
from datetime import datetime, timedelta

import matplotlib.pyplot as plt


class DataLoader(object):

    def __init__(self, filename):
        """This function will load and start parsing the file. The 
           schema of the file looks like the following 
           Date(%Y-%m-%d %H:%M:%S), Voltage(float), Sampling 
           frequency(milliseconds), data0, data1...\n
         """
        lines = open(filename).readlines()
        self.data = []
        datefmt = "%Y-%m-%d %H:%M:%S"
        for line in lines:
            data = line.split(',')
            dtime = datetime.strptime(data.pop(0), datefmt)
            volts = float(data.pop(0))
            freq = int(data.pop(0))
            samples = {"Frequency": freq, "Voltage": volts, "Intial_dtime": dtime}
            samples['samples'] = [{"time": dtime + timedelta(milliseconds=mul*freq), "signal": int(i)} for mul, i in enumerate(data)]
            self.data.append(samples)
     
#    def moredata(self, data):
#        self.data.append(process(data))


if __name__ == '__main__':
    filename = sys.argv[1]
    data = DataLoader(filename)
    print("Yay")
    plt.plot(sorted(data.data, key=lambda x: x['samples']))
    plt.show()
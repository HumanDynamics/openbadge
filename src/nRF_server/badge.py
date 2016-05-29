from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers
from nrf import Nrf, SimpleDelegate
from math import floor
from dateutil import tz
import struct
import datetime
import time

#This class is the contents of one chunk of data
class Chunk():
    maxSamples = 114
    
    def __init__(self, header, data):
        self.ts,self.fract,self.voltage,self.sampleDelay,self.numSamples = header
        self.samples = data[0:]
    
    def setHeader(self,header):
        self.ts,self.fract,self.voltage,self.sampleDelay,self.numSamples = header
    
    def getHeader(self):
        return (self.ts,self.fract,self.voltage,self.sampleDelay,self.numSamples)
        
    def addData(self,data):
        self.samples.extend(data)
        if len(self.samples) > self.numSamples:
            print("too many samples received?")
            raise UserWarning("Chunk overflow")
    
    def reset(self):
        self.ts = None
        self.fract = None
        self.voltage = None
        self.sampleDelay = None
        self.numSamples = None
        self.samples = []
        
    def completed(self):
        return len(self.samples) >= self.numSamples

# This class handles incoming data from the badge. It will buffer the
# data so external processes can read from it more easy. Reset will
# delete all buffered data
class BadgeDelegate(DefaultDelegate):
    tempChunk = Chunk((None,None,None,None,None),[])
    #data is received as chunks, keep the chunk organization
    chunks = []
    #to keep track of the dialogue
    gotStatus = False
    gotDateTime = False
    gotHeader = False
    numSamples = 0  # expected number of samples from current chunk
    gotEndOfData = False #flag that indicates no more data will be sent
    #badge states, reported from badge
    sentStartRec = False
    
    clockSet = False  # whether the badge's time had been set
    dataReady = False # whether there's unsent data in FLASH
    recording = False # whether the badge is collecting samples
    timestamp_sec = None # badge time in seconds
    timestamp_ms = None  # fractional part of badge time
    voltage = None       # badge battery voltage
    
    timestamp = None # badge time as timestamp (includes seconds+milliseconds)

    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)
        self.reset()
   
    def reset(self):
        self.tempChunk = Chunk((None,None,None,None,None),[])
        self.chunks = [] 
        self.gotStatus = False
        self.gotEndOfData = False
        self.dataReady = False
        self.gotHeader = False
        self.sentStartRec = False
        

    def handleNotification(self, cHandle, data):
        if not self.gotStatus:  # whether date has been set
            self.clockSet,self.dataReady,self.recording,self.timestamp_sec,self.timestamp_ms,self.voltage = struct.unpack('<BBBLHf',data)
            self.gotStatus = True
        elif not self.sentStartRec:
            self.sentStartRec = True
        elif not self.gotHeader:
            self.tempChunk.reset()
            self.tempChunk.setHeader(struct.unpack('<LHfHB',data)) #time, fraction time (ms), voltage, sample delay
            if (self.tempChunk.sampleDelay == 0): # got an empty header? done
                self.gotEndOfData = True
                pass
            else:
                self.tempChunk.ts = self._longToDatetime(self.tempChunk.ts)  # fix time
                self.tempChunk.ts = self.tempChunk.ts + datetime.timedelta(milliseconds=self.tempChunk.fract)  # add ms
                self.gotHeader = True

        else: # just data
            sample_arr = struct.unpack('<%dB' % len(data),data) # Nrfuino bytes are unsigned bytes
            self.tempChunk.addData(sample_arr)
            if self.tempChunk.completed():
                #add chunk with tempChunk's data to list
                #print self.tempChunk.ts, self.tempChunk.samples
                self.chunks.append(Chunk(self.tempChunk.getHeader(),self.tempChunk.samples))
                self.gotHeader = False  #we should move on to a new chunk

    # reads UTC time from badge, stores is at local time (that what datetime
    # does for some reason)
    def _longToDatetime(self,n):
        local_ts = datetime.datetime.fromtimestamp(n)
        return local_ts

class Badge(Nrf):
    dlg = None
    def __init__(self, periph):
        Nrf.__init__(self, periph)
        self.NrfReadWrite.enable()
        self.NrfNotifications.enable()
        self.dlg = BadgeDelegate(params=1)
        self.setDelegate(self.dlg)

    def read(self,fmt):
        d = self.NrfReadWrite.read()
        arr = struct.unpack(fmt, d)
        return arr


    def write(self,fmt,*arr):
        s = struct.pack(fmt, *arr)
        return self.NrfReadWrite.write(s)

    # sends status request with UTC time to the badge
    def sendStatusRequest(self):
        n = datetime.datetime.utcnow()
        long_epoch_seconds, ts_fract = self._datetimeToEpoch(n)
        return self.write('<cLH',"s",long_epoch_seconds,ts_fract)

    # sends start rec request with UTC time to the badge
    def sendStartRecRequest(self,timeoutMinutes):
        n = datetime.datetime.utcnow()
        long_epoch_seconds, ts_fract = self._datetimeToEpoch(n)
        return self.write('<cLHH',"1",long_epoch_seconds,ts_fract,timeoutMinutes)


    # send request for data since given date
    # Note - given date should be in local timezone. It will
    # be converted to UTC before sending to the badge
    def sendDataRequest(self, lastChunkDate):
        n = self._localToUTC(lastChunkDate)
        long_epoch_seconds, ts_fract = self._datetimeToEpoch(n)
        return self.write('<cLH',"r",long_epoch_seconds,ts_fract)

    def _datetimeToEpoch(self, n):
        epoch_seconds = (n - datetime.datetime(1970,1,1)).total_seconds()
        long_epoch_seconds = long(floor(epoch_seconds))
        ts_fract = n.microsecond/1000;
        return(long_epoch_seconds,ts_fract)

    # converts local time to UTC
    def _localToUTC(self, localDateTime):
        localTz = tz.tzlocal()
        utcTz = tz.gettz('UTC')
        localDateTimeWithTz = localDateTime.replace(tzinfo=localTz)
        return localDateTimeWithTz.astimezone(utcTz).replace(tzinfo=None)

if __name__ == "__main__":
    import time
    import sys
    import argparse
    import datetime

    print(Badge.localToUTC(datetime.datetime.now()))

    bdg_addr = "E1:C1:21:A2:B2:E0"
    bdg = Badge(bdg_addr)
    time.sleep(1.0)

    try:
      while not bdg.dlg.gotStatus:
            bdg.NrfReadWrite.write("s")  # ask for status
            bdg.waitForNotifications(1.0)  # waiting for status report

      print "got status"

      while not bdg.dlg.gotDateTime:
            bdg.NrfReadWrite.write("t")  # ask for time
            bdg.waitForNotifications(1.0)
            
      print("Got datetime: {},{}".format(bdg.dlg.badge_sec,bdg.dlg.badge_ts))

    except:
      retcode=-1
      e = sys.exc_info()[0]
      print("unexpected failure, {}".format(e))

    finally:
      bdg.disconnect()
      del bdg

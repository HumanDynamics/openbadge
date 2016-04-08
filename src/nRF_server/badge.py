from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers
from nrf import Nrf, SimpleDelegate
import struct
import datetime
import time

#This class is the contents of one chunk of data
class Chunk():
    maxSamples = 116
    
    def __init__(self, header, data):
        self.ts,self.fract,self.voltage,self.sampleDelay = header
        self.samples = data[0:]
    
    def setHeader(self,header):
        self.ts,self.fract,self.voltage,self.sampleDelay = header
    
    def getHeader(self):
        return (self.ts,self.fract,self.voltage,self.sampleDelay)
        
    def addData(self,data):
        self.samples.extend(data)
        if len(self.samples) > self.maxSamples:
            logger.error("chunk overflow")
            #raise UserWarning("Chunk overflow")
    
    def reset(self):
        self.ts = None
        self.fract = None
        self.voltage = None
        self.sampleDelay = None
        self.samples = []
        
    def completed(self):
        return len(self.samples) >= self.maxSamples

# This class handles incoming data from the badge. It will buffer the
# data so exeternal processes can read from it more easy. Reset will
# delete all buffered data
class BadgeDelegate(DefaultDelegate):
    tempChunk = Chunk((None,None,None,None),[])
    #data is received as chunks, keep the chunk organization
    chunks = []
    #to keep track of the dialogue
    gotStatus = False
    gotDateTime = False
    gotHeader = False
    #badge states, reported from badge
    needDate = False
    dataReady = False
    badge_sec = None # badge time in seconds
    badge_ts = None # badge time as timestamp

    def __init__(self, params):
        btle.DefaultDelegate.__init__(self)
        self.reset()
   
    def reset(self):
        self.tempChunk = Chunk((None,None,None,None),[])
        self.chunks = [] 
        self.gotStatus = False
        self.gotHeader = False
        self.needDate = False
        self.dataReady = False

    def handleNotification(self, cHandle, data):
        if not self.gotStatus:  # whether date has been set
            self.gotStatus = True  #reverted to false if it turns out invalid
            if str(data) == 'n':
                self.needDate = True
                self.dataReady = False
            elif str(data) == 'd':
                self.needDate = False
                self.dataReady = True
            elif str(data) == 's':
                self.needDate = False
                self.dataReady = False #synced but no data ready, no need to do anything
            else:
                self.gotStatus = False #invalid status.  retry?
        elif not self.gotDateTime:
            self.badge_sec = struct.unpack('<L',data)[0]
            self.badge_ts = self._longToDatetime(self.badge_sec) #fix time
            self.gotDateTime = True
        elif not self.gotHeader:
            self.tempChunk.reset()
            self.tempChunk.setHeader(struct.unpack('<Lhfh',data)) #time, fraction time (ms), voltage, sample delay
            self.tempChunk.ts = self._longToDatetime(self.tempChunk.ts) #fix time
            #print "{},{},{}".format(self.ts, self.voltage, self.sampleDelay)
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
    # does for some reason
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


    def write(self,arr,fmt):
        s = struct.pack(fmt, arr)
        return self.NrfReadWrite.write(s)

    # sends status request with UTC time to the badge
    '''
    def sendStatusRequest(self):
        n = datetime.datetime.utcnow()
        epoch_seconds = (n - datetime.datetime(1970,1,1)).total_seconds()
        long_epoch_seconds = long(round(epoch_seconds))
        self.dlg.needDate = False
        return self.write(long_epoch_seconds,'<L') 
    '''

    # sends UTC time to the badge
    def sendDateTime(self):
        n = datetime.datetime.utcnow()
        epoch_seconds = (n - datetime.datetime(1970,1,1)).total_seconds()
        long_epoch_seconds = long(round(epoch_seconds))
        self.dlg.needDate = False
        return self.write(long_epoch_seconds,'<L') 


if __name__ == "__main__":
    import time
    import sys
    import argparse

    bdg_addr = "E1:C1:21:A2:B2:E0" #"CC:4A:FD:5C:E3:5B"
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

      #print "Sending date and time"
      #bdg.sendDateTime()

      '''
      print "Getting data"
      wait_count = 0;
      while True:
        if bdg.waitForNotifications(1.0):
           # handleNotification() was called
           continue
        print "Waiting..."
        wait_count = wait_count+1
        if wait_count > 3: break
      print bdg.dlg.samples 
      print bdg.dlg.ts
      print bdg.dlg.voltage
      print bdg.dlg.sampleDelay
      bdg.dlg.reset()
      print bdg.dlg.sampleDelay
      '''
    except:
      retcode=-1
      e = sys.exc_info()[0]
      print("unexpected failure, {}".format(e))

    finally:
      bdg.disconnect()
      del bdg

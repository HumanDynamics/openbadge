from __future__ import division
from __future__ import print_function

from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers
from bluepy.btle import BTLEException

from nrf import Nrf, SimpleDelegate
import logging.handlers

from badge_dialogue import BadgeDialogue

import struct
from math import floor
import datetime
import signal
import traceback
import time

WAIT_FOR = 1.0  # timeout for WaitForNotification calls.  Must be > samplePeriod of badge
PULL_WAIT = 2
RECORDING_TIMEOUT = 10

class TimeoutError(Exception):
    """
    # Raises a timeout exception if a function takes too long
    # http://stackoverflow.com/questions/2281850/timeout-function-if-it-takes-too-long-to-finish
    """
    pass


class timeout:
    """
    Or, to use with a "with" statement
    """
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

class Expect:
    none,status,timestamp,header,samples,scanHeader,scanDevices = range(7)

class Chunk():
    """
    #This class is the contents of one chunk of mic data
    """
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


class SeenDevice():
    """
    Represents an instance of a proximity data for a device found during a scan
    """
    def __init__(self,(ID,rssi,count)):
        self.ID = ID
        self.rssi = rssi
        self.count = count


class Scan():
    """
    This class holds the content of one scan record
    """

    def __init__(self, header, devices):
        self.ts, self.voltage, self.numDevices = header
        self.devices = devices[0:]

    def setHeader(self,header):
        self.ts, self.voltage, self.numDevices = header

    def getHeader(self):
        return (self.ts, self.voltage, self.numDevices)

    def addDevices(self,devices):
        self.devices.extend(devices)
        if len(self.devices) > self.numDevices:
            print("too many devices received?")
            raise UserWarning("Chunk overflow")

    def reset(self):
        self.ts = None
        self.voltage = None
        self.numDevices = None
        self.devices = []

    def completed(self):
        return len(self.devices) >= self.numDevices


class BadgeDelegate(DefaultDelegate):
    """
    This class handles incoming data from the badge. It will buffer the
    data so external processes can read from it more easy. Reset will
    delete all buffered data
    """
    tempChunk = Chunk((None,None,None,None,None),[])
    tempScan = Scan((None, None, None), [])

    #data is received as chunks, keep the chunk organization
    chunks = []
    scans = []
    #to keep track of the dialogue - data expected to be received next from badge
    expected = Expect.none

    gotStatus = False
    gotTimestamp = False
    gotEndOfData = False #flag that indicates no more data will be sent

    # Parameters received from badge status report
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
        self.tempScan = Scan((None,None,None),[])
        self.chunks = []
        self.scans = []

        self.expected = Expect.none

        self.gotStatus = False
        self.gotTimestamp = False
        self.gotEndOfData = False
        self.gotEndOfScans = False
        self.dataReady = False
        self.clockSet = False  # whether the badge's time had been set
        self.dataReady = False # whether there's unsent data in FLASH
        self.recording = False # whether the badge is collecting samples
        self.scanning = False
        self.timestamp_sec = None # badge time in seconds
        self.timestamp_ms = None  # fractional part of badge time
        self.voltage = None       # badge battery voltage

        self.timestamp = None # badge time as timestamp (includes seconds+milliseconds)

    def handleNotification(self, cHandle, data):
        if self.expected == Expect.status:  # whether we expect a status packet
            self.dataReady = True
            self.clockSet,self.scanning,self.recording,self.timestamp_sec,self.timestamp_ms,self.voltage = struct.unpack('<BBBLHf',data)
            self.gotStatus = True
            self.expected = Expect.none
        elif self.expected == Expect.timestamp:
            self.timestamp_sec,self.timestamp_ms = struct.unpack('<LH',data)
            self.gotTimestamp = True
            self.expected = Expect.none
        elif self.expected == Expect.header:
            self.tempChunk.reset()
            self.tempChunk.setHeader(struct.unpack('<LHfHB',data)) #time, fraction time (ms), voltage, sample delay, number of samples
            if (self.tempChunk.sampleDelay == 0): # got an empty header? done
                self.gotEndOfData = True
                self.expected = Expect.none
                pass
            else:
                self.tempChunk.ts += (self.tempChunk.fract/1000.0)
                self.expected = Expect.samples
        elif self.expected == Expect.samples: # just samples
            sample_arr = struct.unpack('<%dB' % len(data),data) # Nrfuino bytes are unsigned bytes
            self.tempChunk.addData(sample_arr)
            if self.tempChunk.completed():
                #add chunk with tempChunk's data to list
                #print self.tempChunk.ts, self.tempChunk.samples
                self.chunks.append(Chunk(self.tempChunk.getHeader(),self.tempChunk.samples))
                self.expected = Expect.header  #we should move on to a new chunk
        elif self.expected == Expect.scanHeader:
            self.tempScan.reset()
            header = struct.unpack('<LfB',data)
            self.tempScan.setHeader(header) #time, voltage, number of devices
            #print self.tempScan.ts
            if (self.tempScan.ts == 0): # got an empty header? done
                self.gotEndOfScans = True
                self.expected = Expect.none
                pass
            else:
                self.expected = Expect.scanDevices

        elif self.expected == Expect.scanDevices: # just devices
            num_devices = int(len(data)/4)
            raw_arr = struct.unpack('<' + num_devices * 'Hbb',data)

            tuple_arr = zip(raw_arr[0::3],raw_arr[1::3],raw_arr[2::3])
            device_arr = [SeenDevice(params) for params in tuple_arr]
            self.tempScan.addDevices(device_arr)
            if self.tempScan.completed():
                #add scan with tempScan's data to list
                #print self.tempScan.ts, self.tempChunk.devices
                self.scans.append(Scan(self.tempScan.getHeader(),self.tempScan.devices))
                self.expected = Expect.scanHeader  #we should move on to a new chunk
        else:  # not expecting any data from badge
            print("Error: not expecting data")


class BadgeConnection(Nrf):
    """
    Extends a Nrf device and wraps with struct
    """
    dlg = None

    def __init__(self, periph, dlg):
        Nrf.__init__(self, periph)
        self.NrfReadWrite.enable()
        self.NrfNotifications.enable()
        self.setDelegate(dlg)

    def read(self, fmt):
        d = self.NrfReadWrite.read()
        arr = struct.unpack(fmt, d)
        return arr

    def write(self, fmt, *arr):
        s = struct.pack(fmt, *arr)
        return self.NrfReadWrite.write(s)


class BadgeAddressAdapter(logging.LoggerAdapter):
    """
    Log adapter that passes badge 'addr' to be prepended to the log message.
    """
    def process(self, msg, kwargs):
        return '[%s] %s' % (self.extra['addr'], msg), kwargs

class Badge():
    def __init__(self, addr,logger):
        self.addr = addr
        self.logger = adapter = BadgeAddressAdapter(logger, {'addr': addr})
        self.dlg = None
        self.conn = None
        self.connDialogue = BadgeDialogue(self)

    def connect(self):
        self.logger.info("Connecting to {}".format(self.addr))
        self.dlg = BadgeDelegate(params=1)
        self.conn = BadgeConnection(self.addr, self.dlg)

    def disconnect(self):
        if self.conn is not None:
            self.logger.info("Disconnecting from {}".format(self.addr))
            self.conn.disconnect()
        else:
            self.logger.info("Can't disconnect from {}. Not connected".format(self.addr))

    # sends status request with UTC time to the badge
    def sendStatusRequest(self):
        long_epoch_seconds, ts_fract = now_utc_epoch()
        self.dlg.expected = Expect.status
        return self.conn.write('<cLH',"s",long_epoch_seconds,ts_fract)

    # sends request to start recording, with specified timeout
    #   (if after timeout minutes badge has not seen server, it will stop recording)
    def sendStartRecRequest(self, timeout):
        long_epoch_seconds, ts_fract = now_utc_epoch()
        self.dlg.gotTimestamp = False
        self.dlg.expected = Expect.timestamp
        return self.conn.write('<cLHH',"1",long_epoch_seconds,ts_fract,timeout)

    # sends request to stop recording
    def sendStopRec(self):
        return self.conn.write('<c',"0")

    # sends request to start scan, with specified timeout and other scan parameters
    #   (if after timeout minutes badge has not seen server, it will stop recording)
    def sendStartScanRequest(self, timeout, window, interval, duration, period):
        long_epoch_seconds, ts_fract = now_utc_epoch()
        self.dlg.gotTimestamp = False
        self.dlg.expected = Expect.timestamp
        return self.conn.write('<cLHHHHHH',"p",long_epoch_seconds,ts_fract,timeout,window,interval,duration,period)

    # sends request to stop recording
    def sendStopScan(self):
        return self.conn.write('<c',"q")

    def sendIdentifyReq(self, timeout):
        return self.conn.write('<cH',"i",timeout)

    def sendDataRequest(self, ts, ts_fract):
        """
        send request for data since given date
        Note - date is given in UTC epoch
        :param ts: last timestamp, seconds
        :param ts_fract: last timestamp, fraction
        :return:
        """
        self.dlg.expected = Expect.header
        return self.conn.write('<cLH',"r",ts,ts_fract)

    def sendScanRequest(self, ts):
        """
        send request for proximity data since given date
        Note - date is given in UTC epoch
        :param ts: last timestamp, seconds
        :return:
        """
        self.dlg.expected = Expect.scanHeader
        return self.conn.write('<cL',"b",ts)

    def sync_timestamp(self):
        """
        used for sync the date of the badge
        :param addr:
        :return:
        """
        retcode = -1
        try:
            with timeout(seconds=5, error_message="Connect timeout"):
                self.connect()

            self.logger.info("Connected")

            with timeout(seconds=5, error_message="Status request timeout (wrong firmware version?)"):
                while not self.dlg.gotStatus:
                    self.sendStatusRequest()  # ask for status
                    self.conn.waitForNotifications(WAIT_FOR)  # waiting for status report

                    self.logger.info("Got status")

                    self.sendIdentifyReq(10)

                if self.dlg.timestamp_sec != 0:
                    self.logger.info("Badge datetime was: {},{}".format(self.dlg.timestamp_sec, self.dlg.timestamp_ms))
                else:
                    self.logger.info("Badge previously unsynced.")

            retcode = 0

        except BTLEException, e:
            self.logger.error("Bluetooth error")
            self.logger.error(e.code)
            self.logger.error(e.message)
        except TimeoutError, te:
            self.logger.error("TimeoutError: " + te.message)
        except Exception as e:
            s = traceback.format_exc()
            self.logger.error("unexpected failure, {} ,{}".format(e, s))
        finally:
            self.disconnect()

        return retcode

    def start_recording(self):
        """
        Requests badge to start recording
        :param addr:
        :return:
        """
        retcode = -1
        try:
            with timeout(seconds=5, error_message="Connect timeout"):
                self.connect()

            self.logger.info("Connected")

            # Starting audio rec
            self.logger.info("Starting audio recording")
            with timeout(seconds=5, error_message="StartRec timeout (wrong firmware version?)"):
                while not self.dlg.gotTimestamp:
                    self.sendStartRecRequest(RECORDING_TIMEOUT)  # start recording
                    self.conn.waitForNotifications(WAIT_FOR)  # waiting for time acknowledgement

                self.logger.info("Got time ack")

                if self.dlg.timestamp_sec != 0:
                    self.logger.info("Badge datetime was: {},{}".format(self.dlg.timestamp_sec, self.dlg.timestamp_ms))
                else:
                    self.logger.info("Badge previously unsynced.")

            # Reset flag (hacky)
            self.dlg.gotTimestamp = False

            # Starting scans
            self.logger.info("Starting proximity scans")
            with timeout(seconds=5, error_message="StartScan timeout (wrong firmware version?)"):
                while not self.dlg.gotTimestamp:
                    self.sendStartScanRequest(RECORDING_TIMEOUT, 0, 0, 0, 0)  # start recording
                    self.conn.waitForNotifications(WAIT_FOR)  # waiting for time acknowledgement

                self.logger.info("Got time ack")

                if self.dlg.timestamp_sec != 0:
                    self.logger.info("Badge datetime was: {},{}".format(self.dlg.timestamp_sec, self.dlg.timestamp_ms))
                else:
                    self.logger.info("Badge previously unsynced.")

            retcode = 0

        except BTLEException, e:
            self.logger.error("Bluetooth error")
            self.logger.error(e.code)
            self.logger.error(e.message)
        except TimeoutError, te:
            self.logger.error("TimeoutError: " + te.message)
        except Exception as e:
            s = traceback.format_exc()
            self.logger.error("unexpected failure, {} ,{}".format(e, s))
        finally:
            self.disconnect()

        return retcode

    def pull_data(self, last_audio_ts, last_audio_ts_fract, last_proximity_ts):
        """
        Attempts to read data from the device
        """
        retcode = -1
        try:
            with timeout(seconds=5, error_message="Connect timeout"):
                self.connect()

            self.logger.info("Connected")

            with timeout(seconds=5, error_message="Dialogue timeout (wrong firmware version?)"):
                while not self.dlg.gotStatus:
                    self.sendStatusRequest()  # ask for status
                    self.conn.waitForNotifications(WAIT_FOR)  # waiting for status report

                self.logger.info("Got status")
                self.logger.info("Badge datetime was: {},{}, Voltage: {}".format(
                    self.dlg.timestamp_sec, self.dlg.timestamp_ms, self.dlg.voltage))

            # data request using the "r" command - data since time X
            self.logger.info("Requesting data...")

            self.sendDataRequest(last_audio_ts, last_audio_ts_fract)  # ask for data
            wait_count = 0
            while True:
                if self.dlg.gotEndOfData == True:
                    break
                if self.conn.waitForNotifications(WAIT_FOR):
                    # if got data, don't inrease the wait counter
                    continue
                self.logger.info("Waiting for more data...")
                wait_count = wait_count + 1
                if wait_count >= PULL_WAIT: break
            self.logger.info("finished reading data")

            # data request using the "r" command - data since time X
            self.logger.info("Requesting scans...")
            self.sendScanRequest(last_proximity_ts)
            wait_count = 0
            while True:
                if self.dlg.gotEndOfScans == True:
                    break
                if self.conn.waitForNotifications(WAIT_FOR):
                    # if got data, don't inrease the wait counter
                    continue
                self.logger.info("Waiting for more data...")
                wait_count = wait_count + 1
                if wait_count >= PULL_WAIT: break
            self.logger.info("finished reading data")

            retcode = 0

        except BTLEException, e:
            self.logger.error("failed pulling data")
            self.logger.error(e.code)
            self.logger.error(e.message)
        except TimeoutError, te:
            self.logger.error("TimeoutError: " + te.message)
        except Exception as e:
            s = traceback.format_exc()
            self.logger.error("unexpected failure, {} ,{}".format(e, s))
        finally:
           self.disconnect()

        return retcode


def print_bytes(data):
    """
    Prints a given string as an array of unsigned bytes
    :param data:
    :return:
    """
    raw_arr = struct.unpack('<%dB' % len(data), data)
    print(raw_arr)


def datetime_to_epoch(d):
        """
        Converts given datetime to epoch seconds and ms
        :param d: datetime
        :return:
        """
        epoch_seconds = (d - datetime.datetime(1970, 1, 1)).total_seconds()
        long_epoch_seconds = long(floor(epoch_seconds))
        ts_fract = d.microsecond / 1000;
        return (long_epoch_seconds, ts_fract)


def now_utc():
        """
        Returns current UTC as datetime
        :return: datetime
        """
        return datetime.datetime.utcnow()


def now_utc_epoch():
        """
        Returns current UTC as epoch seconds and ms
        :return: long_epoch_seconds, ts_fract
        """
        return datetime_to_epoch(now_utc())

if __name__ == "__main__":
   pass

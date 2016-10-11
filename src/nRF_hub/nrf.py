#!/usr/bin/env python

from __future__ import absolute_import, division, print_function

from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate, AssignedNumbers
import struct
import math
 
#UUID base used by NRF51 softdevice custom characteristics (like in UART service)
def _NRF_UUID(val):
    return UUID("6e40{}-b5a3-f393-e0a9-e50e24dcca9e".format(val))
#UUID base for standard BLE characteristics
def _BLE_UUID(val):
    return UUID("{}-0000-1000-8000-00805f9b34fb".format(val))

class SensorBase:
    # Derived classes should set: rxUUID, txUUID, configUUID

    def __init__(self, periph):
        self.periph = periph
        self.rx = None
        self.tx = None

    def enable(self):
        if self.tx is None:
            self.tx = self.periph.getCharacteristics(uuid=self.txUUID)[0]
        if self.rx is None:
            self.rx = self.periph.getCharacteristics(uuid=self.rxUUID)[0]

    def disable(self):
        None
        #if self.ctrl is not None:
        #    self.ctrl.write(self.sensorOff)

class NrfReadWrite(SensorBase):
    rxUUID = _NRF_UUID("0003")
    txUUID = _NRF_UUID("0002")

    def __init__(self, periph):
        SensorBase.__init__(self, periph)

    def read(self):
        return self.rx.read()

    def write(self,arr):
        return self.tx.write(arr)

class NrfNotifications(SensorBase):
    rxUUID = _NRF_UUID("0003")
    
    # setting up for future use. this feature is not supported by BluePy
    configUUID = _BLE_UUID("00002902")

    # instead, we will use the handle directly
    # Handle for characteristic that configures notifications/indications
    CONFIG_HANDLE = 0x000c;

    def __init__(self, periph):
        SensorBase.__init__(self, periph)

    def enable(self):  #enable notifications
        self.periph.writeCharacteristic(handle=self.CONFIG_HANDLE,val=struct.pack('<bb', 0x01, 0x00))

    def disable(self):  #disable notifications/indications
        self.periph.writeCharacteristic(handle=self.CONFIG_HANDLE,val=struct.pack('<bb', 0x00, 0x00))

class SimpleDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        print(repr(data))

class Nrf(Peripheral):
    def __init__(self,addr):
        # address type must be random for RFD
        Peripheral.__init__(self,addr,btle.ADDR_TYPE_RANDOM)
        # self.discoverServices()
        self.NrfReadWrite = NrfReadWrite(self)
        self.NrfNotifications = NrfNotifications(self)

if __name__ == "__main__":
    import time
    import sys
    import argparse
    import datetime

    parser = argparse.ArgumentParser()
    parser.add_argument('host', action='store',help='MAC of BT device')

    arg = parser.parse_args(sys.argv[1:])

    print('Connecting to ' + arg.host)
    nrf = Nrf(arg.host)

    try:
        nrf.NrfReadWrite.enable()
        nrf.NrfNotifications.enable()
        nrf.setDelegate(SimpleDelegate())
        
        while 1:
            if nrf.waitForNotifications(5.0):
                '''result = struct.unpack('<6Bb',nrf.NrfReadWrite.read())
                print result
                address = [result[i] for i in range(6)]  #get address as list
                strength = result[6]  #get signal strength
                print address, strength '''
                print("reading: {}".format(struct.unpack('<L',nrf.NrfReadWrite.read())))
        while 1:
            pass
        '''
        nrf.waitForNotifications(1.0)
        print "status: {}".format(nrf.NrfReadWrite.read())
      
        n = datetime.datetime.utcnow()
        epoch_seconds = (n - datetime.datetime(1970,1,1)).total_seconds()
        long_epoch_seconds = long(round(epoch_seconds))
        s = struct.pack('<L',long_epoch_seconds)
        nrf.NrfReadWrite.write(s) 
      
        nrf.waitForNotifications(5.0)
        print "now: {}".format(nrf.NrfReadWrite.read())
      
      
        nrf.NrfReadWrite.write("d")
        nrf.waitForNotifications(5.0)
        data = nrf.NrfReadWrite.read()
        print "data: {}w".format(struct.unpack('<%dB' % len(data),data))

        while nrf.waitForNotifications(1.0):
            pass
        '''
    finally:
      nrf.disconnect()
      del nrf

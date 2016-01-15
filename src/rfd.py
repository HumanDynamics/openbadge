from bluepy import btle
from bluepy.btle import UUID, Peripheral, DefaultDelegate
import struct

def _RFD_UUID(val):
    return UUID("{}-0000-1000-8000-00805f9b34fb".format(val))

class SensorBase:
    # Derived classes should set: svcUUID, ctrlUUID, dataUUID

    def __init__(self, periph):
        self.periph = periph
        self.ctrl = None
        self.data = None

    def enable(self):
        if self.ctrl is None:
            self.ctrl = self.periph.getCharacteristics(uuid=self.ctrlUUID)[0]
        if self.data is None:
            self.data = self.periph.getCharacteristics(uuid=self.dataUUID)[0]

    def disable(self):
        pass
        #if self.ctrl is not None:
        #    self.ctrl.write(self.sensorOff)

class RfdReadWrite(SensorBase):
    dataUUID = _RFD_UUID("00002221")
    ctrlUUID = _RFD_UUID("00002222")

    def __init__(self, periph):
        SensorBase.__init__(self, periph)

    def read(self):
        return self.data.read()

    def write(self,arr):
        return self.ctrl.write(arr)

class RfdNotifications(SensorBase):
    dataUUID = _RFD_UUID("00002221")

    # setting up for future use. this feature is not supported by BluePy
    ctrlUUID = _RFD_UUID("00002902")

    # instead, we will use the handle directly
    NOTIFICATION_HANDLE = 0x000f;

    def __init__(self, periph):
        SensorBase.__init__(self, periph)

    def enable(self):
        self.periph.writeCharacteristic(handle=self.NOTIFICATION_HANDLE,val=struct.pack('<bb', 0x01, 0x00))

    def disable(self):
        self.periph.writeCharacteristic(handle=self.NOTIFICATION_HANDLE,val=struct.pack('<bb', 0x00, 0x00))

class SimpleDelegate(DefaultDelegate):
    def __init__(self):
        DefaultDelegate.__init__(self)

    def handleNotification(self, cHandle, data):
        print(repr(data))

class Rfd(Peripheral):
    def __init__(self,addr):
        # address type must be random for RFD
        Peripheral.__init__(self,addr,btle.ADDR_TYPE_RANDOM)
        # self.discoverServices()
        self.RfdReadWrite = RfdReadWrite(self)
        self.RfdNotifications = RfdNotifications(self)

if __name__ == "__main__":
    import time
    import sys
    import argparse
    import datetime

    parser = argparse.ArgumentParser()
    parser.add_argument('host', action='store', help='MAC of BT device')

    arg = parser.parse_args(sys.argv[1:])

    print('Connecting to ' + arg.host)
    rfd = Rfd(arg.host)

    try:
        rfd.RfdReadWrite.enable()
        rfd.RfdNotifications.enable()
        rfd.setDelegate(SimpleDelegate())

        time.sleep(1.0)

        rfd.waitForNotifications(1.0)
        print("status: {}".format(rfd.RfdReadWrite.read()))

        n = datetime.datetime.utcnow()
        epoch_seconds = (n - datetime.datetime(1970,1,1)).total_seconds()
        long_epoch_seconds = long(round(epoch_seconds))
        s = struct.pack('<L',long_epoch_seconds)
        rfd.RfdReadWrite.write(s)

        rfd.waitForNotifications(5.0)
        print("now: {}".format(rfd.RfdReadWrite.read()))


        rfd.RfdReadWrite.write("d")
        rfd.waitForNotifications(5.0)
        data = rfd.RfdReadWrite.read()
        print("data: {}w".format(struct.unpack('<%dB' % len(data),data)))

        while rfd.waitForNotifications(1.0):
            pass
    finally:
        rfd.disconnect()
        del rfd

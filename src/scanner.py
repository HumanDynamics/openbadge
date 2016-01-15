#!/usr/bin/env python

from bluepy.btle import Peripheral, UUID, Scanner
import bluepy.sensortag as sensortag


class One(Peripheral):

    pass


class Two(UUID):

    pass

s = Scanner()
s.scan(timeout=10)

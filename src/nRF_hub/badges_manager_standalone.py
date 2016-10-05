import os
import re
import logging

from badge import Badge,now_utc_epoch


class BadgesManagerStandalone():
    def __init__(self, logger):
        self._badges= None
        self.logger = logger
        self._device_file = "device_macs.txt"

        self._init_ts,self._init_ts_fract = now_utc_epoch()
        logger.debug("Will request data since {} {}".format(self._init_ts,self._init_ts_fract))

    def _read_file(self,device_file):
        """
        refreshes an internal list of devices included in device_macs.txt
        Format is device_mac<space>device_name
        :param device_file:
        :return:
        """
        if not os.path.isfile(device_file):
            self.logger.error("Cannot find devices file: {}".format(device_file))
            exit(1)
        self.logger.info("Reading devices from file: {}".format(device_file))

        regex = re.compile(r'^([A-Fa-f0-9]{2}(?::[A-Fa-f0-9]{2}){5}).*')

        with open(device_file, 'r') as devices_macs:
            devices = [regex.findall(line) for line in devices_macs]
            devices = filter(lambda x: x, map(lambda x: x[0] if x else False, devices))
            devices = [d.upper() for d in devices]

        for d in devices:
            self.logger.info("    {}".format(d))

        return devices

    def pull_badges_list(self):
        devices = self._read_file(self._device_file)
        self._badges = {mac: Badge(mac,
                             self.logger,
                             key=mac,  # using mac as key since no other key exists
                             init_audio_ts_int=self._init_ts,
                             init_audio_ts_fract=self._init_ts_fract,
                             init_proximity_ts=self._init_ts,
                             ) for mac in devices
                  }

    def send_badge(self, mac):
        """
        Sends timestamps of the given badge to the server
        :param mac:
        :return:
        """
        pass # not implemented in standalone

    @property
    def badges(self):
        if self._badges is None:
            raise Exception('Badges list has not been initialized yet')
        return self._badges

if __name__ == "__main__":
    logging.basicConfig()
    logger = logging.getLogger('badge_server')
    logger.setLevel(logging.DEBUG)

    mgr = BadgesManagerStandalone(logger=logger)
    mgr.pull_badges_list()
    print(mgr.badges)
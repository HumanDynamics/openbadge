from __future__ import absolute_import, division, print_function
import requests
import time

from badge import *
from server import BADGE_ENDPOINT, BADGES_ENDPOINT


class BadgeManagerServer:
    def __init__(self, logger):
        self._badges = None
        self.logger = logger

    def _jason_badge_to_object(self, d):
        conv = lambda x: int(float(x))
        return Badge(d.get('badge'),
                    self.logger,
                    d.get('key'),
                    init_audio_ts_int=conv(d.get('last_audio_ts')),
                    init_audio_ts_fract=conv(d.get('last_audio_ts_fract')),
                    init_proximity_ts=conv(d.get('last_proximity_ts'))
        )

    def _read_badges_list_from_server(self, retry=True, retry_delay_sec=5):
        """
        Reads badges info from the server
        :param retry: if blocking is set, hub will keep retrying
        :return:
        """
        server_badges = {}
        done = False

        while not done:
            try:
                self.logger.info("Requesting devices from server...")
                response = requests.get(BADGES_ENDPOINT)
                if response.ok:
                    self.logger.info("Updating devices list ({})...".format(len(response.json())))
                    for d in response.json():
                        server_badges[d.get('badge')] = self._jason_badge_to_object(d)
                    done = True
                else:
                    raise Exception('Got a {} from the server'.format(response.status_code))

            except (requests.exceptions.ConnectionError, Exception) as e:
                self.logger.error("Error reading badges list from server : {}".format(e))
                if not retry:
                    done = True
                else:
                    self.logger.info("Sleeping for {} seconds before retrying".format(retry_delay_sec))
                    time.sleep(retry_delay_sec)

        return server_badges

    def _read_badge_from_server(self, badge_key, retry=False, retry_delay_sec=5):
        """
        Reads given badge info from the server
        :param retry: if blocking is set, hub will keep retrying
        :return:
        """
        done = False

        while not done:
            try:
                self.logger.info("Requesting device {} from server...".format(badge_key))
                response = requests.get(BADGE_ENDPOINT(badge_key))
                if response.ok:
                    #self.logger.debug("Received ({})...".format(response.json()))
                    return self._jason_badge_to_object(response.json())
                else:
                    raise Exception('Got a {} from the server'.format(response.status_code))

            except (requests.exceptions.ConnectionError, Exception) as e:
                self.logger.error("Error reading badge from server : {}".format(e))
                if not retry:
                    done = True
                else:
                    self.logger.info("Sleeping for {} seconds before retrying".format(retry_delay_sec))
                    time.sleep(retry_delay_sec)

        return None

    def _update_badge_with_server_badge(self,badge,server_badge):
        """
        Updates the timestamp of the given badge if the given server badge has more recent timestamps
        :param badge:
        :param server_badge:
        :return:
        """
        mac = badge.addr
        server_ts_int = server_badge.last_audio_ts_int
        server_ts_fract = server_badge.last_audio_ts_fract
        if badge.is_newer_audio_ts(server_ts_int, server_ts_fract):
            #self.logger.debug("Updating {} with new audio timestamp: {} {}"
            #                  .format(mac, server_ts_int, server_ts_fract))
            badge.set_audio_ts(server_ts_int, server_ts_fract)
        else:
            #self.logger.debug("Keeping existing timestamp for {}. Server values were: {} {}"
            #                  .format(mac, server_ts_int, server_ts_fract))
            pass

        # proximity
        server_proximity_ts = server_badge.last_proximity_ts
        if server_proximity_ts > badge.last_proximity_ts:
            #self.logger.debug("Updating {} with new proximity timestamp: {}".format(mac, server_proximity_ts))
            badge.last_proximity_ts = server_proximity_ts

        else:
            #self.logger.debug("Keeping existing proximity timestamp for {}. Server value was: {}"
            #                  .format(mac, server_proximity_ts))
            pass

    def pull_badges_list(self):
        # first time we read from server
        if self._badges is None:
            server_badges = self._read_badges_list_from_server(retry=True)
            self._badges = server_badges
        else:
            # update list
            server_badges = self._read_badges_list_from_server(retry=False)
            for mac in server_badges:
                if mac not in self._badges:
                    # new badge
                    self._badges[mac] = server_badges[mac]
                else:
                    # existing badge. Update if needed
                    # audio
                    badge = self._badges[mac]
                    server_badge = server_badges[mac]
                    self._update_badge_with_server_badge(badge,server_badge)

    def pull_badge(self, mac):
        """
        Contacts to server (if responding) and updates the given badge data
        :param mac:
        :return:
        """
        badge = self._badges[mac]
        server_badge = self._read_badge_from_server(badge .key)
        if server_badge is None:
            self.logger.warn("Could not find device {} in server, or communication problem".format(badge .key))
        else:
            # update timestamps if more recent
            self._update_badge_with_server_badge(badge, server_badge)

    def send_badge(self, mac):
        """
        Sends timestamps of the given badge to the server
        :param mac:
        :return:
        """
        try:
            badge = self._badges[mac]
            data = {
                'last_audio_ts': badge.last_audio_ts_int,
                'last_audio_ts_fract': badge.last_audio_ts_fract,
                'last_proximity_ts': badge.last_proximity_ts,
            }

            self.logger.debug("Sending update badge data to server, badge {} : {}".format(badge.key, data))
            response = requests.patch(BADGE_ENDPOINT(badge.key), data=data)
            if response.ok is False:
                if response.status_code == 400:
                    self.logger.debug("Server had more recent date, badge {} : {}".format(badge.key, response.text))
                else:
                    raise Exception('Server sent a {} status code instead of 200: {}'.format(response.status_code,
                                                                                         response.text))
        except Exception as e:
            self.logger.error('Error sending updated badge into to server: {}'.format(e))

    @property
    def badges(self):
        if self._badges is None:
            raise Exception('Badges list has not been initialized yet')
        return self._badges

if __name__ == "__main__":
    logging.basicConfig()
    logger = logging.getLogger('badge_server')
    logger.setLevel(logging.DEBUG)

    mgr = BadgeManagerServer(logger=logger)
    mgr.pull_badges_list()
    print(mgr.badges)
    mgr.pull_badges_list()
    print(mgr.badges)
    for mac in mgr.badges:
        print("Updating: {}".format(mac))
        mgr.send_badge(mac)

    b1 = mgr.pull_badge(mac)
    print(b1)

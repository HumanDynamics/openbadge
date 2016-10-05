import requests

from badge import *
from server import BADGE, BADGES


class BadgesManagerServer:
    def __init__(self, logger):
        self._badges = None
        self.logger = logger

    def _read_from_server(self, retry=True):
        """
        Reads badges info from the server
        :param retry: is blocking is set, hub will keep retrying
        :return:
        """
        server_badges = {}
        conv = lambda x: int(float(x))
        done = False

        while not done:
            try:
                self.logger.info("Requesting devices from server...")
                response = requests.get(BADGES)
                if response.ok:
                    self.logger.info("Updating devices list ({})...".format(len(response.json())))
                    for d in response.json():
                        server_badges[d.get('badge')] = Badge(d.get('badge'),
                                                            self.logger,
                                                            d.get('key'),
                                                            init_audio_ts_int=conv(d.get('last_audio_ts')),
                                                            init_audio_ts_fract=conv(d.get('last_audio_ts_fract')),
                                                            init_proximity_ts=conv(d.get('last_proximity_ts'))
                                                        )
                    done = True
                else:
                    raise Exception('Got a {} from the server'.format(response.status_code))

            except (requests.exceptions.ConnectionError, Exception) as e:
                self.logger.error("Error reading badges list from server : {}".format(e))
                if not retry:
                    done = True

        return server_badges

    def pull_badges_list(self):
        # first time we read from server
        if self._badges is None:
            server_badges = self._read_from_server(retry=True)
            self._badges = server_badges
        else:
            # update list
            server_badges = self._read_from_server(retry=False)
            for mac in server_badges:
                if mac not in self._badges:
                    # new badge
                    self._badges[mac] = server_badges[mac]
                else:
                    # existing badge. Update if needed
                    # audio
                    badge = self._badges[mac]
                    server_ts_int = server_badges[mac].last_audio_ts_int
                    server_ts_fract = server_badges[mac].last_audio_ts_fract
                    if badge.is_newer_audio_ts(server_ts_int, server_ts_fract):
                        self.logger.debug("Updating {} with new audio timestamp: {] {}"
                                     .format(mac, server_ts_int, server_ts_fract))
                        badge.set_audio_ts(server_ts_int, server_ts_fract)
                    else:
                        self.logger.debug("Keeping existing timestamp for {}. Server values were: {} {}"
                                     .format(mac, server_ts_int, server_ts_fract))

                    # proximity
                    server_proximity_ts = server_badges[mac].last_proximity_ts
                    if server_proximity_ts > badge.last_proximity_ts:
                        self.logger.debug("Updating {} with new proximity timestamp: {}".format(mac, server_proximity_ts))
                        badge.set_audio_ts(server_ts_int, server_ts_fract)

                    else:
                        self.logger.debug("Keeping existing proximity timestamp for {}. Server value was: {}"
                                     .format(mac, server_proximity_ts))

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
            response = requests.patch(BADGE(badge.key), data=data)
            if response.ok is False:
                raise Exception('Server sent a {} status code instead of 200\n{}'.format(response.status_code,
                                                                                         response.text))
        except Exception as e:
            print('Error sending updated badge into to server: {}'.format(e))

    @property
    def badges(self):
        if self._badges is None:
            raise Exception('Badges list has not been initialized yet')
        return self._badges

if __name__ == "__main__":
    logging.basicConfig()
    logger = logging.getLogger('badge_server')
    logger.setLevel(logging.DEBUG)

    mgr = BadgesManagerServer(logger=logger)
    mgr.pull_badges_list()
    print(mgr.badges)
    mgr.pull_badges_list()
    print(mgr.badges)
    for mac in mgr.badges:
        print("Updating: {}".format(mac))
        mgr.send_badge(mac)

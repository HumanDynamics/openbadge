from __future__ import absolute_import, division, print_function
import socket
import requests
import logging
import traceback

from json import load
from urllib2 import urlopen
from server import HUB_ENDPOINT, HUB_ENDPOINT
from urllib import quote_plus


def register_hub():
    """
    Registers current computer as a hub
    Note - hub must be defined in the server (lookup by hostname)
    :return:
    """
    hub_name = socket.gethostname()


def send_hub_ip():
    """
    Updates the hub's IP on the server side
    :return:
    """
    hostname = socket.gethostname()
    encoded_hostname = quote_plus(hostname)
    try:
        my_ip = load(urlopen('http://jsonip.com'))['ip']
    except Exception as e:
        s = traceback.format_exc()
        logger.error('Error getting IP: {} {}'.format(e,s))
        my_ip = '0.0.0.0'

    logger.info("Updating IP for {} ({}) : {}".format(hostname,encoded_hostname,my_ip))
    try:
        data = {
            'ip_address': my_ip,
        }

        logger.debug("Sending update to server: {}".format(data))
        response = requests.patch(HUB_ENDPOINT(encoded_hostname), data=data)
        if response.ok is False:
            raise Exception('Server sent a {} status code instead of 200: {}'.format(response.status_code,
                                                                                         response.text))
    except Exception as e:
        logger.error('Error sending updated badge into to server: {}'.format(e))

if __name__ == "__main__":
    register_hub()
    logging.basicConfig()
    logger = logging.getLogger('badge_server')
    logger.setLevel(logging.DEBUG)
    send_hub_ip()
# Defining end points for backend-server
from __future__ import absolute_import, division, print_function
import settings

SERVER = 'http://'+settings.BADGE_SERVER_ADDR+':'+settings.BADGE_SERVER_PORT+'/'
BADGES_ENDPOINT = '{}badges/'.format(SERVER)


def _badge(x):
    """
    Generates endpoint for a given badge
    :param x:
    :return:
    """
    return '{}{}/'.format(BADGES_ENDPOINT, x)

BADGE_ENDPOINT = _badge

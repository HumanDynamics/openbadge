

SERVER = 'http://localhost:8000/'
BADGES = '{}badges/'.format(SERVER)
# BADGE = lambda x: 'badges/{}/'.format(x)


def _badge(x):
    return 'badges/{}/'.format(x)

BADGE = _badge
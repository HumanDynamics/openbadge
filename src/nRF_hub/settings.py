from __future__ import absolute_import, division, print_function
from dotenv import load_dotenv, find_dotenv
from os.path import join, dirname
import os
import sys

dotenv_path = join(dirname(__file__), '.env')
load_dotenv(dotenv_path)

BADGE_SERVER_ADDR = os.environ.get("BADGE_SERVER_ADDR")
if BADGE_SERVER_ADDR is None:
    print("BADGE_SERVER_ADDR is not set")
    sys.exit(1)

BADGE_SERVER_PORT = os.environ.get("BADGE_SERVER_PORT")
if BADGE_SERVER_PORT is None:
    print("BADGE_SERVER_PORT is not set")
    sys.exit(1)


#!/bin/bash
set -e
if [ "$1" = 'make' ]; then
    cd /app/firmware/nRF_badge/data_collector
    exec "$@"
elif [ "$1" = 'getMAC' ]; then
    cd /app/firmware/nRF_badge/util
    ./getMAC.sh
else
    exec "$@"
fi


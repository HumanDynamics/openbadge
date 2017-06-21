#!/bin/bash
set -e
if [ "$1" = 'make' ]; then
    cd /app/firmware/nRF_badge/data_collector
    exec "$@"
else
    exec "$@"
fi

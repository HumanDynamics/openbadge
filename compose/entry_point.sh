#!/bin/bash
set -e
if [ "$1" = 'make' ]; then
	if [ "$2" = 'test' ]; then
		cd /app/firmware/nRF_badge/data_collector/unit_test
		exec "$@"
	else
		cd /app/firmware/nRF_badge/data_collector
		exec "$@"
	fi
elif [ "$1" = 'getMAC' ]; then
    cd /app/firmware/nRF_badge/util
    ./getMAC.sh
else
    exec "$@"
fi


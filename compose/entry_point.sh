#!/bin/bash
set -e
if [ "$1" = 'make' ]; then
    cd /app/firmware/nRF_badge/data_collector
    exec "$@"
elif [ "$1" = 'make_rssi_scanner' ]; then
    cd /app/firmware/nRF_badge/rssi_scanner
    exec "make" ${@:2} 
elif [ "$1" = 'getMAC' ]; then
    cd /app/firmware/nRF_badge/util
    ./getMAC.sh
elif [ "$1" = 'terminal' ]; then
    cd /app/BadgeFramework
    python ./terminal.py ${@:2}
elif [ "$1" = 'run_all_tests' ]; then
    cd /app/IntegrationTests
    python ./run_all_tests.py ${@:2}
elif [ "$1" = 'run_speed_test' ]; then
    cd /app/IntegrationTests
    python ./manual_badge_speed_test.py ${@:2} ${@:3}
else
    exec "$@"
fi


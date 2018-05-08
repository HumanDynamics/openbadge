#!/bin/bash
set -e
if [ "$1" = 'make' ]; then
    cd /app/firmware/nRF_badge/data_collector
    if [ "$2" = 'test' ]; then
        make -f Maketests "${@:3}"
        #./_test/run_all_tests.o
        #Srm -rf _test
    else
        exec "$@"
    fi
elif [ "$1" = 'make_rssi_scanner' ]; then
    cd /app/firmware/nRF_badge/rssi_scanner
    exec "make" ${@:2} 
elif [ "$1" = 'getMAC' ]; then
    cd /app/firmware/nRF_badge/util
    ./getMAC.sh
else
    exec "$@"
fi


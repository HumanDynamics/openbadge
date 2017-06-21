#!/bin/bash

COMPILE_CMD=""
LOAD_CMD=""
MAKE_CMD="docker-compose run nrf make"

load() {
	${MAKE_CMD} ${LOAD_CMD}
}

compile() {
	${MAKE_CMD} ${COMPILE_CMD}
}

get_mac() {
	echo "### Extracting MAC, please wait"
	docker-compose run nrf getMAC | tail -1 | tee -a macs.log
}

if [[ $# -ne 1 ]]
then
    echo "use: programming_loop.sh [test|prod]"
    echo "Where test loads the tester code, and prod loads the final code"
    exit 1
fi

mode="$1"
case $mode in
    test)
    COMPILE_CMD="badge_03v4_tester"
    LOAD_CMD="flashNew"
    shift # past argument
    ;;
    prod)
    COMPILE_CMD="badge_03v4_noDebug"
    LOAD_CMD="flashCode"
    shift # past argument
    ;;
    *)
            # unknown option
            echo "Unknown option"
            exit 1
    ;;
esac
shift
echo "Using parameters: ${COMPILE_CMD} ${LOAD_CMD}"

# make code
echo "============================================================"
echo "Compiling code- ${COMPILE_CMD}"
echo "============================================================"
compile

COUNTER=1

# program devices
idVendor="1366"
idProduct="1015"
idString="${idVendor}:${idProduct}"

echo
echo "============================================================"
echo "Place new badge. if a badge is already placed, remove it and place it in the programmer again"
inotifywait -q -r -m /dev/bus/usb -e CREATE | while read e
do
    #check if the relevant device is now connected, based on
    #idVendor and idProduct codes
    devIsConnected=$(lsusb | grep "ID ${idString}" | wc -l)

    if [[ ( "$devIsConnected" == 1) ]]
    then
	    echo "============================================================"
	    echo "########## Programming new badge (${COUNTER}) #######"
        load
    	get_mac
	    let COUNTER=COUNTER+1
	    echo "### Done. Place next badge!!!"

    fi
done

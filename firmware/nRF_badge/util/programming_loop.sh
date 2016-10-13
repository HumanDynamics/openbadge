#!/bin/bash

DEV=/dev/ttyACM0
COMPILE_CMD=""
LOAD_CMD=""

load() {
	pushd .
	cd ../data_collector
	make ${LOAD_CMD}
	popd
}

compile() {
	pushd .
	cd ../data_collector
	make ${COMPILE_CMD}
	popd
}

wait_while_device_exists() {
	echo "### Please remove badge (Waiting until device does not exit)"
        echo $DEV
        while [ -e $DEV ]
        do
                sleep 0.1
        done
}

wait_while_device_not_exists() {
	echo "### Place next badge (Waiting until device exists)"
        echo $DEV
        while [ ! -e $DEV ]
        do
                sleep 0.1
        done
}

get_mac() {
	echo "### Extracting MAC, please wait"
	./getMAC | tail -1 | tee -a macs.log
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
echo " ============================================================"
echo " Compiling code- ${COMPILE_CMD}"
echo " ============================================================"
compile


while [ 1 == 1 ]
do
	echo " ============================================================"
	echo "########## Starting new device #######"
	# wait for device to appear
	wait_while_device_not_exists
	load
	get_mac
	wait_while_device_exists
done

#cd ../data_collector
#make flashNew

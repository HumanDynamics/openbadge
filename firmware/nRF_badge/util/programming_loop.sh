#!/bin/bash

DEV=/dev/ttyACM0

flash_tester() {
	pushd .
	cd ../data_collector
	make flashNew
	popd
}

compile_tester() {
	pushd .
	cd ../data_collector
	make badge_03v4_tester
	popd
}

wait_while_device_exists() {
	echo "Waiting until device does not exit"
        echo $DEV
        while [ -e $DEV ]
        do
                sleep 0.1
        done
}

wait_while_device_not_exists() {
	echo "Waiting until device exists"
        echo $DEV
        while [ ! -e $DEV ]
        do
                sleep 0.1
        done
}

get_mac() {
	echo "Extracting MAC"
	./getMAC | tail -1 | tee -a macs.log
}

# make code
compile_tester

while [ 1 == 1 ]
do
	echo "########## Starting new device #######"
	# wait for device to appear
	wait_while_device_not_exists
	flash_tester
	get_mac
	wait_while_device_exists
done

#cd ../data_collector
#make flashNew

#!/usr/bin/env bash

if [[ `adb devices | grep "\tdevice"` != "" ]]
then
    ./run_cordova.sh
    # echo "IT EXISTS"
fi
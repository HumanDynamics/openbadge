#!/bin/bash

set -eo pipefail


if which docker >/dev/null 2>&1
then 
	# docker installed
  if docker images | grep 'android-cordova'  >/dev/null 2>&1
  then
      echo "Cordva container is found"
  else 
	    docker pull kallikrein/android-cordova
	fi
  echo -e "Run the following command on your Terminal\n\n"
  echo "alias cordova='docker run -it --rm --privileged -v /dev/bus/usb:/dev/bus/usb -v \$PWD:/src kallikrein/android-cordova cordova'"
	
fi

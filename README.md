# OpenBadge project #

Welcome to the Open Badge project. For more information, pease visit our
[wiki](https://github.com/HumanDynamics/OpenBadge/wiki)

Additional repositories that are part of this project:
* https://github.com/HumanDynamics/openbadge-hub-py - a python based hub (base station) for pulling data from badges

* https://github.com/HumanDynamics/openbadge-hub - a Javascript/Cordova app that communicates with the badges and stores
 the data to a server
* https://github.com/HumanDynamics/openbadge-server - the backend server for the app

![Badge](/images/v3_badge.jpg?raw=true "Open Badge")

## Docker wrapper ##
The nRF working environment is now available as a Docker container. The following commands have been tested under Ubuntu
linux, but should work under iOS as well (with some modifications, especially to the device names).

Important notes:
1. The badge must be connected to the programmer before starting the docker (using docker-compose run)
2. Docker will mount the project directory under /app inside the container, so the container can see the latest changes
 (no need to build the container after every code change)

Commands:
* docker-compose build : builds the docker container "nrf". You must run this command at least once before calling
 docker-compose run.
* docker-compose run nrf <COMMAND> : where <COMMAND> is a command to run inside the container.
  * When calling make (e.g. make badge_03v4_noDebug flashErase  flashS130 flashAPP), the script will automatically
  change the working directory to the data_collector directory. You can see the different make options by calling:
  "docker-compose run nrf make"
* docker-compose run nrf bash : opens an interactive bash shell
OpenBadge project
=================

Installation
------------

This is a guide to install the library on Ubuntu Trusty 14.04 LTS. 

The first thing you need is to download and install [BlueZ](http://www.bluez.org/download/) libraray with a version number that starts with 5.

    wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.37.tar.xz
    tar xf bluez-5.37.tar.xz
		cd bluez-5.37
    mkdir release
    ../configure --disable-systemd
    make -j4
    sudo make install

Then you need to establish a new Python Virtualenv

    cd Place/Where/You/Store/Envs/
    virtualenv env
    source env/bin/activate

Finally clone the project and install the requirements

    git clone https://github.com/HumanDynamics/OpenBadge.git
    cd OpenBadge
    pip install -r requirements.txt


Cordova installation
--------------------

1. Download and Install [Docker.](https://docs.docker.com/linux/)
2. Run the following command inside the repo `./docker_run.sh`
3. Run the following command `alias cordova='docker run -it --rm --privileged -v /dev/bus/usb:/dev/bus/usb -v $PWD:/src kallikrein/android-cordova cordova'`
4. To create a Cordova Android project run `cordova create PROJECT` where PROJECT is the name of the project you want to create
5. `cd PROJECT` and run `cordova platform add android` to add Android support to your project.


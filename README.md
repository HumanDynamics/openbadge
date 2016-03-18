OpenBadge project
=================

Welcome to the Open Badge project. For more information, pease visit our [wiki](https://github.com/HumanDynamics/OpenBadge/wiki)

![Badge](/images/v3_badge.jpg?raw=true "Open Badge")

Installation
------------

This is a guide to install the libraries required for the badge server on Ubuntu Trusty 14.04 LTS. 

The first thing you need is to download and install [BlueZ](http://www.bluez.org/download/) version 5.29 or higher:

    wget http://www.kernel.org/pub/linux/bluetooth/bluez-5.37.tar.xz
    tar xf bluez-5.37.tar.xz
    cd bluez-5.37
    mkdir release
    ./configure --disable-systemd
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


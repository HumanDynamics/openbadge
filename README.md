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

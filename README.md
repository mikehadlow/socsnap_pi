socsnap_pi
==========

Twitter based social picture device for Raspberry Pi.

[Homepage](http://socsnap.com)

###Install

Socsnap is currently only released as source code, so you'll have to build it yourself.

First, get all the dependencies installed:

    sudo apt-get install liboauth-dev
    sudo apt-get install libcurl-dev
    
Socsnap uses the excellent ZeroMQ messaging library internally. There's no prebuilt binary for this on Raspbery Pi, so you have to build it yourself:
    
    wget http://download.zeromq.org/zeromq-3.2.3.tar.gz
    tar -zxvf zeromq-3.2.3.tar.gz
    cd zeromq-3.2.3
    ./configure
    make
    make install

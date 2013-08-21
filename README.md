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

Get the source code for SocSnap by cloning this repository (you'll need to have git installed to do this):

    sudo apt-get install git
    git clone git@github.com:mikehadlow/socsnap_pi.git
    
Edit the twitter.h file with your keys:

    :TODO
    
Compile and run socsnap:

    cd socsnap_pi
    make
    cd bin
    ./socsnap

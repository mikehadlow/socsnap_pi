SocSnap
=======

Twitter based social picture device for Raspberry Pi.

[Homepage](http://socsnap.com)

###Install

Socsnap is currently only released as source code, so you'll have to build it yourself.

First, install dependencies:

    sudo apt-get install liboauth-dev
    sudo apt-get install libcurl-dev
    sudo apt=get install ncurses-dev
    
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
    
###Kiosk
    
If you want your Raspbery Pi to behave like a kiosk device (so it launches socsnap on startup) edit your /etc/inittab and replace

    1:2345:respawn:/sbin/getty 38400 tty1
    
with

    1:2345:respawn:<dir of socsnap>/socsnap

###Creating Splash Screens

SocSnap shows graphical splash screens as it operates. They are located in the grahics directory. You can edit the png files and then create new 'raw' files with the following command:

    avconv -vcodec png -i splash_main.png -vcodec rawvideo -f rawvideo -pix_fmt rgb565 splash_main.raw


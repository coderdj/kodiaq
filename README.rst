=======================================
kodiaq Data Acquisition Software
=======================================

Maintainer: D. Coderre, University of Bern

Free and open source software: GPL version 3 (see LICENSE.txt for details)


1. Brief 
----------------------------------

kodiaq is a DAQ control program based on CAEN digitizer hardware. It 
allows the configuration of a network of readout PCs controlled by a 
single master in order to facilitate horizontal scaling to reach very 
high readout speeds. 

In addition to the networked high-throughput mode, kodiaq can also 
be run standalone as a single instance and controlled from the command line.

The code is designed to be used with the emo (http://www.github.com/coderdj/emo)
web interface. 

Please see the full documentation at http://xenon1t.github.io/kodiaq/

2.0 Installtion - The super easy way
-------------------------------------

The easiest way to install kodiaq is to use the install script. This will automatically check out the newest version of everything and install it onto your PC. It only works in Ubuntu and has only been tested on 14.04 server. If you are using a different distribution you will have to install manually.


2. Installation - Lite Version
-----------------------------------------

Prerequisites:
   * CAEN A2818 or A3818 installed and configured
   * CAENVMElib installed, included in kodiaq/caen. Follow instructions in the readme
   * libncurses5-dev, also libmongoclient (for mongodb support) or libpbf (for file support)
   * For compilation (on ubuntu) build-essential, autoconf, automake, libtool
   * For compression: libsnappy-dev

To install the minimum required prerequisites on Ubuntu 14.04 run the following command::

    sudo apt-get install build-essential automake autoconf libtool libncurses5-dev libsnappy-dev                

Steps to build and run the standalone module:
   1. ./configure --enable-lite 
   2. make
   3. cd src/slave
   4. ./koSlave
   
Note that the file DAQConfig.ini must be present in src/slave/DAQConfig.ini. If it is anywhere else you can specify the location with the '-i' option, like ./koSlave -i "myconfig.ini". An example config file sits at::
   
    cp src/master/data/RunModes/DAQOptionsMaster.ini 

Edit this file with the settings specific to your setup. Note that an expert should edit this file while a normal user should simply choose between several pre-made files depending on the operation mode.

3. Installation - Full version
---------------------------------------------

Prerequisites:
   * For user module see above
   * Master module
      * For DDC10 online high energy veto support, libtcl8.5 and libexpect are required.
      * libmongoclient (also requires boost). See installation instructions below.
   * For slave module
      * CAENVMElib as well as an A2818/A3818 PCI card with its driver installed. 
      * libsnappy, libpthread, libprotoc (dev versions)
    

If you are using the web interface to control the DAQ then the Mongodb libraries are required.

Steps:
     1. Install libmongoclient. Steps for ubuntu 14.04::
     		
     		sudo apt-get install scons libboost-all-dev python2.6 git
     		git clone https://git@github.com/mongodb/mongo-cxx-driver.git 
		cd mongo-cxx-driver
		scons --full --use-system-boost install-mongoclient

     2. Cofigure with ./configure. There are some possible options.
         * --enable-master to compile with master program
	 * --enable-slave to compile with slave program
	 * --enable-ddc10 to compile with support for DDC module. Only activated if koMaster is also compiled.
	 * --enable-all to compile master (with ddc10 support), slave, and user programs
	 * --enable-lite to compile a lite version of the software that is suitable for standalone systems	
	Note: if configure doesn't work you may have to run ./autogen.sh to regenerate the configuration scripts.
     3. make
     4. The executables for master/slave programs should be
     available in src/*.
     
   
Contact
---------

Please contact xe-daq@lngs.infn.it with any questions on configuration
or installation of the DAQ system.


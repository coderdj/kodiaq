=======================================
kodiaq Data Acquisition Software
=======================================

0. Copyright 
--------------------------------

All original kodiaq content copyright 2013 the University 
of Bern.

Caen libraries and drivers included for convenience and are copyright Caen. 
Find the most recent drivers at www.caen.it.

Source and compiled code to be distributed only with 
authorization according to the XENON1T MOU.

Author(s)
	* Daniel Coderre, LHEP, University of Bern   
   	* Lukas Bütikofer, LHEP, University of Bern

1. Brief 
----------------------------------

kodiaq is a networked DAQ software inferface for XENON1T.
It is designed to read CAEN V1724 digitizers in parallel
from multiple reader PCs in order to achieve high speeds
without sacrificing synchronization or control. 

The program can either be run as a single instance standalone program 
for a one-machine DAQ deployment (a test stand for example) or in full 
networked mode.

In networked mode a single dispatcher controls an unlimited number of
reader PCs. The dispatcher can either be controlled by simple text commands
from the console or by the XENON1T online monitoring web interface.

Please see the full documentation at http://xenon1t.github.io/kodiaq/

2. Installation - Lite Version
-----------------------------------------

Prerequisites:
   * CAEN A2818 or A3818 installed and configured
   * CAENVMElib installed, included in kodiaq/caen. Follow instructions in the readme
   * libncurses5-dev, also libmongoclient (for mongodb support) or libpbf (for file support)
  
Steps to build and run the standalone module:
   1. ./configure --enable-lite 
   2. make
   3. cd src/slave
   4. ./koSlave
   
Note that the file DAQConfig.ini must be present in src/slave/DAQConfig.ini. If you haven't done this yet you can use the following line::
   
    cp src/master/data/RunModes/DAQOptionsMaster.ini src/slave/DAQConfig.ini

Then edit this file with your settings.

3. Installation - Full version
---------------------------------------------

Prerequisites:
   * For user module see above
   * Master module
      * For DDC10 online high energy veto support, libtcl8.5 and libexpect are required.
   * For slave module
      * CAENVMElib as well as an A2818/A3818 PCI card with its driver installed. 
      * libsnappy, libpthread, libprotoc (dev versions)
      * For mongodb support, libmongoclient. Note that this also requires the boost libraries.

If you are using the web interface to control the DAQ then the Mongodb libraries are required.

Steps:
     1. Cofigure with ./configure. There are some possible options.
         * --enable-master to compile with master program
	 * --enable-slave to compile with slave program
	 * --enable-ddc10 to compile with support for DDC module. Only activated if koMaster is also compiled.
	 * --enable-all to compile master (with ddc10 support), slave, and user programs
	 * --enable-lite to compile a lite version of the software that is suitable for standalone systems	
     2. make
     3. The executables for master/slave programs should be
     available in src/*.The folder klite contains shortcuts to the lite
     version executable and the initializeation file. Please refer to
     the online documentation to configure this file.
     
   
Contact
---------

Please contact xe-daq@lngs.infn.it with any questions on configuration
or installation of the DAQ system.


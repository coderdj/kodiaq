=======================================
kodiaq Data Acquisition Software
=======================================

0. Copyright 
--------------------------------

All original kodiaq content copyright 2013 the University 
of Bern.

Dependencies such as CAENVME libraries and mongodb are 
copyrighted and/or trademarked by their respective owners.

Source and compiled code to be distributed only with 
authorization according to the XENON1T MOU.

Author(s): Daniel Coderre, LHEP, University of Bern           

1. Brief 
----------------------------------

kodiaq is a networked DAQ software inferface for XENON1T.
It is designed to read CAEN V1724 digitizers in parallel
from multiple control PCs in order to achieve high speeds
without sacrificing synchronization or control. 

The program is split into 3 parts. The koSlave program 
should run on each readout PC. Additionally one instance
of the koMaster program should run on a PC that is 
accessible by network (preferably locally) to all of the
slaves. The connectivity between master and slave must be
defined prior to starting the daemons. The koUser program
can be installed on any linux system and can be used to 
connect to koMaster from any PC with a network connection
to the master, provided the appropriate ports are open.

Please see the full documentation at http://xenon1t.github.io/kodiaq/

2. Installation - User Client
-----------------------------------------

Follow these instructions if you just want the user client
to login to the DAQ.

Prerequisites:
    * This code has only been tested and will only be supported on
    Linux. Compilation on any other OS is accidental.
    * You need the standard build libraries, such as gcc and make (on
    Ubuntu these are provided in build-essential)
    * The ncurses libraries (development version) are also needed:
    libncurses, libmenu, libform libtinfo

Steps to build the code:
    1. ./configure
    2. make
    3. Resize your terminal to 132x43 at least
    4. ./kodiaq_client
    5. Login and have fun!

3. Installation - Full version
---------------------------------------------

Prerequisites:

     * For user module see above
     * For master module:
       * For DDC10 online high energy veto support, libtcl8.5 and
       libexpect are required.
     * For slave module:
       * CAENVMElib as well as an A2818/A3818 PCI card with its driver
       installed. 
       * libsnappy, libpthread, libprotoc (dev versions)
       * For mongodb support, libmongoclient. Note that this also
       requires the boost libraries.

Steps:

     1. Cofigure with ./configure. Possible options:
        * --enable-master : compile with master program
	* --enable-slave : compile with slave program
	* --disable-user :  do not compile UI
	* --enable-ddc10 :  compile with support for DDC module. Only
	activated if koMaster is also compiled.
	* --enable-all : compile master (with ddc10 support), slave,
	and user programs
	* --enable-lite : compile a lite version of the software that
	is suitable for standalone systems	
      2. make
      3. The executables for master/slave/user programs should be available
      in src/*. A shortcut to starting the UI is in the top directory.
      The folder klite contains shortcuts to the lite version executable and
      the initializeation file. Please refer to the online
      documentation to configure this file.
   
   
Contact
---------

Please contact xe-daq@lngs.infn.it with any questions on configuration
or installation of the DAQ system.


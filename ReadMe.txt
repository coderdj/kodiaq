#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

*   *   ***     ***      ***         *          ***
*  *   *   *    *  *      *         * *        *   *
* *   *     *   *   *     *        *   *      *     *
**    *     *   *    *    *       *     *     *     *
* *   *     *   *    *    *      *********    *  *  *
*  *   *   *    *   *     *     *         *    *   *
*   *   ***     ***      ***   *           *    ***  *

#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#*#

=========================================================
1. Brief 
=========================================================

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

==========================================================
2. Installation
==========================================================

Prerequisites:
                - CAEN PCI card A2818/A3818 and firmware
		- CAENVMElib installed and reachable 
		- mongodb C++ library installed 
		- Various libraries may be needed if they
		  are not already on the system. For 
		  example: ncurses, snappy, and boost_thread

1. Compile the XeDAQ library:
                cd ${kodiaq}/src/common
		make

2. Compile and run the koSlave programs on slave PCs
   Hint: you may want to do this in a detached screen
                cd ${kodiaq}/src/slave
		make
		./koSlave

3. Compile and run the koMaster program on the master PC
   Hint: you may want to do this in a detached screen
                cd ${kodiaq}/src/master
		make
		./koMaster

4. Compile and run the koUser interface on any control PCs
   Hint: this works for linux. Other platforms may work
         with some tuning but are not supported
	        cd ${kodiaq}/src/koUser
		make
		./koUser
   
   Some notes on the user program:
                - A terminal size of 132x43 (minimum) is
		  required. If your laptop screen is too
		  small, buy a real one.
		- Logging into or out of the UI has no impact
		  on the DAQ. 
		- In case of connectivity problems, check that
		  you have the hostname and ports correct and 
		  that the ports are reachable from your location.
		  For security reasons it is recommended to log
		  into the DAQMaster PC via SSH and log into 
		  the DAQ from there. 
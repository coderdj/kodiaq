=============================
Deployment of the Full System
=============================

kodiaq consists of three parts:

  * The master module runs on one computer and manages the entire system
  * Slave modules run on each readout PC and drive the boards
  * User modules are used to log into the master and steer the DAQ

All of these modules are required for running. Installation and
configuration of the modules will be described in this section.

All three modules require the libXeDAQ, which is compiled
automatically no matter which modules are added. The libXeDAQ contains
shared code for the network protocols, options file handling, logging,
and other assorted helper functions. The source for this library is
found in src/common.

Building the software is done using autotools. Line-by-line
instructions are given below for each module, however in general the
software is simply compiled by calling: ::

  ./configure
  make

Some of the modules have unique dependencies not shared by the others.
For example the master and slave modules require the mongodb driver
while the user does not. End users may not want to install mongodb
just to use the DAQ UI. Therefore the configure script has several
flags that tell kodiaq which modules to include.

+--------------------+----------------------------------------+
| Argument           |  Description                           |
+====================+========================================+
| {no arguments}     | Compile the user module only           |
+--------------------+----------------------------------------+
| --disable-user     | Do not compile the user module         |
+--------------------+----------------------------------------+
| --enable-slave     | Compile the slave module               |
+--------------------+----------------------------------------+
| --enable-master    | Compile the master module              |
+--------------------+----------------------------------------+
| --enable-ddc10     | Enable ddc-10 support for master       |
+--------------------+----------------------------------------+
| --enable-lite      | Compile standalone slave module        |
+--------------------+----------------------------------------+

In all cases the shared common directory is compiled. For each option
enabled the configure script automatically checks that the unique
dependencies for each module are installed. The make script then
compiles only the activated modules.

Summary of Dependencies per Module
----------------------------------

+--------------------+----------------------------------------+
| Module             |  Dependencies                          |
+====================+========================================+
| common             | | * c++ compiler (i.e. gcc)            |
+--------------------+----------------------------------------+
| user               | | * libncurses-dev                     |
|                    | | * must contain libmenu, libform, and |
|                    | |   libtinfo                           |
+--------------------+----------------------------------------+
| slave/lite         | | * libpthread-dev                     |
|                    | | * libCAENVME                         |
|                    | | * libsnappy-dev                      |
|                    | | * libmongoclient (if mongodb output  |
|                    | |   is required)                       |
|                    | | * libprotobuf-dev                    |
+--------------------+----------------------------------------+
| master             | | * libmongoclient (if mongodb output  |
|                    | |   is required)                       |
+--------------------+----------------------------------------+
| ddc10              | | * libtcl8.5                          |
|                    | | * libexpect                          |
+--------------------+----------------------------------------+


Deployment, Step by Step
------------------------

Here is a general outline of the steps needed to deploy a system. More
specific information on each step will be found in later sections.

   1. Hook up the hardware. A readout PC (from now on called a slave)
      needs a CAEN A2818 or A3818 PCI(e) card. Up to 8 digitizers can
      be hooked up via optical link. You also need one V2718 crate
      controller with output '0' fanned to the s-in of all digitizers.
   2. Install the koSlave module on each slave PC. Start the daemon in
      a detached screen.
   3. Find a spot for the master module to live. It is suggested to
      use an independent PC as dispatcher but also possible to install
      it alongside a slave. Install the master here. Make sure the
      linkage between master and slave is defined correctly. Start the
      master daemon in a detached screen.
   4. Write your options file. Use the default as a base. You will
      need to define the physical electronics setup exactly.
   5. Log into the UI and start the DAQ!

The specifics on installing the slave and master, as well as how to
write an options file are given in the next sections.

koSlave
---------

The kodiaq slave module is the meat of the acquisition system. It
contains all the classes for controlling the V1724 digitizers as well
as pushing the data stream to mongodb. The koSlave module is a daemon
that runs all the time and listens for connections to the master.


There are several dependencies that must be met for the slave module
to work.
  
   * You need a PC with a CAEN A2818 or A3818 PCI(-e) card installed
     and also the proper drivers.
   * There are several library dependencies
      * CAENVMElib (available from CAEN, this is a closed-source
        library)
      * mongodb greater than version 2.55 (earlier versions can work with
        a slight modification to the source code). Specifically
	libmongoclient is needed. 
      * libsnappy for on-the-fly compression
      * libpthread for parallel processing
      * Google protocol buffers libraries (called libprotobuf-dev on
        ubuntu)
      * Normal build libraries (build-essential package on ubuntu)
     
Checkout the code from github. To compile use the following steps: ::
    
      cd kodiaq (top-level directory)
      ./configure --enable-slave (optionally --disable-user if the UI is not needed)
      make

The connection to the master must also be defined. Right now this is
hard-coded in the koSlave.cc file. The line
fNetworkInterface.Initialize(...) must be edited to give the proper
address of the master. It is forseen to put this in a config file.

To start the slave just run koSlave, preferably in a detached screen.
The program will automatically scan the master and check to see if
it puts the network up. If so the slave will connect automatically.

In case that the slave loses connection it will stop the DAQ and
return to this idle state, waiting for another connection to the
master. It should be fine just to leave it on indefinitely if there
are no crashes.


koMaster
---------

The master should be installed somewhere with a reliable network
connection to the slaves. It can also run on a slave PC if necessary
since it uses very few resources. Two open ports are required for
communication with slaves and two more for communication with clients.
These can be any ports but they must be defined in koSlave and in the
UI. 

To install the master, mongodb must be installed (libmongoclient).
Additionally, if support for configuring the DDC-10 high energy veto
module is required then libddc is also needed. 

To build use the following: ::

    cd kodiaq
    ./configure --enable-master (--enable-ddc10 for ddc10 support)
    make
    
The executable is in src/master/koMaster. This should also be run in a
detached screen and can be left on more or less indefinitely unless
there are issues.

The DDC-10 module uses telnet and requires libtcl8.5 and libexpect.

Run Modes
^^^^^^^^^^^^^
The operational modes for the DAQ are defined in
src/master/data/RunModes.ini. This file is simply a list where the
first entry is a string with a run mode identifier and the second
entry is the path to the .ini file for that mode. This file can be
edited while the master is running. For an exampe .ini file take a
look in src/master/data/RunModes/DAQOptionsMaster.ini. 


Deployment of the Standalone Slave Module
-----------------------------------------

It is also possible to deploy a standalone module for running small
DAQ systems. This consists of the slave module which is steered via a
text-based interface on the console.

To deploy the standalone module, make sure all the same dependencies are met as
for the slave module described previously. Then build using the
following commands: ::

    cd kodiaq
    ./configure --enable-lite
    make

Assuming you are successful, the koSlave executable should be
installed with a special flag that allows local operation. To operate
this module, use the script in the klite directory: ::

    cd klite
    ./StartDAQ.sh

This will start the DAQ with the options defined in
klite/DAQConfig.ini. Please note that editing of the DAQConfig.ini
file is intended for expert users only. The available parameters in
this file are described later in this documentation.

The lite program has only two options. The DAQ is started with the 's'
key. Pressing the 'q' key at any time will shut down the DAQ and stop
the program.

Currently the standalone module requires a mongodb database and the
mongo C++ driver. It is anticipated that the ability to write directly
to files (which would allow the mongodb driver dependency to be
removed) will be added in a future update.



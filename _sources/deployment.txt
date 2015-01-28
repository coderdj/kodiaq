=============================
Deployment of the Full System
=============================

kodiaq consists of two parts:

* The master module runs on one computer and manages the entire system
* Slave modules run on each readout PC and drive the boards

Both of these modules are required for running in networked mode. Installation and
configuration of the modules will be described in this section.

Both modules require the libkodiaq, which is compiled
automatically no matter which options are given. The libkodiaq library contains
shared code for the network protocols, options file handling, logging,
and other assorted helper functions. The source for this library is
found in src/common.

Building the software is done using autotools. Line-by-line
instructions are given below for each module, however in general the
software is simply compiled by calling: ::

  ./configure --enable-all
  make

Some of the modules have unique dependencies not shared by the others.
For example the master module require the mongodb driver
while the slave module does not (if no mongodb output is needed). 
The configure script has several
flags that tell kodiaq which modules to include.

.. cssclass:: table-hover
+--------------------+----------------------------------------+
| Argument           |  Description                           |
+====================+========================================+
| {no arguments}     | Compile the lite module only           |
+--------------------+----------------------------------------+
| --enable-slave     | Compile the slave module               |
+--------------------+----------------------------------------+
| --enable-master    | Compile the master module              |
+--------------------+----------------------------------------+
| --enable-ddc10     | Enable ddc-10 support for master       |
+--------------------+----------------------------------------+
| --enable-lite      | Compile standalone slave module        |
+--------------------+----------------------------------------+
| --enable-all       | Compile all modules                    |
+--------------------+----------------------------------------+

In all cases the shared common directory is compiled. For each option
enabled the configure script automatically checks that the unique
dependencies for each module are installed. The make script then
compiles only the activated modules.

Summary of Dependencies per Module
----------------------------------

.. cssclass:: table-hover
+--------------------+----------------------------------------+
| Module             |  Dependencies                          |
+====================+========================================+
| common             | | * c++ compiler (i.e. gcc)            |
+--------------------+----------------------------------------+
| slave/lite         | | * libpthread-dev                     |
|                    | | * libCAENVME                         |
|                    | | * libsnappy-dev                      |
|                    | | * libmongoclient (if mongodb output  |
|                    | |   is required)                       |
|                    | | * libprotobuf-dev (if file output    |
|                    | |   is required                        |
+--------------------+----------------------------------------+
| master             | | * libmongoclient                     |
|                    | | * libboost-all-dev                   |
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
   be hooked up in daisy chain via optical link. You also need one V2718 crate
   controller with LEMO output '0' fanned to the s-in of all digitizers.
2. Install the koSlave module on each slave PC. Start the daemon in
   a detached screen.
3. Find a spot for the master module to live. It is suggested to
   use an independent PC as dispatcher but also possible to install
   it alongside a slave. Install the master here. Make sure the
   linkage between master and slave is defined correctly. Start the
   master daemon in a detached screen. Follow the on-screen prompt to bring up the network.
4. Write your options file. Use the default as a base. You will
   need to define the physical electronics setup exactly. If using a web database you can 
   use the UI to define your options.
5. Log into the web database and start the DAQ. Or start the DAQ from the command line using
   the master module (less cool but works).

The specifics on installing the slave and master, as well as how to
write an options file are given in the next sections.

General network structure
-------------------------

It's a good idea to know a bit about how the networked mode actually works since the options
will make a little more sense. Basically you have a single instance of koMaster which acts as a dispatcher
and communicates with all the slave nodes and with an outside database. All the slave nodes connect to
this dispatcher. The connection is done over two ports. One port is interactive and is used for commands
and responses from the dispatcher to the slaves and vice versa. The other port is used to transmit a
constant stream of data from the slaves to the dispatcher which reports the rate, run mode, status, etc.

The master additionally supports independent operation of multiple detectors over the same dispatcher. Each
detector is basically its own independent deployment but they can also be operated together. Detectors are
defined in the master config file and each detector operates over two unique ports.

Within a single detector the slaves are organized such that each has its own ID number and name. The ID numbers
are integers and are used to define different options for different slaves. In the .ini file you can send an
option to a specific slave by prefacing a line with '%n' where 'n' is the ID number of the slave. The name of
each slave can be anything and is used in status reporting.

If you want to use the DAQ with the web interface you need a mongodb database set up. The connectivity to this DB
is defined in the koMaster section below.

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
   * mongodb cxx driver greater than version 2.6. Specifically
     lbmongoclient is needed. 
   * libsnappy for on-the-fly compression
   * libpthread for parallel processing
   * libpbf for file output
   * Normal build libraries (build-essential package on ubuntu) as well as automake, autotools, and libtool
   
.. note:: It is possible to compile without libmongoclient and/or libpbf. If you compile without libmongoclient you will not be able to output to a mongodb. If you compile without libpbf you will not be able to write to file. Depending on your installation one of these may be fine. You will be notified at the end of the configure script which output forms are available for your installation.

Assuming you want to install everything, do it as follows.::

      apt-get install libsnappy-dev build-essential git-core scons libprotobuf-dev protobuf-compiler libboost-all-dev automake autotools libtool

Now make a directory for the code and install libpbf::

      mkdir kodiaq
      cd kodiaq
      git clone https://github.com/coderdj/libpbf.git libpbf
      cd libpbf
      make
      make install 
      cd ..

We also want a newer version of mongodb so we can install that as well.::

      git clone https://github.com/mongodb/mongo-cxx-driver.git mongodb
      cd mongodb
      git checkout 26compat    #currently for 2.6 compatible driver
      scons --use-system-boost --full install-mongoclient
      cd ..

Since mongo seems to change procedures and requirements with nearly every release you may prefer to check their most recent documentation.

Checkout the DAQ code from github as follows.::

     git clone https://github.com/XENON1T/kodiaq.git kodiaq

Now compile the CAEN software. There is a copy of this bundled with kodiaq or you can get the most recent versions from http://www.caen.it. The CAEN software is in kodiaq/caen. You need to install CAENVMElib and the driver for your PCI card (A2818 or A3818). The instructions for installation are given in the README files within these directories. 
     
Everything should be in place so you can now compile the kodiaq package itself.::
    
      cd kodiaq (top-level directory)
      ./configure --enable-slave 
      make

The connection to the master must also be defined. This is defined in the file src/slave/SlaveConfig.ini,
which looks as follows:

    COM_PORT 2002
    DATA_PORT 2003
    NAME xedaq01
    ID 1

This examples defines slave 'xedaq01' with ID '1' to use ports 2002 and 2003. These ports must
also correspond to a detector defined in src/master/MasterConfig.ini (see below) or the slave will
never be contacted by the network and will just listen perpetually.

To start the slave just run koSlave, preferably in a detached screen.
The program will automatically scan the master and check to see if
it puts the network up. If so the slave will connect automatically.

In case that the slave loses connection it will stop the DAQ and
return to this idle state, waiting for another connection to the
master. It is designed to be left running indefinitely regardless of whether the DAQ is used or not.


koMaster
---------

The master should be installed somewhere with a reliable network connection to the slaves. It can also run on a slave PC if necessary. Two open ports are required for communication with slaves and two more for communication with clients. These can be any ports but they must be defined in koSlave and in the UI. 

To install the master, the mongodb C++ driver (version >=2.6) must be installed (libmongoclient). You can follow the directions in the appropriate section of the koSlave installation instructions. 

To build use the following: ::

    cd kodiaq
    ./configure --enable-master (--enable-ddc10 for ddc10 support)
    make
    
The executable is in src/master/koMaster. This should also be run in a
detached screen and can be left on more or less indefinitely unless
there are issues.

The options for the master are stored in a configuration file in src/master at MasterConfig.ini. The options look as follows\
:

    MONITOR_DB online        // name of monitoring database
    MONITOR_ADDR xedaq01     // address of mongodb server (hostname or IP)
    DETECTOR tpc 2002 2003   // define detector 'tpc' to communicate over ports 2002 and 2003
    DETECTOR muon_veto 2004 2005  // define detector 'muon_veto' to communicate over ports 2004 and 2005

The detector names allow the web interface to send specific commands to specific detectors only.

The DDC-10 module uses telnet and requires libtcl8.5 and libexpect. 

Run Modes
^^^^^^^^^^^^^
The operational mode for the DAQ can be pulled from an online mongodb database 
or defined in a local file (src/master/DAQConfig.ini). The options with explanations are
found later in this document or in src/master/data/RunModes/DAQOptionsMaster.ini. 





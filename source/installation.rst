==============================
Installation of the standalone reader
==============================

Fetching the source code
-------------------------

Our source code is hosted on `github
<https://github.com/XENON1T/kodiaq>`_. In addition to checking out the
code, you can also use github
to report bugs and issues and keep track of the current development.
The repository is password-protected and an authorized github account
is required for access.

Assuming you have repository access::

    git clone https://github.com/XENON1T/kodiaq.git

You will have to provide your login credentials. This will then create
the file 'kodiaq' in whatever directory you ran the command in. 

Prerequisites for installation
-------------------------------

This software supports reading from Caen V1724 digitizers. Connection to
a Caen V1724 is mediated by either an A2818 or A3818 PCI(e) card. This card 
should be installed in your computer and configured according to the manufacturer's
instructions. See http://www.caen.it for details.

For convenience the PCI card drivers are included in the caen folder in the 
kodiaq top-level directory. However it is recommended to check the CAEN webpage for
updated versions. 

In addition to the PCI card driver you also need CAENVMElib. This is included in 
the kodiaq/caen folder as well and can be installed using the instructions in that 
directory.

Software requirements are relatively lightweight and consist of the normal build tools. 
You will need g++, make, and autotools. Additionally you need the ncurses library
for the display. On Ubuntu these can be installed by::

  sudo apt-get install build-essential automake autoconf libtool libncurses5-dev

If you want to write data as well as reading is (generally the point of a DAQ) you will
need some output method installed. Currently kodiaq supports writing to either a mongodb
database or pbf files. 

To write to mongodb you will need to install a mongodb database somewhere to write to. 
The database should run at least mongodb 2.6. Connection to the database is defined in 
the options file. The DAQ program also needs the mongodb c++ driver (26-compat version 
as of 9.9.2014), which can be obtained from http://www.mongodb.com. The readme file has 
step by step instructions on installing this driver.

To write to files you need to install libpbf from https://github.com/XENON1T/libpbf.

How to install
--------------

Assuming you meet all the requirements installation should just be ::
  
  ./configure --enable-lite
  make

This will install the standalone executable into src/slave. The configure script should
automatically detect your mongodb and/or libpbf installations. The scripts output will 
tell you which output format(s) will be supported by your kodiaq installation.

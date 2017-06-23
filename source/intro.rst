=======
Welcome
=======

Welcome to the documentation for the kodiaq data acquisition software.
If you are a general user of the DAQ and just want an overview of how
to control it then you're in the right place. The first few pages of
these docs are meant as a quick start guide and should get you up and running asap.


Scope of the program
--------------------

kodiaq is the hardware half of a data acquisition system suitable for
high-rate particle physics use. It uses CAEN V1724 digitizers and
reads out complete waveforms to a temporary buffer (mongodb). 
The software half of the DAQ is the cito event builder, which reads 
the data from the buffer and builds events.

kodiaq is parallelized and scalable in order to achieve almost
arbitrarily fast throughput speeds. A central dispatcher (the master)
controls several client modules for readout (the slaves). The master
and slave daemons are always running and require no intervention from
the user. Users can control the DAQ with a simple text-based interface
to the master daemon or with a full featured web interface. The web 
interface is included in a separate repository. Communication between 
the web interface and kodiaq is mediated by an online database.

In addition to the networked mode it is also possible to run kodiaq in 
a standalone mode. In this case the dispatcher (and web interface) are 
omitted in favor of a simple terminal-based control interface directly 
with a single instance of the slave program.

This guide covers how to install and log into the DAQ.

Use Guidelines
--------------

kodiaq is released under the GPLv3 License. See the text here: 
https://github.com/XENON1T/kodiaq/blob/master/LICENSE.txt

kodiaq was developed at the University of Bern and is maintained
at the University of Freiburg. If you have questions, comments, want to help, 
or want to buy us a beer we would love to hear from you. Complaints? 
Fix it first. ;-)

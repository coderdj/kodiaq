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
the user. Users control the DAQ by logging into the master using a
control interface. Additionally a web interface exists which ties the
kodiaq (reader), cito (event builder), and wax (data processor)
together. The web interface provides basic end user control and
monitoring.

This guide covers how to install and log into the DAQ.

Use Guidelines
--------------

kodiaq is currently closed-source and its distribution is limited.
However, many parts of the code are general enough to be reused for 
other data acquisition purposes. If you are interested in the system
feel free to drop us a line.

kodiaq is developed at the University of Bern. If you have questions, 
comments, want to help, or want to buy us a beer we would love to hear from you.
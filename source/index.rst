.. kodiaq documentation master file, created by
   sphinx-quickstart on Thu Mar  6 17:35:36 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to kodiaq 
======================================================

.. image:: ../source/bear.png
    :align: left
    :alt: alternate text
    
kodiaq is a program designed to read out and format data from CAEN V1724 digitizers. 
It runs in both networked and standalone mode. In standalone mode a single instance of
the readout program reads data to either a database or a local file from digitizers connected 
to the same PC. In networked mode a dispatcher controls several PCs which read data out 
from digitizers connected to them and deposit it into a central database. Networked mode 
is suitable for high-speed deployments where parallelization is needed to overcome the 
speed limit imposed by the CONET-2 interface. 

These docs are intended for users interested in deploying the system and provide step by step 
instructions as well as an explanation of the program's features.

Our code is hosted on github so be sure to check out the repository
`here <https://github.com/XENON1T/kodiaq.git>`_. You might also be
interested in checking out the `wax project
<http://tunnell.github.io/wax/index.html>`_. 

Contents 
================================

.. toctree::
   :maxdepth: 2
   
   intro
   webinterface
   installation
   usage         
   deployment 
   options 
   protocols
   monitoringdatabase
   jargon
   help



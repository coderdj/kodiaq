========================
Using the Standalone Reader
========================

The standalone reader exists in src/slave. An options file in this directory 
called DAQConfig.ini should be defined to set your run options. This is explained 
in detail in the options section of this documentation.

Assuming DAQConfig.ini is configured you should be able to run the code like this::
  
  cd src/slave
  ./koSlave

Starting and stopping the run is done using the keyboard prompts. 

Special options for standalone mode
-----------------------------------

Most of the DAQ options for the DAQConfig.ini file are the same in standalone
and networked mode. The differences are listed here.

  * Run names cannot be defined with a '*' wildcard. Run names must be given 
    absolute names. The same holds for file output paths.
  * The '%' operator to define a certain slave node is not necessary and 
    must be omitted. 

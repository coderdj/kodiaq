========================
DAQ Options
========================

DAQ options are stoed in .ini files. There are two types of DAQ options
files. Static options are defined for the master and slave programs
upon program start and remain the same for the life of the program.
These files deal with master/slave connectivity and database addresses.
Configuration of these files is defined in the 'deployment' section.

This section deals with the 'run modes' ini files that define the settings
for a particular run. These settings are reset each run and are designed
to chage from run to run.

The .ini files are in json format. This allows the files to be easily
stored in a MongoDB collection in the full system deployment, or simply edited as
text files for a local deployment.

The design of the program is that these files will be defined by
developers while end users will have to option to pick from several
pre-defined modes. 

It is important to not edit these files unless you know what you are
doing. There is only a limited amount of sanity checking done on the
input of these files and improper editing could break the DAQ. If you
are fortunate it will break in a way that is obvious, if you are
unlucky you might not even notice. Particularly the VME options are
passed to the boards *directly as printed in the order they are printed* 
in the .ini file. Improper setting of any of these options is very 
difficult to debug.

That said, if you are a power user please read on for an explanation 
of the options available.

General Syntax
----------------

The .ini file is mostly passed verbatim to the slaves when a run mode is
selected. However a limited amount of parsing is done.

Options are given in JSON format. If the ini file is not valid JSON
it will be rejected. You can check if you write valid JSON by using a 
JSON validator, for example www.jsonlint.com. In our system we allow our
web view to validate the JSON before putting it into the database.
 
Option names must exactly match one of the pre-defined
options (case sensitive). The arguments can be strings, bools, or numeric.
Additionally, nested objects are allowed and are used for register, link, 
and board definition.

In certain cases there are options that should only go to one slave
PC. An example of this is the electronics definitions. The readout
electronics are hard-wired to the slave PCs and the cabling must be
defined in the options file. Most options allow for this directly 
within the format. 


Electronics Definitions
------------------------

CAEN V1724 digitizers can be read out in one of two ways: either via a
single optical link to a V2718 crate controller, which reads the
boards out through the VME backend, or via the optical links on the
front of the V1724 digitizers themselves. When reading out through the
crate controller, rates are somewhat limited, presumably either
through latency in the VME backplane or a delay introduced when
processing the data in the crate controller module (this is a
closed-source, commercial system so we are not sure which). Maximum
throughput in this mode is around 30 MB/s depending on data type and
options. Therefore it is recommended to always connect the DAQ in
"daisy-chain mode" if possible. Though reading out via the crate
controller is possible with kodiaq, this guide will only cover configuring
the DAQ for readout through the optical links on the V1724 boards
themselves (where speeds of up to 90MB/s have been observed).

When connecting the boards in "daisy-chain" mode, optical cables are
used to hook the boards in series to a single optical input on the
A2818 or A3818 PCI(e) card. Up to 8 digitizers can be connected to a
single optical port, meaning an A2818 supports up to 8 digitizers
while an A3818 can support up to 32. Keep in mind the readout speed of
a single slave is still capped at 70-90 MB/s (depending on options),
so it is generally advantageous to spread the digitizers out over
several machines. 

The software-side configuration is a bit counter intuitive at first
for daisy-chain mode, but it follows the convention set in the CAENVME
library. Two variables must be defined, a LINK and a BASE_ADDRESS
(board). When the boards are connected in daisy-chain mode, each
digitizer has its own LINK defined, as opposed to when the boards are
read out via the crate controller, in which case they share a link.

The syntax for defining a link is ::

  {
   "links": [
      {"type": "V2718", # or "V1718" if you use USB
       "reader": Int,   # ID of reader
       "crate": Int,    # CAEN crate number
       "link": Int      # Link index (physically on the optical card)
      },
      ... #more links
      ]
   }
Keep in mind that the CAEN software was designed first for reading out
via the crate controller and extended to read out via the V1724
optical link directly and this will make a bit more sense.

    * link defines a JSON list field containing a sub-dictionary for 
      each link definition.
    * "type" is the type of optical link. We will always use
      'V2718' type links, even when reading out via the V1724. If you have
      a system that uses the USB adapter you might want "V1718". 
    * "link" (in the sub-object) is the ID of the optical port the cable is
      hooked up to. This is always 0 for the A2818 since it just has one
      port but can be 0-3 for the A3818. 
    * "crate" is the crate ID. In daisy chain mode each digitizer
      gets its own crate ID. Of course when reading out through a
      crate controller this made more sense, since there was one crate
      controller per crate. When reading out via the digitizer itself
      just think of this as an integral identifier. Note when reading out 
      via optical link only the link/crate identifier is used by the CAEN
      drivers to identify the board. The VME address is not queried.

Once a link is defined, the software has to know what type of board is
hooked up. So far V1724 digitizers and V2718 crate controllers are
supported. Actually, only one V2718 module is required for the entire
DAQ, though more can be supported if needed.

The syntax for defining a module is ::

 {
   "boards": [
    {
      "crate": Int,
      "serial": String,
      "reader": Int,
      "type": Sting,
      "vme_address": String,
      "link": Int
    },
    ... #more boards
    ]
  }

The definitions of these are as follows.
 
     * "board" is a list field containing sub-dictionaries defining each board.
     * "crate" is the crate number assigned in the corresponding 'link' command.
     * "serial" is a unique identification number for the board.
       Each board has a serial number printed on the front and it is
       recommended to use that.
     * "reader" is the ID of the reader to which the board is connected
     * "type" gives the type of the board. At the moment only 
       the types 'V1724' or 'V2718' are supported.
     * "vme_address" is the board's VME address as a string (in hex). You can 
       set this using radial dials on the board itself. For example if you set
       a digitizer to "EEFF" then this option is "EEFF0000". 
     * "link" as before is the link over which the board is connected.
       
The following is an example initialization using one slave PCs with ID 2. 
It has one digitizers and one crate controller hooked up
via separate links on an A3818. ::
  
  {
     "boards": [
     {
      "crate": 0,
      "serial": "2374",
      "reader": 2,
      "type": "V2718",
      "vme_address": "DC000000",
      "link": 0
     },
     {
      "crate": 0,
      "serial": "1254",
      "reader": 2,
      "type": "V1724",
      "vme_address": "800D0000",
      "link": 1
     }
     ],
     "links":[
     {
      "type": "V2718",
      "reader": 2,
      "crate": 0,
      "link": 0
     },
     {
      "crate": 0,
      "type": "V2718",
      "reader": 2,
      "link": 1
     }
     ],
  }

For standalone deployments containing only one slave, the "reader" identifier
is not used. Additionally any "%n" lines must be removed. 
This is because for a standalone deployment the options file 
is not run through the parser that removes and strips this identifier.

Run Options
------------

The user can define several options related to the run. These are
options for kodiaq itself. To control the board internal options, see
the section on board options.

   * **blt_size {int}** 
     Size of a block transfer. The default is the maximum size of
     8 MB. There is probably no reason to change this unless you use a
     different model of digitizer.
   * **run_start {int} **
     Define how a run is started. 0 means via VME register and 1 means
     via s-in. Option 1 should always be used if you have multiple
     digitizers as it synchronizes the clocks of the digitizers.
   * **run_start_module {int} **     
     If run_start is set to '1', this option provides the ID of the crate
     controller that will be used to start the run. Ignored if run_start is 0.
   * **baseline_mode {int}**
     kodiaq contains an automated routine to adjust the
     baselines so that the full dynamic range of each input channel is
     used. This option lets the user set when that should happen.
     Giving the argument '0' means the baselines are not determined.
     They are set via the values given in the files in
     src/slave/baselines. The argument '1' tells the program to
     automatically determine baselines whenever the DAQ is armed.
   * **ddc10_options**
     If a DDC10 high energy veto module is used, this line lets you
     define the options. There is one string followed by fifteen
     integer arguments. A detailed explanation of the arguments
     appears in the documentation for the custom ddc10 firmware. The arguments are:
     * address: address of the module (ip)
     * sign: sign
     * window: integration window
     * delay: veto delay
     * signal_threshold: signal threshold
     * integration_threshold: integration threshold
     * width_cut: width cut
     * rise_time_cut: rise time cut
     * component_status: component status
     * parameter_n: where n = 0, 1, 2, or 3. 4 parameters for veto 
       function (see ddc10 docs)
     * outer_ring_factor: outer ring factor
     * inner_ring_factor: inner ring factor
     * prescaling: prescaling

An example of how these options appears in the .ini file is shown below. ::
  
  {     
     "blt_size": 524288,
     "run_start_module": 2374,
     "run_start": 1,
     "DDC-10": {
       "signal_threshold": 150,
       "address": "130.92.139.240",
       "prescaling": 1000,
       "delay": 200,
       "component_status": 1,
       "inner_ring_factor": 1,
       "rise_time_cut": 30,
       "parameter_0": 0,
       "window": 100,
       "parameter_1": 0,
       "outer_ring_factor": 2,
       "integration_threshold": 20000,
       "parameter_3": 50,
       "sign": 1,
       "width_cut": 50,
       "parameter_2": 0
     },
     "baseline_mode": 1
  }


Output Options
---------------

The output options all relate to where the data is pushed to. Not all
of these options are required, but be sure to include the options
related to your chosen write mode.

   * **write_mode {int}**
     Define what is done with the data. 0 - no writing (test mode). 1
     - write to file (not yet supported). 2 - write to mongodb. If you
     choose mode '2', make sure the MONGO_OPTIONS are defined.
   * **compression {int}** turns compression on (1) or off (0). Compression
     is done with Google's snappy algorithm.
   * **mongo_address {string}** This is the mongoDB connection string 
     for your database deployment. It must be a valid connection string
     readable by the C++ driver. Strings are checked for plausibility 
     before a connection is attempted (using the built-in isValid() call).
     Please note that multiple mongos instances are not supported as a  
     fallback case. The only instance where you can have multiple servers
     defined in the connection string is for a replica set.

     Here's are a couple examples, for a standalone server and a replica::

         mongodb://user:pass@server:port/DB
         mongodb://user:pass@server0:port0,server1:port1,server2:port2/DB?replicaSet=set_name

   * **mongo_database {string}** defines a database name for output. In case
     you use authentication in your connection string this must be identical 
     to your authentication database.
   * **mongo_collection {string}** defines a collection name. If you end
     the collection name with a wildcard (for example data*) the software
     will automatically assign a date/time string for the end of the collection
     name of form: data_YYMMDD_HHMM.
   * **mongo_min_insert_size {int}** gives the minimum number of BSON documents
     that must be collected before an insert is performed. kodiaq uses 
     bulk inserts to put data into mongodb. This means each insert is 
     actually a vector of  BSON documents.
   * **mongo_write_concern {int}** defines the write concern for mongo. 
     Putting this to  normal mode (set 1) turns write concern on. This 
     means the client will  wait for a reply from the mongo database after 
     writing. On the one hand, this is very good since it confirms an 
     insert made it to mongo. On the other hand it is very slow. Turning 
     the option of (value 0) is required for high-rate data-taking.
       
  * **processing_mode {int}** defines the block splitting mode. This is a
    reformatting of the data before it is put into the database. The
    options are:
    * 0 - No block splitting. Each mongodb document will contain an 
      entire block transfer which could contain one or many event headers.
    * 1 - coarse block splitting. Splits the block into separate events. 
      This is for the default board firmware where each event is a trigger. 
      One mongodb document contains data from all the channels for this trigger.
    * 2 - fine block splitting. Splits the block into separate events with one
      event per channel. 
    * 3 - Same as 2 but even further splits channels into occurrences. Only 
      works with the default firmware with zero-length-encoding enabled. 
      In this mode each zero-length-encoded chunk of data is its own doc. 
      This meant to emulate the custom firmware.
    * 4 - header extraction for custom firmware. Parses custom firmware events
      and puts each channel into its own document. The document timestamp is 
      taken from the channel in this case (note the header as in the other modes).

  * **processing_readout_threshold {int}** is the readout threshold. 
    This defines a minimum number of documents that must be read before 
    being processed. Can be tuned to achieve maximum write speeds in cases 
    where rates fluctuate. At the end of a run the entire buffer will be 
    written out regardless of whether this threshold was reached or not.

  * **processing_num_threads {int}** defines how many parallel threads 
    should be used for data parsing. It isn't suggested to make this 
    a ridiculous number. The boards can only be read one at a time (there 
    is a mutex-protected call to the CAEN block transfer function). The 
    goal at high rates is to have the boards always being read. Therefore 
    enough processing threads (and processing power) must exist to do all 
    of the data parsing, BSON creation, and data input. As a rule this 
    number is usually set based on the number of threads in the processor 
    on the computer. 
  * **occurrence_integral {int}** defines whether or not to compute an integral
    for each occurrence and store it in the documents. If this option is set to
    zero, no integral is computed. If the option is non-zero it defines the number
    of bins to use for the pre-sample baseline. This should usually be the
    same as the pre-trigger window set via VME option.
    NOTE: This number must be even and >=2. If the number is non-even it will be
    reduced by one. If the number is <2 it will be set to zero. If the number is larger
    than the sample size you will use the entire sample to determine the baseline
    and will get bad results.
* **lite_mode {int}** defines whether to store data or not. It is possible to run
    the DAQ in a sort of 'lite_mode' where the raw data for each occurrence is not stored. Rather
    one can store the occurrence_integral and the meta-data. If storage space is an issue
    this may be a reasonable option to do many types of analysis while greatly reducing the data size.

An example of how these options appear in the .ini file is shown
below. ::

  {
      "write_mode": 0,

      "mongo_collection": "data*",
      "mongo_write_concern": 0,
      "mongo_min_insert_size": 1,
      "mongo_database": "raw",
      "mongo_address": "xedaq00"

      "processing_readout_threshold": 0,
      "processing_num_threads": 8,

      "compression": 1
  }


VME Options
------------------

The VME options allow setting of the VME registers in the V1724
modules. This allows direct control of the acquisition system. Some of
these settings are absolutely required to remain at their default
values, others can be tuned based on the desired performance of the
DAQ. Please only change these values if you know what you are doing.
Problems with incorrect settings in these registers are very hard to
debug.

Two things should be kept in mind:

   1. The VME options are loaded to the boards in the order they are
      listed in the .ini file.
   2. Absolutely no sanity checking is done on these options. The
      value passed as an argument will be directly set to the
      digitizers.
      
The general format of a VME option setting is as follows: ::

  "registers": [
    {
      "board": String,
      "comment": String,
      "value": String,
      "register": String
    },
  ... #more options
  ]
    
Where the values of the parameters are:

    * **registers** is the lag that identifies a list of register settings.      
    * **register** is a hexidecimal value of the register to set. These
      are sixteen-bit (four word) values and are added to the board
      VME address to write to a specific memory register of a specific board.
    * **board** is an ID number of a board in case it is desired
      to write the option just to a single board. -1 means all boards.
    * **value** is the register value in hex.
    * **comment** is a user comment that can be helpful for keeping things straight.

Note that 'register' and 'value' are hex numbers but should be given as strings. 
The strings are parsed into unsigned integers in the code.

Here are a few examples of how to use the command. 

To write the value of 10 to register EF00 for all boards: ::

  {
      "board": "-1",
      "comment": "BERR register, 10=enable BERR",
      "value": "10",
      "register": "EF00"
   },

     
To write the same value to the same register only for board 800: ::

  {
      "board": "800",
      "comment": "BERR register, 10=enable BERR",
      "value": "10",
      "register": "EF00"
  },


Generally, the same options should be written to all digitizers so
setting the board, crate, and link IDs to -1 is advisable. The
exception to this is the acquisition monitor board, which may require
some special settings.

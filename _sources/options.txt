#########################
DAQ Options
#########################

The DAQ options are stored in .ini files, usually found in
src/master/data/RunModes and linked in the RunModes.ini file. The
design of the program is that these files will be defined by
developers while end users will have to option to pick from several
pre-defined modes. A suggested use of the end system is to have the
.ini files and the RunModes.ini file with permissions set to read only
for normal users.

It is important to not edit these files unless you know what you are
doing. There is only a limited amount of sanity checking done on the
input of these files and improper editing could break the DAQ. If you
are fortunate it will break in a way that is obvious, if you are
unlucky you might not even notice. Particularly the VME options are
passed to the boards *directly as printed* in the .ini file. Improper
setting of any of these options is very difficult to debug and there
is rarely any reason to change them.

That said, read on for an explanation of the options available.

General Syntax
----------------

The .ini file is mostly passed verbatim to the slaves when a run mode is
selected. However a limited amount of parsing is done.

Options lines follow the format ::
 
    OPTION_NAME ARG_1 ARG_2 ... ARG_N

where the OPTION_NAME must exactly match one of the pre-defined
options (case sensitive). The arguments can be strings or numeric and
are separated by spaces. If not enough arguments are provided, the
option is ignored. If too many arguments are provided, only the first
N arguments are read in, where N is the number of arguments expected.
The option string is ended with a line break.

In certain cases there are options that should only go to one slave
PC. An example of this is the electronics definitions. The readout
electronics are hard-wired to the slave PCs and the cabling must be
defined in the options file. In order to send a particular option to
just one slave, the line should be preceeded by '%n' (without quotes)
where n is the id of the slave as defined in its own network interface
constructor. The '%' symbol must be the first symbol in the line as
the master will parse the files searching specifically for this
character. Please don't start any lines with the '%' symbol and then
not follow them with an integer slave ID. Also, currently only up to
10 slaves are supported numbered 0-9. 

Also of note, commenting can be done with the '#' symbol. Any lines
preceeded by a '#' will be transmitted to the slaves but ignored by
the parser.

Electronics Definitions
------------------------

CAEN V1724 digitizers can be read out in one of two ways: either via a
single optical link to a V2718 crate controller, which reads the
boards out through the VME backend, or via the optical links on the
front of the V1724 digitizers themselves. When reading out through the
crate controller, rates are severely limited, presumably either
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

     LINK {type string} {optical port int} {crate int}

Keep in mind that the CAEN software was designed first for reading out
via the crate controller and extended to read out via the V1724
optical link directly and this will make a bit more sense.

    * LINK is the control word that tells kodiaq what the option is
    * {type string} is the type of optical link. We will always use
      'V2718' type links, even when reading out via the V1724.
    * {optical port int} is the ID of the optical port the cable is
      hooked up to. This is always 0 for the A2818 since it just has one
      port but can be 0-3 for the A3818.
    * {crate int} is the crate ID. In daisy chain mode each digitizer
      gets its own crate ID. Of course when reading out through a
      crate controller this made more sense, since there was one crate
      controller per crate. When reading out via the digitizer itself
      just think of this as an integral identifier.

Once a link is defined, the software has to know what type of board is
hooked up. So far V1724 digitizers and V2718 crate controllers are
supported. Actually, only one V2718 module is required for the entire
DAQ, though more can be supported if needed.

The syntax for defining a module is ::

     BASE_ADDRESS {type string} {VME Address} {Board ID} {Crate int} {board int}

The definitions of these are as follows.
 
     * BASE_ADDRESS tells the parser that it found a board definition.
     * {type string} gives the type of the board. At the moment only 
       the types 'V1724' or 'V2718' are supported.
     * {Board ID} is a unique identification number for the board.
       Each board has a serial number printed on the front and it is
       recommended to use that.
     * {Crate int} is the crate number assigned in the corresponding LINK command.
     * {board int} is zero for boards hooked up via daisy chain (since
       each is defined as its own crate). For readout through the
       crate controller each board on a crate is given a unique int
       starting at zero.
       
The following is an example initialization using two slave PCs. One
slave (number two) has two digitizers and a crate controller hooked up
via separate links on an A3818. The other slave (number one) has a
single digitizer only. ::

     #Slave ID 2
     %2LINK V2718 0 1 # -- Digitizer Link
     %2LINK V2718 0 0 # -- Digitizer Link
     %2LINK V2718 1 0 # -- Crate Controller Link
     %2BASE_ADDRESS  V1724 32100000 876 1 0 # -- Digitizer 
     %2BASE_ADDRESS  V1724 22230000 770 0 0 # -- Digitizer
     %2BASE_ADDRESS  V2718 EE000000 1868 0 1 # -- Crate Controller
    
     #Slave ID 1
     %1LINK V2718 0 0 # -- Digitizer Link
     %1BASE_ADDRESS V1724 22240000 749 0 0 # -- Digitizer

For standalone deployments containing only one slave, the '%n'
identifier must be removed. This is because for a standalone
deployment the options file is not run through the parser that removes
and strips this identifier.

Run Options
------------

The user can define several options related to the run. These are
options for kodiaq itself. To control the board internal options, see
the section on board options.

   * **BLT_SIZE {int}** 
     Size of a block transfer. The default is the maximum size of
     524288 bytes. There is probably no reason to change this.
   * **RUN_START {int} {int}**
     Define how a run is started. 0 means via VME register and 1 means
     via s-in. Option 1 should always be used if you have multiple
     digitizers as it synchronizes the clocks of the digitizers. The
     second option is ignored if the first argument is set to zero (but must
     still be provided). If the first argument is set to one the board
     ID of the crate controller that will be used to start the run
     must be provided as the second argument.
   * **BASELINE_MODE {int}**
     kodiaq contains an automated routine to adjust the
     baselines so that the full dynamic range of each input channel is
     used. This option lets the user set when that should happen.
     Giving the argument '0' means the baselines are not determined.
     They are set via the values given in the files in
     src/slave/baselines. The argument '1' tells the program to
     automatically determine baselines whenever the DAQ is armed. It
     is forseen to add an option to have the baselines recalibrated every
     hour or so without stopping the run, however this option does not
     exist yet.
   * **SUM_MODULE {int}**
     It is anticipated to run with one digitizer recording the
     attenuated sum of all channels, a clock, and some NIM signals.
     These channels should be flagged for special processing by the
     event builder. This is done with this option. Note if you want to
     flag more than one digitizer, just put a second SUM_MODULE option
     in (do not make one with two arguments as the second will be
     ignored).
   * **DDC10_OPTIONS**
     If a DDC10 high energy veto module is used, this line lets you
     define the options. An explanation of the options will follow
     once a more finalized version of the DDC10 class exists.
     
Output Options
---------------

The output options all relate to where the data is pushed to. Not all
of these options are required, but be sure to include the options
related to your chosen write mode.

   * **WRITE_MODE {int}**
     Define what is done with the data. 0 - no writing (test mode). 1
     - write to file (not yet supported). 2 - write to mongodb. If you
     choose mode '2', make sure the MONGO_OPTIONS are defined.
   * **MONGO_OPTIONS {string1} {string2} {int1} {int2} {int3} {int4}**
     Define the mongo linkage. This one is a bit more complicated and
     will be explained in detail.
     
     * {string1} gives the address of the PC running the mongodb database. 
       This can be either a hostname or an ip address.
     * {string2} defines a collection name for the data. The format of this 
       is {dbname}.{collectionname}. The database name must be provided. 
       If you provide a static collection name all data will be
       written to the same collection. Replacing the collection name with 
       a '*' character will put each run in its own collection,
       dynamically named based and the date and time the run is started.
       *Example:* 'data.pmtdata' puts the data in database 'data' and
       collection 'pmtdata' while 'data.*' puts the data in database
       'data' but creates a new collection every run with form
       data_YYMMDD_HHMM.
     * {int1} defines the compression mode. All data will be
       transferred to mongodb over the network so it is usually
       advantageous to compress it first. kodiaq uses snappy, which
       compresses the data by about 50% at a few hundres MB/s. Setting
       this value to 0 means no compression while 1 turns on
       compression. Whether or not the data is compressed is communicated to
       the event builder via the run attributes document (see the protocols
       section).
     * {int2} gives the max insert size. kodiaq uses bulk inserts to
       put data into mongodb. This means each insert is actually a vector of
       BSON documents. If the vector hits this maximum size, an insert is
       forced before more documents are generated.
     * {int3} defines the write concern for mongo. Putting this to
       normal mode (set 0) turns write concern on. This means the client will
       wait for a reply from the mongo database after writing. On the
       one hand, this is very good since it confirms an insert made it to
       mongo. On the other hand it is very slow. Turning the option of (value
       1) is required for high-rate data-taking.
     * {int4} defines a block splitting mode. This is a reformatting
       of the data before it is put into the database. The options are: 

       * 0 - No block splitting. Each mongodb document will contain an
         entire block transfer which could contain one or many event headers.
       * 1 - coarse block splitting. Splits the block into separate
         events. This is for the default board firmware where each
         event is a trigger. One mongodb document contains data from
         all the channels for this trigger.
       * 2 - fine block splitting. Splits the block into separate
         events. Then further splits the events into occurrences (only works if
         zero length encoding is set on via VME option). This means each
         zero-length-encoded chunk of data is its own doc. This mode
         only works with the default firmware and is meant to emulate the
         custom firmware.
       * 3 - header extraction for custom firmware. Not yet implemented.

  * **PROCESSING_THREADS {int}**
    Defines how many parallel threads should be used for data
    processing. It isn't suggested to make this a ridiculous number. The
    boards can only be read one at a time (there is a mutex-protected call
    to the CAEN block transfer function). The goal at high rates is to
    have the boards always being read. Therefore enough processing threads
    (and processing power) must exist to do all of the data parsing, BSON
    creation, and data input. As a rule this number is usually set based
    on the number of threads in the processor on the computer.
  * **READOUT_THRESHOLD {int}**
    Defines a minimum number of documents that must be ready before
    being inserted into mongodb. Can be tuned to achieve maximum write
    speeds in cases where rates fluctuate. At the end of a run the entire
    buffer will be written out regardless of whether this threshold was
    reached or not.

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
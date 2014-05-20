===========================
Monitoring DB 
===========================

A mongodb exists exclusively for the storage of monitoring and backend
operational data. The data will be described in this section. This
database is shared by the event builder, data processor, and file
builder (all separate packages) and the databases corresponding to
these elements can be found in their documentation.

==========
Run Modes
==========

The run mode database contains all DAQ run modes that have been
defined. Each run mode is a mongodb document. The following fields are
defined:

Registers
=========

* Type: array<int64>
* Function: Set register values of V1724
* Format: 64 bit numbers. Lower 2 bytes define the module (0 for all modules). The next 2 bytes define the register to be set. The upper 8 bytes give the register value. This looks like 0x VVVVVVVV RRRRMMMM (where V-value, R-register, M-module).

.. note:: See the CAEN manual for a description of register settings.

===========================
Data Formats 
===========================

Right now the only option for data output is writing to a mongodb
database. It is forseen to add later direct file output, at which
point this documentation will be updated. 

This section describes the data formats produced by the DAQ reader.

Output to Mongodb
------------------

Mongodb is an open-source enterprise level no-sql database package. It
automatically indexes and stores data sent to it locally or over a
network. Kodiaq uses mongodb both as a buffer to store data
temporarily while it is processed by an event builder, or as a more
long term storage for data taken in triggered mode. 

Mongodb takes data in the form of BSON document objects. A BSON object
always contains a unique ID number assigned and may also contain other
data fields such as integers, strings, and binary data. The general
procedure followed by the reader is to process the raw data from the
digitizers, strip the headers and put the information contained
therein into meta-data fields in the BSON documents, put the binary
data into a data field, and ship the document to mongodb.

Run Control Document
^^^^^^^^^^^^^^^^^^^^^

Data Documents
^^^^^^^^^^^^^^^

The following fields *must* be included in a data document. 
 
   * *_id* - Is the mongodb-assigned ID tag of the document. This must
     be unique for each doc. It is possible to extract the date and
     time of creation from this number.
   * *module* - The module ID. This is the serial number printed on
     the front of the digitizer.
   * *channel* - A value from 0-7 corresponding to the input channel.
   * *time* - Time stamp from the digitizer. Digitizers have 31-bit
     clocks with 10ns samples, so they wrap around about every 21 seconds.
     Kodiaq senses this wraparound and increments a counter which starts on
     the 32nd bit. The time stamp is then saved as a 64-bit integer.
   * *data* - This is the data payload. It is in binary format. The
     exact formatting of this payload depends on what mode the detector is
     run in. See the "data format" section below.
     
The following fields *may* be included in the data document and can
provide additional debugging information. The information is not
needed for event building.

    * *insertsize* - The size of the mongodb bulk insert. A bulk
      insert is when a vector of documents is pushed to the DB at the same
      time. This is simply the length of this vector. The minimum and
      maximum lengths are configurable via the options file.
    * *override* - Flags a document that it should not be included in
      event building. Currently however not recognized by cito. This flag is
      probably not needed.
    * *datalength* - The length of the data in bytes.
    * *zipped* - If the data is compressed or not (this information is
      also given in the run control document).

Data Format
^^^^^^^^^^^^^^^^^^^^^

The data format always follows closely the CAEN digitizer format. The
data can be thought of as an array of 32-bit words. Raw samples are 14
bits and are stored two per word (16-bits are allocated for each with
the highest two bits simply being ignored). 

Depending on the options chosen, some formatting may be done which strips 
headers out of the data and extracts this information into metadata. 

Full Events in Triggered Mode
"""""""""""""""""""""""""""""

In triggered mode, the digitizers are triggered via the TRIN connector
on the front of each board. This mode can be used, for example, for
LED calibration or for triggering via a discriminator module. In this
case the binary data provided in the mongodb docs is exactly in the
CAEN digitizer format. 

+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| 1| 0| 1| 0|                                Event Size                                         |
+--+--+--+--+--+-----+--+-----------------------------------------------+-----------------------+
|Board ID      | RES | 0|              Pattern                          |       Channel Mask    |
+--------------+-----+--+-----------------------------------------------+-----------------------+
|         Reserved      |                           Event Counter                               |
+-----------------------+-----------------------------------------------------------------------+
|                                         Trigger Time Tag                                      |
+-----------------------------------------------------------------------------------------------+
|                                       Size Ch. 0                                              |
+-----------------------------------------------------------------------------------------------+
|                                   Control Word                                                |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[1]  CH[0]                  | 0| 0|        Sample[0]  CH[0]                 |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| {MORE SAMPLES}                                                                                |
+-----------------------------------------------------------------------------------------------+
| Contol Word                                                                                   |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample [N-1]  CH[0]               | 0| 0|      Sample[N-2]  CH[0]                 |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| Size Ch. 1                                                                                    |
+-----------------------------------------------------------------------------------------------+
|                                   Control Word                                                |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[1]  CH[1]                  | 0| 0|        Sample[0]  CH[1]                 |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| {MORE SAMPLES}                                                                                |
+-----------------------------------------------------------------------------------------------+
| Control Word                                                                                  |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample [N-1]  CH[1]               | 0| 0|       Sample[N-2]  CH[1]                |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
|                                                                                               |
|                                 MORE CHANNELS                                                 |
+-----------------------------------------------------------------------------------------------+
| Size Ch. 7                                                                                    |
+-----------------------------------------------------------------------------------------------+
| Control Word                                                                                  |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[1]  CH[1]                  | 0| 0|        Sample[0]  CH[1]                 |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| {MORE SAMPLES}                                                                                |
+-----------------------------------------------------------------------------------------------+
| Control Word                                                                                  |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample [N-1]  CH[7]               | 0| 0| Sample[N-2]  CH[7]                      |
+--+--+-----------------------------------------+--+--+-----------------------------------------+

Keep in mind that this data format is *after* extraction in the case
that the "compressed" flag is true in the run control document.

Occurrences recorded in Self-Triggering Mode
"""""""""""""""""""""""""""""""""""""""""""""

In self-triggering mode (or when turning on occurrence parsing to
simulate it) each document contains just one channel worth of data. In
this mode all headers are stripped and the information is extracted
into the BSON metadata fields. The binary data contains only the
samples and has the following format. Again, the first line in the table is not
part of the data format and is only included to illustrate where each
bit is.

+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|31|30|29|28|27|26|25|24|23|22|21|20|19|18|17|16|15|14|13|12|11|10|09|08|07|06|05|04|03|02|01|00|
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
| 0| 0|       Sample[1]                         | 0| 0|        Sample[0]                        |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[3]                         | 0| 0|        Sample[2]                        |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[5]                         | 0| 0|        Sample[4]                        |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
|                                               MORE SAMPLES                                    |
+--+--+-----------------------------------------+--+--+-----------------------------------------+
| 0| 0|       Sample[N]                         | 0| 0|        Sample[N-1]                      |
+--+--+-----------------------------------------+--+--+-----------------------------------------+

Of course this data is once again after extraction in the case that
the compression flag is true in the run control document.


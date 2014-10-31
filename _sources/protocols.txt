===========================
Data Formats 
===========================

This section describes the data formats produced by the DAQ reader when writing to a database. 
This consists of the individual data documents as well as the run control document. 

Protocols for writing and reading from the online monitoring database are given in the next
chapter.

Mongodb
------------------

Mongodb is an open-source enterprise level no-sql database package. It
automatically indexes and stores data sent to it locally or over a
network. Kodiaq uses mongodb both as a buffer to store data
temporarily while it is processed by an event builder. 

Mongodb stores data as a collection of BSON document objects. A BSON object
always contains a unique ID number assigned and may also contain other
data fields such as integers, strings, and binary data. The general
procedure followed by the reader is to process the raw data from the
digitizers, strip the headers and put the information contained
therein into meta-data fields in the BSON documents, put the binary
data into a data field, and ship the document to mongodb.

Run Control Document
^^^^^^^^^^^^^^^^^^^^^

The run control document is stored in a dedicated runs database. Currently this 
is part of the online monitor database but since it is required for DAQ operation 
it is included here. An occurrence processing mode must be selected for this format to be used.

The collection for the runs database is online.runs. 

The document has the following fields (more can be added).

    * *id* - The mongodb ID tag. Can be used to pull specific docs from the database.
    * *runmode* - {string} The name of the operation mode used for this run
    * *runtype* - {string} An additional type describing how the run should be treated by the event builder
    * *starttime* - {int64} The CAEN digitizer time of the first event (usually zero)
    * *endtime* - {int64} Added when a run is stopped. The CAEN digitizer time of the end of the run.
    * *starttimestamp* - {datetime} A datetime object of the run start (unix time)
    * *endtimestamp* - {datetime} A datetime object of the run stop (unix time)
    * *user* - {string} User who started the run
    * *compressed* - {int} 0 for non-compressed data, 1 for data compressed with snappy
    * *name* - {string} Name of this run (contains the start date and time usually)
    * *daq_options* - {string} A string containing the options file used for this run (string output of a koOptions object
    * *comments* - List of comment objects. Comment objects contain fields 'date' (datetime), 'user' (string), and 'text' (string). 
    * *data_taking_ended* - {bool} Set to true when the DAQ stops the run normally
    * *trigger_ended* - {bool} Set to true once the event builder finishes with the run
    * *processing_ended* - {bool} Set to true once the online processor finishes the run. The online processor should also make additional entries into the run object saying where the processed file was stored and what options were used for processing.
    * *saved_to_file* - {bool} Set to true once the data is stored to file. The file builder should also update the document with the path of the saved file.

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


Output of Full Events
"""""""""""""""""""""

If the processing mode is set to '0' or '1' (or no processing) the data is dumped 
to mongodb documents exactly as it comes out of the digitizers. With '0' each doc can 
contain many events. Mode '1' means each doc contains exactly 1 trigger. 

In this case the data format is the CAEN format. This appears in the manual for the 
V1724 (available www.caen.it) and is reproduced below. Note that this format is valid 
for default V1724 firmware and may be different if a custom firmware is used.

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

If compression is turned on the data must first be extracted before it has the format shown above.


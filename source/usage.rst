========================
Using the Standalone Reader
========================

The standalone reader exists in src/slave. An options file in this directory 
called DAQConfig.ini should be defined to set your run options. This is explained 
in detail in the options section of this documentation.

We'll assume DAQConfig.ini is configured correctly. Make sure to update the MongoDB
collection before starting a new run though!

Then you should be able to run the code like this::
  
  cd src/slave
  ./koSlave

Starting and stopping the run is done using the keyboard prompts: (s) to start, (p) 
to stop, (q) to quit.

Data is written to your MongoDB collection in the following format ::

  {
     "time": {samples since run start},
     "module": {module number},
     "channel": {channel number},
     "data": {data payload, 16 bit samples}
  }

To extract the data, the following numpy function should save you a trip to Google ::

   payload = doc['data']
   data = np.fromstring(payload, dtype=np.int16)



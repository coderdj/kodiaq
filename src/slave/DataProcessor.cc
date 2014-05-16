// *************************************************************
// 
// kodiaq Data Acquisition Software
//  
// File      : DataProcessor.cc
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 19.09.2013
// Updated   : 26.03.2013
//  
// Brief     : Data formatting and processing. Sits between 
//             digitizer output buffer and file/db write buffer
// 
// *************************************************************

#include "DataProcessor.hh"
#include "DigiInterface.hh"
#include <snappy.h>

DataProcessor::DataProcessor()
{
   //Default values
   //
   //This class does not create these pointers and will not clear them
   m_DigiInterface = NULL;
   m_DAQRecorder   = NULL;
   m_koOptions     = NULL;
   m_bErrorSet     = false;   
   itype=REC_NONE;
}

DataProcessor::~DataProcessor()
{
}

void* DataProcessor::WProcess(void* data)
{
   
      //This function is a bit weird, but blame pthreads for their odd syntax
      //To use this in a pthread it has to be a virtual static void* that takes
      //an argument of type void*. In either a stroke of brilliance or a sketchy
      //move, we pass a DataProcessor_mongodb to the function as the void* data
      //(calling function has to pay attention to this!) and use this object to 
      //do the processing within the thread.
   DataProcessor *DP = static_cast<DataProcessor_dump*>(data);
   bool processed=false;
#ifdef HAVE_LIBMONGOCLIENT   
   if(DP->itype==REC_MONGO)  { 
      DataProcessor_mongodb *dpm = dynamic_cast<DataProcessor_mongodb*>(DP);
      dpm->Process();
      processed=true;
   }
#endif
#ifdef HAVE_LIBPBF
   if(DP->itype==REC_FILE)  {
      DataProcessor_protobuff *dpf = dynamic_cast<DataProcessor_protobuff*>(DP);
      dpf->Process();
      processed=true;
   }
#endif
   if(!processed)
     DP->Process();
   return (void*)data;
}

DataProcessor::DataProcessor(DigiInterface *digi, DAQRecorder *recorder,
			     koOptions *options)
{
   m_DigiInterface   = digi; 
   m_DAQRecorder     = recorder;
   m_koOptions       = options;
   m_bErrorSet       = false;
}

void DataProcessor::LogError(string err)
{
   m_bErrorSet  = true;
   m_sErrorText = err;
   return;
}

bool DataProcessor::QueryError(string &err)
{
   if(!m_bErrorSet) return false;
   err = m_sErrorText;
   m_sErrorText = "";
   m_bErrorSet  = false;
   return true;
}

u_int32_t DataProcessor::GetTimeStamp(u_int32_t *buffer)
//Pull a time stamp out of a CAEN header
{
   int pnt = 0;
   while((buffer[pnt])==0xFFFFFFFF) //filler between events
     pnt++;
   if((buffer[pnt]>>20)==0xA00)   { //look for a header
      pnt+=3;
      return (buffer[pnt] & 0x7FFFFFFF);
   }   
   return 0;   
}

void DataProcessor::SplitData(vector<u_int32_t*> *&buffvec, 
				  vector<u_int32_t>  *&sizevec)
// Break BLTs into individual triggers by locating headers and parsing
// The buffvec and sizevec returned by reference must be cleared by the
// calling function when they are no longer needed!
{
   
   //Will delete the buffers passed to the function and return these ones
   vector <u_int32_t*> *retbuff = new vector <u_int32_t*>();
   vector <u_int32_t > *retsize = new vector <u_int32_t >();
   
   for(unsigned int x=0; x<buffvec->size();x++)  {	
      if((*sizevec)[x]==0)   {	  //sanity check
	 delete[] (*buffvec)[x];
	 continue;
      }
      
      unsigned int idx = 0;
      while(idx<((*sizevec)[x]/sizeof(u_int32_t)) && 
	    (*buffvec)[x][idx]!=0xFFFFFFFF)   {
	 if(((*buffvec)[x][idx]>>20) == 0xA00) { //found a header	    
	    u_int32_t size = (*buffvec)[x][idx]&0xFFFF*4;
	    u_int32_t *rb = new u_int32_t[size]; //return buffer created
	    //copy trigger to new buffer
	    copy((*buffvec)[x]+idx,(*buffvec)[x]+(idx+size),rb); 
	    retbuff->push_back(rb);
	    retsize->push_back(size*sizeof(u_int32_t));
	    idx+=size;
	 }
	 else
	   idx++;
      }//end while
      delete[] (*buffvec)[x];
   }
   delete buffvec;
   delete sizevec; 
   buffvec=retbuff;
   sizevec=retsize;
   return;
}

void DataProcessor::SplitDataChannels(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec,
				      vector<u_int32_t> *timeStamps, vector<u_int32_t> *channels,
				      vector<u_int32_t> *eventIndices)
{
   
   vector <u_int32_t*> *retbuff = new vector <u_int32_t*>();
   vector <u_int32_t>  *retsize = new vector <u_int32_t>();
   
   //timestamps, channels should be provided as pointers to empty vectors
   //Only works with ZLE on! Calling function should be aware of this.
   
   //buffvec is the buffers to be parsed (one blt per element)
   //sizevec is the size of each buffer in buffvec (in bytes)
   //retbuff will be the returned buffers with headers stripped
   //retsize will be the returned sizes in words
   //timeStamps will be the timestamps of the returned buffers
   //channels will be the channels of the returned buffers
   //eventIndices will be a list of indices where new BLTs start
   
   for(unsigned int x=0; x<buffvec->size();x++)  {	
      //loop through main vector containing the raw data
      if((*sizevec)[x]==0)      { //sanity check
	 delete[] (*buffvec)[x];
	 continue;   
      }
      
      unsigned int idx=0;           //used to iterate through the buffer
      u_int32_t headerTime=0;
      
      while(idx<(((*sizevec)[x])/(sizeof(u_int32_t)))) {	   
	 if(((*buffvec)[x][idx])==0xFFFFFFFF){idx++; continue;}//empty data, iterate
	 if(((*buffvec)[x][idx]>>20)!=0xA00){idx++; continue;} //found a header
	 
	 if(eventIndices!=NULL)
	   eventIndices->push_back(retbuff->size());
	 //Read information from header. Need channel mask and header time
	 u_int32_t mask = ((*buffvec)[x][idx+1])&0xFF;
	 headerTime = ((*buffvec)[x][idx+3])&0x7FFFFFFF;
	 idx+=4;    //skip header, we have what we need
	 
	 for(int channel=0; channel<8;channel++){   //loop through channels
	    if(!((mask>>channel)&1))      //Do we have this channel in the event?
	      continue;
	    u_int32_t channelSize = ((*buffvec)[x][idx]);
	    idx++; //iterate past the size word
	    
	    int sampleCnt=0;        //this counts the samples for timing purposes
	    unsigned int wordCnt=1; //this counts the words so we know when we get to the end of size
	    while(wordCnt<channelSize) { //until we get to the end of this channel data                
	       if(((*buffvec)[x][idx]>>28)!=0x8) { //data is no good
		  sampleCnt+=((2*(*buffvec)[x][idx]));
		  idx++;
		  wordCnt++;
		  continue;
	       }
	       int GoodWords = (*buffvec)[x][idx]&0xFFFFFFF;
	       idx++;                   //iterate past the control word
	       wordCnt++;
	       //char *keep = new char[GoodWords*4];
	       u_int32_t *keep = new u_int32_t[GoodWords];
	       //	       memcpy(keep,((*buffvec)[x]+idx),4*GoodWords);
	       copy((*buffvec)[x]+idx,(*buffvec)[x]+(idx+GoodWords),keep);
	       
	       idx+=GoodWords;
	       wordCnt+=GoodWords;
	       retbuff->push_back(keep);
	       retsize->push_back(GoodWords*4);
	       channels->push_back(channel);
	       timeStamps->push_back(headerTime+sampleCnt);
	       sampleCnt+=2*GoodWords;
	    }//end while through this channel data          
	 }//end for through channels                             
      }//end while through this trigger
      delete[] (*buffvec)[x];
   }
 
   delete buffvec;
   delete sizevec;
   buffvec=retbuff;
   sizevec=retsize;
   return;
}

void DataProcessor::SplitDataChannelsNewFW(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec, 
					   vector<u_int32_t> *timeStamps, vector<u_int32_t> *channels)
{
   vector <u_int32_t*> *retbuff = new vector <u_int32_t*>();
   vector <u_int32_t>  *retsize = new vector <u_int32_t>();
   
   //timestamps, channels should be provided as pointers to empty vectors
      
   //buffvec is the buffers to be parsed (one blt per element)
   //sizevec is the size of each buffer in buffvec (in bytes)
   //retbuff will be the returned buffers with headers stripped
   //retsize will be the returned sizes in words
   //timeStamps will be the timestamps of the returned buffers
   //channels will be the channels of the returned buffers

    for(unsigned int x=0; x<buffvec->size();x++)  {	
       //loop through main vector containing the raw data
       if((*sizevec)[x]==0)      {
	  //sanity check
	  delete[] (*buffvec)[x];
	  continue;
       }	
       
       unsigned int idx=0;           //used to iterate through the buffer
       //u_int32_t headerTime=0;
       
       while(idx<(((*sizevec)[x])/(sizeof(u_int32_t)))) {	    
	  if(((*buffvec)[x][idx])==0xFFFFFFFF){
	     idx++; continue;
	  }
	  //empty data, iterate
	  if(((*buffvec)[x][idx]>>20)!=0xA00)   {
	     idx++; continue;
	  }
	  //found a header
	    
	  //Read information from header. Need channel mask and header time
	  u_int32_t mask = ((*buffvec)[x][idx+1])&0xFF;
	  //headerTime = ((*buffvec)[x][idx+3])&0x7FFFFFFF;
	  idx+=4;    //skip header, we have what we need
	  
	  for(int channel=0; channel<8;channel++){
	     //loop through channels
	     if(!((mask>>channel)&1))      //Do we have this channel in the event?
	       continue;	     
	     u_int32_t channelSize = ((*buffvec)[x][idx]);
	     idx++; //iterate past the size word
	     u_int32_t channelTime = ((*buffvec)[x][idx])&0x3FFFFFFF;
	     idx++;
	     //char *keep = new char[(channelSize-2)*4];	     
	     u_int32_t *keep = new u_int32_t[channelSize-2];
	     copy((*buffvec)[x]+idx,(*buffvec)[x]+(idx+channelSize-2),keep); //copy channel data
	     idx+=channelSize-2;
	     retbuff->push_back(keep);
	     retsize->push_back((channelSize-2)*4);
	     channels->push_back(channel);
	     timeStamps->push_back(channelTime);
	  }
       }//end while       
       delete(*buffvec)[x];
    }//end for through buffvec
   delete buffvec;
   delete sizevec;
   buffvec=retbuff;
   sizevec=retsize;
   return;   
}

#ifdef HAVE_LIBMONGOCLIENT
//
//
// DataProcessor_mongodb
// 
// 
DataProcessor_mongodb::DataProcessor_mongodb() : DataProcessor()
{
   itype=REC_MONGO;
}

DataProcessor_mongodb::~DataProcessor_mongodb()
{
}

DataProcessor_mongodb::DataProcessor_mongodb(DigiInterface *digi, 
					     DAQRecorder *recorder, 
					     koOptions *options)
                      :DataProcessor(digi,recorder,options)
{
   itype=REC_MONGO;
}

/*void* DataProcessor_mongodb::WProcess(void* data)
{
   //This function is a bit weird, but blame pthreads for their odd syntax
   //To use this in a pthread it has to be a virtual static void* that takes
   //an argument of type void*. In either a stroke of brilliance or a sketchy
   //move, we pass a DataProcessor_mongodb to the function as the void* data
   //(calling function has to pay attention to this!) and use this object to 
   //do the processing within the thread.
   DataProcessor_mongodb *DP = static_cast<DataProcessor_mongodb*>(data);
   DP->ProcessMongoDB();
   return (void*)data;
}*/
//
void DataProcessor_mongodb::Process()
{
   // Creates BSON objects out of a data buffer according to the .ini options
   // Designed to run within a pthread. Will run until ExitCondition is reached
   
   bool      bExitCondition     = false;
   u_int32_t uiInsertSize       = 0;

   if(DataProcessor::GetDAQRecorder()==NULL || 
      DataProcessor::GetDigiInterface()==NULL)
     return;
   
   DAQRecorder_mongodb *mongo = dynamic_cast<DAQRecorder_mongodb*>
     (DataProcessor::GetDAQRecorder());
   DigiInterface *fDigiInterface = DataProcessor::GetDigiInterface();
   
   int       imongoID            = mongo->RegisterProcessor();
   if(imongoID==-1){
      LogError("DataProcessor_mongodb::ProcessMongoDB() - Failed to register with recorder.");
      return;
   }
   
   // Declare containers for data
   vector<u_int32_t*> *buffvec  =NULL;
   vector<u_int32_t > *sizevec  =NULL;
   vector<u_int32_t > *channels =NULL;
   vector<u_int32_t > *times    =NULL;
   
   bool b30BitTimes=false; //difference if using old or new fw timing
   
   vector <mongo::BSONObj> *vInsertVec = new vector<mongo::BSONObj>();
   
   while(!bExitCondition) {
      bExitCondition = true;
      for(unsigned int x=0; x<fDigiInterface->GetDigis();x++)  {	   
	 CBV1724 *digi = fDigiInterface->GetDigi(x);
	    if(digi->Activated())         bExitCondition=false;
	 
	    //Data buffer of "filled" digi is only locked for long enough to 
	    //get the data out. Then it can go back to taking data.
	    if(digi->RequestDataLock()!=0) continue;
	    buffvec = digi->ReadoutBuffer(sizevec);
	    digi->UnlockDataBuffer();
	    // **********
	    // PROCESSING
	    // **********
	   
	    b30BitTimes=false;
	    if(m_koOptions->GetProcessingOptions().Mode==1)    //Trigger Splitting
	      SplitData(buffvec,sizevec);
	    else if(m_koOptions->GetProcessingOptions().Mode==2){ //Occurence splitting
	       channels = new vector<u_int32_t>();
	       times    = new vector<u_int32_t>();
	       SplitDataChannels(buffvec,sizevec,times,channels);
	    }
	    else if(m_koOptions->GetProcessingOptions().Mode==3)  { //NewFW Channel Splitting
	       channels = new vector<u_int32_t>();
	       times    = new vector<u_int32_t>();
	       SplitDataChannelsNewFW(buffvec,sizevec,times,channels);
	       b30BitTimes=true;
	    }
	    
	    
	    // **************
	    // END PROCESSING
	    // **************
	    
	    // *********
	    // INSERTION
	    // *********
	    
	    int ModuleNumber = digi->GetID().BoardID;
	    int nBuff        = buffvec->size();
	    
	    for(int b=0; b<nBuff;b++) {
	       mongo::BSONObjBuilder bson;
	       u_int32_t *buff = (*buffvec)[b];
	       u_int32_t eventSize = (*sizevec)[b];
	       u_int32_t TimeStamp = 0;
	       
	       if(m_koOptions->GetProcessingOptions().Mode!=2
		 && m_koOptions->GetProcessingOptions().Mode!=3){
		  TimeStamp = GetTimeStamp(buff); //time stamp out of header
		  bson.append("channel",-1); //all channels
	       }	       
	       else	 {
		  TimeStamp = (*times)[b]; //time stamp from parsed vector
		  bson.append("channel",(*channels)[b]);
	       }
	    
	       bson.append("module",ModuleNumber);
	       
	       int ResetCounter = mongo->GetResetCounter(TimeStamp,b30BitTimes);
	       long long Time64=0;
	       if(!b30BitTimes) Time64 = ((unsigned long)ResetCounter << 31) | TimeStamp;
	       else Time64 = ((unsigned long)ResetCounter << 30) | TimeStamp;
	       bson.append("time",Time64);
	       
	       //should we zip the output?
	       if(m_koOptions->GetMongoOptions().ZipOutput)  { //compress with snappy
		  if(eventSize==0)   {
		     delete[] (*buffvec)[b];
		     continue;
		  }
		  char* compressed = new char[snappy::MaxCompressedLength(eventSize)];
		  size_t compressedLength=0;
		  snappy::RawCompress((const char*)buff, eventSize,
				      compressed,&compressedLength);
		  uiInsertSize+=compressedLength;
		  bson.appendBinData("data",(int) compressedLength, mongo::BinDataGeneral,
				     (const void*)compressed);
		  delete[] ((*buffvec)[b]);
		  delete[] compressed;
	       }
	       else { //don't compress the data
		  uiInsertSize+=eventSize;
		  bson.appendBinData("data",(int)eventSize,mongo::BinDataGeneral,
				     (const void*)buff);
		  delete[] (*buffvec)[b];
	       }
	       //
	       //using bulk inserts, so put into insert vec
	       vInsertVec->push_back(bson.obj());
	       
	       //if we are above the required insert size, put it in
	       if((int)uiInsertSize> m_koOptions->GetMongoOptions().MinInsertSize) {
		  if(mongo->InsertThreaded(vInsertVec,imongoID)==0)  {
		     //insert successful and ownership of vInsertVec passed
		     uiInsertSize=0;
		     vInsertVec = new vector<mongo::BSONObj>();
		  }
		  else  {
		     LogError("MongoDB insert error from processor thread.");
		     bExitCondition=true;
		     delete vInsertVec;
		     vInsertVec=NULL;
		     break;
		  }		  
	       }	       		  	    	       
	    }//end for through nBuff
	 if(channels!=NULL) delete channels;
	 if(times!=NULL) delete times;
	 if(buffvec!=NULL) delete buffvec;
	 if(sizevec!=NULL) delete sizevec;	    
	 buffvec=NULL;
	 sizevec=channels=times=NULL;
	 if(vInsertVec==NULL) break;	    
      }//end loop through digis
      if(vInsertVec==NULL) break; //get out if mongo is failing      
   }//end while(!ExitCondition)
   if(vInsertVec!=NULL) delete vInsertVec; //should this ever happen?
   return;
}

#endif

#ifdef HAVE_LIBPBF
//
// DataProcessor_protobuff
// 
DataProcessor_protobuff::DataProcessor_protobuff() : DataProcessor()
{
   itype=REC_FILE;
}

DataProcessor_protobuff::~DataProcessor_protobuff()
{  
}

DataProcessor_protobuff::DataProcessor_protobuff(DigiInterface *digi, 
						 DAQRecorder *recorder,
						 koOptions *options)
                        :DataProcessor(digi,recorder,options)
{
   itype=REC_FILE;
}

/*
void* DataProcessor_protobuff::WProcess(void* data)
{
   //pthread compatible function, same as for koDataProcessor_mongodb::WProcess
   DataProcessor_protobuff *DP = static_cast<DataProcessor_protobuff*>(data);
   DP->ProcessProtoBuff();
   return (void*)data;
}*/

void DataProcessor_protobuff::Process()
{		 
   bool      bExitCondition     = false;
   int       iModule            = -1;
   bool      b30BitTimes        = false;
   
   //Get the digi interface object. Will pull data from here.
     if(DataProcessor::GetDigiInterface()==NULL ||
	DataProcessor::GetDAQRecorder()==NULL)
             return;
   DigiInterface *fDigiInterface = DataProcessor::GetDigiInterface();
   DAQRecorder_protobuff *protorecorder = dynamic_cast<DAQRecorder_protobuff*>
     (DataProcessor::GetDAQRecorder());
   
   // Declare containers for data
   vector<u_int32_t*> *buffvec      = NULL;
   vector<u_int32_t > *sizevec      = NULL;
   vector<u_int32_t > *channels     = NULL;
   vector<u_int32_t > *times        = NULL;
   vector<u_int32_t > *eventIndices = NULL;
   
   //This vector will hold the objects we will write
   
   while(!bExitCondition) {
      bExitCondition = true;
      for(unsigned int x=0; x<fDigiInterface->GetDigis();x++)  {
	 CBV1724 *digi = fDigiInterface->GetDigi(x);
	    //keep exit condition false as long as at least one board is active
	    if(digi->Activated())         bExitCondition=false;
      
	    //Data buffer of "filled" digi is only locked for long enough to 
	    //get the data out. Then it can go back to taking data.
	    if(digi->RequestDataLock()!=0) continue;
	    buffvec = digi->ReadoutBuffer(sizevec);
	    iModule = digi->GetID().BoardID;
	    digi->UnlockDataBuffer();
	    
            // **********
	    // PROCESSING
	    // **********
      
	    b30BitTimes=false;
	    channels = new vector<u_int32_t>();
	    times = new vector<u_int32_t>();
	    if(m_koOptions->GetProcessingOptions().Mode!=3) {//old firmware
	       eventIndices = new vector<u_int32_t>();	       
	       SplitDataChannels(buffvec,sizevec,times,channels,eventIndices);
	    }	    
	    else{ //new firmware
	       SplitDataChannelsNewFW(buffvec,sizevec,times,channels);
	       b30BitTimes=true;
	    }	    
	    
	    // **************
	    // END PROCESSING
	    // **************
   
	    //
	    // Loop though vector of parsed buffers
	    unsigned int currentEventIndex=0;	    
	    int handle=-1;
	    for(unsigned int b = 0; b < buffvec->size(); b++) {
		 	       	       	       
	       //time
	       u_int32_t TimeStamp = 0;
	       TimeStamp = (*times)[b];
	       int ResetCounter = protorecorder->GetResetCounter(TimeStamp,b30BitTimes);
	       u_int64_t Time64 = 0;
	       if(!b30BitTimes) Time64 = ((u_int64_t)ResetCounter << 31) | TimeStamp;
	       else Time64 = ((u_int64_t)ResetCounter <<30 ) | TimeStamp;

	       //check if we should open a new event or not
	       if(eventIndices==NULL || (currentEventIndex<eventIndices->size()
					 && (*eventIndices)[currentEventIndex]==b)){
		  if(handle!=-1)		    		       
		    protorecorder->GetOutfile()->close_event(handle,true);
		  protorecorder->GetOutfile()->create_event(TimeStamp,handle);
		  if(eventIndices!=NULL) currentEventIndex++;
	       }
	       
//	       if(m_koOptions->GetProcessingOptions().Mode!=3)
//		 protorecorder->GetOutfile()->add_data(handle,(int)(*channels)[b],iModule,
//						       (char*)(*buffvec)[b],(size_t)((*sizevec)[b]),
//						      (*times)[b]);
//	       else
		 protorecorder->GetOutfile()->add_data(handle,(int)(*channels)[b],iModule,
						       (char*)(*buffvec)[b],(size_t)((*sizevec)[b]),
						       Time64);
	       //close event if at end of buffer vec
	       if(b==buffvec->size()-1)    
		  protorecorder->GetOutfile()->close_event(handle,true);

		  
	       
	       //delete data
	       delete[] ((*buffvec)[b]);
	    }//end loop through buffers
	    	    
	    if(channels!=NULL) delete channels;
	    if(times!=NULL) delete times;
	    if(buffvec!=NULL) delete buffvec;
	    if(sizevec!=NULL) delete sizevec;
	    if(eventIndices!=NULL) delete eventIndices;
	    buffvec=NULL;
	    eventIndices=sizevec=channels=times=NULL;
      }      
   }      
   
   return;
}
#endif

//
//
// DataProcessor_dump - debug processor
// 
DataProcessor_dump::DataProcessor_dump() : DataProcessor()
{
}


DataProcessor_dump::~DataProcessor_dump()
{
   
}


DataProcessor_dump::DataProcessor_dump(DigiInterface *digi,
				       DAQRecorder *recorder,
				       koOptions *options)
                    :DataProcessor(digi,recorder,options)
{
}

/*
void* DataProcessor_dump::WProcess(void* data)
{
   DataProcessor_dump *DP = static_cast<DataProcessor_dump*>(data);
   DP->Process();
   return (void*)data;
}*/

void DataProcessor_dump::Process()
{
   
   bool      bExitCondition     = false;
      
   // Get the digi interface object. Will pull data from here.
   if(DataProcessor::GetDigiInterface()==NULL)
     return;
   DigiInterface *fDigiInterface = DataProcessor::GetDigiInterface();
   
   // Declare containers for data
   vector<u_int32_t*> *buffvec  =NULL;
   vector<u_int32_t > *sizevec  =NULL;
   vector<u_int32_t > *channels =NULL;
   vector<u_int32_t > *times    =NULL;
   
   while(!bExitCondition) {
      bExitCondition = true;
      for(unsigned int x=0; x<fDigiInterface->GetDigis();x++)  {
	 CBV1724 *digi = fDigiInterface->GetDigi(x);
	    //keep exit condition false as long as at least one board is active
            if(digi->Activated())         bExitCondition=false;

            //Data buffer of "filled" digi is only locked for long enough to 
            //get the data out. Then it can go back to taking data.
            if(digi->RequestDataLock()!=0) continue;
            buffvec = digi->ReadoutBuffer(sizevec);
            digi->UnlockDataBuffer();

	    // **********
            // PROCESSING
            // **********
                     
            if(m_koOptions->GetProcessingOptions().Mode==1)    //Trigger Splitting
              SplitData(buffvec,sizevec);
            else if(m_koOptions->GetProcessingOptions().Mode==2){ //Occurence splitt$
               channels = new vector<u_int32_t>();
               times    = new vector<u_int32_t>();
               SplitDataChannels(buffvec,sizevec,times,channels);
            }
                  
            // **************
            // END PROCESSING
            // **************
	    
	    //delete data
	    for(unsigned int b=0; b<buffvec->size();b++)  
	       delete[] ((*buffvec)[b]);
	    if(channels!=NULL) delete channels;
	    if(times!=NULL) delete times;
	    if(buffvec!=NULL) delete buffvec;
	    if(sizevec!=NULL) delete sizevec;
	    buffvec=NULL;
	    sizevec=channels=times=NULL;	    	    	    	 
      }
   }
   return;
}


	


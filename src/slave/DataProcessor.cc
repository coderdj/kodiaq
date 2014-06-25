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
}

DataProcessor::~DataProcessor()
{
}

void* DataProcessor::WProcess(void* data)
{
  DataProcessor *DP = static_cast<DataProcessor*>(data);
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

void DataProcessor::SplitBlocks(vector<u_int32_t*> *&buffvec, 
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

void DataProcessor::SplitChannels(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec,
				      vector<u_int32_t> *timeStamps, vector<u_int32_t> *channels,
				  vector<u_int32_t> *eventIndices, bool ZLE)
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
    u_int32_t channelSize=0;
    while(idx<(((*sizevec)[x])/(sizeof(u_int32_t)))) {	   
      if(((*buffvec)[x][idx])==0xFFFFFFFF){idx++; continue;}//empty data, iterate
      if(((*buffvec)[x][idx]>>20)!=0xA00){idx++; continue;} //found a header
      if(!ZLE) channelSize = (((*buffvec)[x][idx]&0xFFFFFF)-4)/8;
      if(eventIndices!=NULL)
	eventIndices->push_back(retbuff->size());
      //Read information from header. Need channel mask and header time
      u_int32_t mask = ((*buffvec)[x][idx+1])&0xFF;
      headerTime = ((*buffvec)[x][idx+3])&0x7FFFFFFF;
      idx+=4;    //skip header, we have what we need
      
      for(int channel=0; channel<8;channel++){   //loop through channels
	if(!((mask>>channel)&1))      //Do we have this channel in the event?
	  continue;
	if(ZLE){
	  channelSize = ((*buffvec)[x][idx]);
	  idx++; //iterate past the size word
	}
	
	int sampleCnt=0;        //this counts the samples for timing purposes
	unsigned int wordCnt=0; //this counts the words so we know when we get to the end of size
	if(ZLE) wordCnt++;

	while(wordCnt<channelSize) { //until we get to the end of this channel data                
	  if(ZLE && ((*buffvec)[x][idx]>>28)!=0x8) { //data is no good
	    sampleCnt+=((2*(*buffvec)[x][idx]));
	    idx++;
	    wordCnt++;
	    continue;
	  }
	  int GoodWords=0;
	  if(ZLE){
	    GoodWords = (*buffvec)[x][idx]&0xFFFFFFF;
	    idx++;                   //iterate past the control word
	    wordCnt++;
	  }
	  else  GoodWords=channelSize;
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

void DataProcessor::SplitChannelsNewFW(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec, 
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

void DataProcessor::Process()
{
  // General processing class. Parses data then passes on to the appropriate recorder object
  bool bExitCondition = false; //set to true if no boards are active
  
  if(m_DigiInterface == NULL || m_koOptions == NULL) 
    return;
  if(m_DAQRecorder==NULL && m_koOptions->GetRunOptions().WriteMode!=WRITEMODE_NONE)
    return;

#ifdef HAVE_LIBMONGOCLIENT
  int mongoID = -1;
  DAQRecorder_mongodb *DAQRecorder_mdb = NULL;
  vector <mongo::BSONObj> *vMongoInsertVec = new vector<mongo::BSONObj>();
  if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_MONGODB){
    DAQRecorder_mdb = dynamic_cast<DAQRecorder_mongodb*>(m_DAQRecorder);
    mongoID = m_DAQRecorder->RegisterProcessor();        
  }
#endif
#ifdef HAVE_LIBPBF
  DAQRecorder_protobuff *DAQRecorder_pb = NULL;
  if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_FILE){
    DAQRecorder_pb = dynamic_cast<DAQRecorder_protobuff*>(m_DAQRecorder);
  }
#endif
 
  //declare data containers
  vector<u_int32_t*> *buffvec      = NULL;
  vector<u_int32_t > *sizevec      = NULL;
  vector<u_int32_t > *channels     = NULL;
  vector<u_int32_t > *times        = NULL;
  vector<u_int32_t > *eventIndices = NULL;
  int                 iModule      = 0;

  while(!bExitCondition){
    bExitCondition = true;
    for(unsigned int x=0; x<m_DigiInterface->GetDigis();x++)  {
      CBV1724 *digi = m_DigiInterface->GetDigi(x);
      if(digi->Activated()) bExitCondition=false;
      
      usleep(10);
      
      // Get the data if there is some
      if(digi->RequestDataLock()!=0) continue;
      buffvec = digi->ReadoutBuffer(sizevec);
      iModule = digi->GetID().BoardID;
      digi->UnlockDataBuffer();
      
      iModule = digi->GetID().BoardID;

      // Parse the data if requested
      if(m_koOptions->GetProcessingOptions().Mode == 1) { //simple block parsing
	SplitBlocks(buffvec,sizevec);
      }
      else if(m_koOptions->GetProcessingOptions().Mode !=0 ){
	channels = new vector<u_int32_t>();
	times = new vector<u_int32_t>();

	if(m_koOptions->GetProcessingOptions().Mode == 2 ) { //channel parsing old fw
	  eventIndices = new vector<u_int32_t>();
	  if(m_koOptions->ZLE())
	    SplitChannels(buffvec,sizevec,times,channels,eventIndices);
	  
	  else SplitChannels(buffvec,sizevec,times,channels,eventIndices,false);
	}
	else if(m_koOptions->GetProcessingOptions().Mode == 3) { //channel parsing new fw
	  SplitChannelsNewFW(buffvec,sizevec,times,channels);
	}
      }
      
      unsigned int currentEventIndex = 0;
      int                 protocHandle = -1;
      //Loop through the parsed buffers
      for(unsigned int b = 0; b < buffvec->size(); b++) {
	u_int32_t TimeStamp = 0;
	int       Channel    = -1;
	// Get time stamp if required
	if(m_koOptions->GetProcessingOptions().Mode==0 || 
	   m_koOptions->GetProcessingOptions().Mode==1) {
	  TimeStamp = GetTimeStamp((*buffvec)[b]);
	  Channel   = -1;
	}
	else {
	  TimeStamp = (*times)[b];
	  Channel   = (*channels)[b];
	}
	
	//Convert the time to 64-bit
	int ResetCounter = 0;
	if(m_DAQRecorder!=NULL) m_DAQRecorder->GetResetCounter(TimeStamp);
	long long Time64 = ((unsigned long)ResetCounter << 31) | TimeStamp;

	//zip data if required
	char* buff=NULL;
	size_t eventSize=0;
	if(m_koOptions->Compression()){
	  buff = new char[snappy::MaxCompressedLength((*sizevec)[b])];
	  snappy::RawCompress((const char*)(*buffvec)[b], (*sizevec)[b], buff, &eventSize);
	  delete[] (*buffvec)[b];
	}
	else{
	  buff = (char*)(*buffvec)[b];
	  eventSize = (*sizevec)[b];
	}

	//Now fill the actual data depending on write mode
#ifdef HAVE_LIBMONGOCLIENT
	if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_MONGODB){
	  mongo::BSONObjBuilder bson;
	  
	  //remove this later!
	  if(m_koOptions->GetMongoOptions().Collection=="online.scope"){
	    time_t currentTime;
	    struct tm *starttime;
	    time(&currentTime);
	    starttime = localtime(&currentTime);
	    bson.appendTimeT("starttimestamp",mktime(starttime));
	  }
	  //end remove later

	  bson.append("module",iModule);
	  bson.append("channel",Channel);
	  bson.append("time",Time64);
	  bson.appendBinData("data",(int)eventSize,mongo::BinDataGeneral,(const void*)buff);
	  vMongoInsertVec->push_back(bson.obj());
	  if((int)vMongoInsertVec->size()>m_koOptions->GetMongoOptions().MinInsertSize){
	    if(DAQRecorder_mdb->InsertThreaded(vMongoInsertVec,mongoID)==0){ //success
	      vMongoInsertVec = new vector<mongo::BSONObj>();
	    }
	    else{
	      LogError("MongoDB insert error from processor thread.");
	      bExitCondition=true;
	      delete vMongoInsertVec;
	      break;
	    }
	  }
	}
#endif
#ifdef HAVE_LIBPBF
	if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_FILE){
	  if(eventIndices == NULL || ( currentEventIndex<eventIndices->size()
				       && (*eventIndices)[currentEventIndex]==b) ){
	    if(protocHandle!=-1)
	      DAQRecorder_pb->GetOutfile()->close_event(protocHandle,true);
	    DAQRecorder_pb->GetOutfile()->create_event(TimeStamp,protocHandle);
	    if(eventIndices!=NULL) currentEventIndex++;
	  }
	  DAQRecorder_pb->GetOutfile()->add_data(protocHandle,Channel,iModule,buff,eventSize,Time64);
	  //special case for last event
	  if(b==buffvec->size()-1 && protocHandle!=-1)
	    DAQRecorder_pb->GetOutfile()->close_event(protocHandle,true);
	}
#endif
	delete[] buff;
      }//end loop through buffers
      if(channels!=NULL) delete channels;
      if(times!=NULL) delete times;
      if(buffvec!=NULL) delete buffvec;
      if(sizevec!=NULL) delete sizevec;
      if(eventIndices!=NULL) delete eventIndices;
      buffvec=NULL;
      eventIndices=sizevec=channels=times=NULL;      
    }//end loop through digis
  }//end while loop
  delete vMongoInsertVec;
  return;
}


	


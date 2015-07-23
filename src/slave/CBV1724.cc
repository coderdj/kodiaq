
// **********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File    : CBV1724.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 12.07.2013
// 
// Brief   : Class for Caen V1724 Digitizers
// 
// ***********************************************************
#include <iostream>
#include <iomanip>
#include <cstring>
#include <time.h>
#include "CBV1724.hh"
#include "DataProcessor.hh"
#include <koHelper.hh>
#ifdef HAVE_LIBMONGOCLIENT
#include "mongo/client/dbclient.h"
#endif

CBV1724::CBV1724()
{   
   fBLTSize=fBufferSize=0;
   bActivated=false;
   fBuffers=NULL;
   fSizes=NULL;
   fBufferOccSize = 0;
   fReadoutThresh=10;
   pthread_mutex_init(&fDataLock,NULL);
   pthread_cond_init(&fReadyCondition,NULL);
   i_clockResetCounter = 0;
   i64_blt_first_time = i64_blt_second_time = i64_blt_last_time = 0;
   bOver15 = false;
}

CBV1724::~CBV1724()
{
   ResetBuff();
   pthread_mutex_destroy(&fDataLock);
   pthread_cond_destroy(&fReadyCondition);
}

CBV1724::CBV1724(board_definition_t BoardDef, koLogger *kLog)
        :VMEBoard(BoardDef,kLog)
{
  fBLTSize=fBufferSize=0;
  bActivated=false;
  fBuffers=NULL;
  fSizes=NULL;
  fReadoutThresh=10;
  pthread_mutex_init(&fDataLock,NULL);
  pthread_cond_init(&fReadyCondition,NULL);
  i64_blt_first_time = i64_blt_second_time = i64_blt_last_time = 0;
  fBufferOccSize = 0;
  bOver15 = false;
}

int CBV1724::Initialize(koOptions *options)
{
  // Initialize and ready the board according to the options object

  // Set private members
  int retVal=0;
  i_clockResetCounter=0;
  bActivated=false;
  UnlockDataBuffer();
  
  //Get size of BLT read using board data and options
  u_int32_t data;
  ReadReg32(CBV1724_BoardInfoReg,data);
  u_int32_t memorySize = (u_int32_t)((data>>8)&0xFF);
  ReadReg32(CBV1724_BuffOrg,data);
  u_int32_t eventSize = (u_int32_t)((((memorySize*pow(2,20))/
				      (u_int32_t)pow(2,data))*8+16)/4);
  ReadReg32(CBV1724_BltEvNumReg,data);
  fBLTSize=options->GetInt("blt_size");
  fBufferSize = data*eventSize*(u_int32_t)4+(fBLTSize);
  fBufferSize = eventSize*data + fBLTSize;
  fReadoutThresh = options->GetInt("processing_readout_threshold");
   
  // Data is stored in these two vectors
  fBuffers = new vector<u_int32_t*>();
  fSizes   = new vector<u_int32_t>();

  // Determine baselines if required
  if(options->GetInt("baseline_mode")==1)    {	
    m_koLog->Message("Determining baselines ");
    int tries = 0;
    int ret=-1;
    while((ret=DetermineBaselines())!=0 && tries<5){
      cout<<" .";
      tries++;
      if(ret==-2){
	m_koLog->Error("Baselines failed with ret -2");
	return -2;
      }	 
    }
    stringstream logmess;
    logmess<<"Baselines returned value "<<ret;
    m_koLog->Message(logmess.str());
    if( ret == -1 )
      retVal = ret;
  }
  else m_koLog->Message("Automatic baseline determination switched off");

  // Reload VME options
  int tries = 0;
  while( LoadVMEOptions( options ) != 0 && tries < 5) {
    tries ++;
    usleep(100);
    if( tries == 5 )
      retVal = -1;
  }

  // Load baselines
  if(options->GetInt("baseline_mode") != 2){
    LoadBaselines();
    m_koLog->Message("Baselines loaded from file");
  }
  fBufferOccSize = 0;


  // Report whether we succeeded or not
  stringstream messstr;
  if( retVal == 0 ){
    messstr<<"Board "<<fBID.id<<" initialized successfully";
    m_koLog->Message( messstr.str() );
  }
  else{
    messstr<<"Board "<<fBID.id<<" failed initialization";
    m_koLog->Message( messstr.str() );
  }
   return retVal;
}

unsigned int CBV1724::ReadMBLT()
// Performs a FIFOBLT read cycle for this board and reads the
// data into the buffer of this CBV1724 object
{
  // Initialize
  unsigned int blt_bytes=0;
  int nb=0,ret=-5;   
   
  // The buffer must be freed in this function (fBufferSize can be large!)
  u_int32_t *buff = new u_int32_t[fBufferSize];       
  do{
    ret = CAENVME_FIFOBLTReadCycle(fCrateHandle,fBID.vme_address,
    				   ((unsigned char*)buff)+blt_bytes,
    				   fBLTSize,cvA32_U_BLT,cvD32,&nb);
    
    if((ret!=cvSuccess) && (ret!=cvBusError)){
      stringstream ss;
      ss<<"Board "<<fBID.id<<" reports read error "<<dec<<ret<<endl;
      LogError(ss.str());
      delete[] buff;
      return 0;
    }
    blt_bytes+=nb;
    if(blt_bytes>fBufferSize)	{
      // For Custom V1724 firmware max event size is ~10mus, corresponding to a 
      // buffer of several MB. Events which are this large are probably non
      // physical. Events going over the 10mus limit are simply ignored by the
      // board (!). 
      stringstream ss;	 
      ss<<"Board "<<fBID.id<<" reports insufficient BLT buffer size. ("
	<<blt_bytes<<" > "<<fBufferSize<<")"<<endl;	 
      m_koLog->Error(ss.str());
      delete[] buff;
      return 0;
    }
  }while(ret!=cvBusError);
   
  if(blt_bytes>0){
    // We reserve too much space for the buffer (block transfers can get long). 
    // In order to avoid shipping huge amounts of empty space around we copy
    // the buffer here to a new buffer that is just large enough for the data.
    // This memory is reserved here but it's ownership will be passed to the
    // processing function that drains it (that function must free it!)
    u_int32_t *writeBuff = new u_int32_t[blt_bytes/(sizeof(u_int32_t))]; 
    memcpy(writeBuff,buff,blt_bytes);
    LockDataBuffer();
    fBuffers->push_back(writeBuff);
    fSizes->push_back(blt_bytes);
    
    // Update total buffer size
    fBufferOccSize += blt_bytes;
    
    if(fBuffers->size()==1) i64_blt_first_time = koHelper::GetTimeStamp(buff);
    
    // If we have enough BLTs (user option) signal that board can be read out
    if(fBuffers->size()>fReadoutThresh)
      pthread_cond_signal(&fReadyCondition);
    UnlockDataBuffer();
  }

  delete[] buff;
  return blt_bytes;
}

void CBV1724::SetActivated(bool active)
// Set this board to active and ready to go
{
   bActivated=active;
   if(active==false){
      if(pthread_mutex_trylock(&fDataLock)==0){	      
	 pthread_cond_signal(&fReadyCondition);
	 pthread_mutex_unlock(&fDataLock);
      }
   }      
   i_clockResetCounter=0;
}

void CBV1724::ResetBuff()
{  
   LockDataBuffer();
   if(fBuffers!=NULL){      
      for(unsigned int x=0;x<fBuffers->size();x++)
	delete[] (*fBuffers)[x];
      delete fBuffers;
      fBuffers=NULL;
   }
   if(fSizes!=NULL){	
      delete fSizes;
      fSizes   = NULL;
   }
   
   UnlockDataBuffer();
}

int CBV1724::LockDataBuffer()
{
   int error = pthread_mutex_lock(&fDataLock);
   if(error==0) 
     return 0;   
   return -1;
}

int CBV1724::UnlockDataBuffer()
{
   int error = pthread_mutex_unlock(&fDataLock);
   if(error==0)
     return 0;  
   return -1;
}

int CBV1724::RequestDataLock()
{
   int error=pthread_mutex_trylock(&fDataLock);
   if(error!=0) return -1;
   struct timespec timeToWait;
   timeToWait.tv_sec = time(0)+3; //wait 3 seconds
   timeToWait.tv_nsec= 0;
   if(pthread_cond_timedwait(&fReadyCondition,&fDataLock,&timeToWait)==0)
     return 0;
   UnlockDataBuffer();
   return -1;
}

vector<u_int32_t*>* CBV1724::ReadoutBuffer(vector<u_int32_t> *&sizes, 
					   int &resetCounter, u_int32_t &headerTime)
// Note this PASSES OWNERSHIP of the returned vectors to the 
// calling function! They must be cleared by the caller!
// The reset counter is computed (did the clock reset during this buffer?) and
// updated if needed. The value of this counter at the BEGINNING of the buffer
// is passed by reference to the caller
{
  headerTime = 0;

  if(fBuffers->size()!=0 ) {
    i64_blt_first_time = koHelper::GetTimeStamp((*fBuffers)[0]);
    headerTime = koHelper::GetTimeStamp((*fBuffers)[0]);
    i64_blt_second_time = koHelper::GetTimeStamp((*fBuffers)[fBuffers->size()-1]);

    resetCounter = i_clockResetCounter;
    if( i64_blt_first_time < 5E8 && bOver15 )
      resetCounter++;

    // Is the object's over18 bool set?
    if( i64_blt_second_time <5E8 && bOver15 ){
      bOver15=false;
      i_clockResetCounter++;
    }
    else if( i64_blt_second_time > 15E8 && !bOver15 )
      bOver15 = true;

  }

   vector<u_int32_t*> *retVec = fBuffers;
   fBuffers = new vector<u_int32_t*>();
   sizes = fSizes;
   fSizes = new vector<u_int32_t>();

   // Reset total buffer size
   fBufferOccSize = 0;

   return retVec;
}

int CBV1724::LoadBaselines()
// Get Baselines from file and put into DAC
{  
   vector <int> baselines;
   if(GetBaselines(baselines)!=0) {
      LogMessage("Error loading baselines.");
      return -1;
   }   
   if(baselines.size()==0)
     return -1;
   return LoadDAC(baselines);
}

int CBV1724::GetBaselines(vector <int> &baselines, bool bQuiet)
// Baselines are stored in local file.
// Q: do we need to keep a record of these per run?
// Could provide them to the run doc somehow
{
  stringstream filename; 
  filename<<"baselines/XeBaselines_"<<fBID.id<<".ini";
  ifstream infile;
  infile.open(filename.str().c_str());
  if(!infile)  {
    stringstream error;
    error<<"No baselines found for board "<<fBID.id;
    LogError(error.str());
    LogSendMessage(error.str());
    return -1;
  }
  baselines.clear();
  
  // Get date and determine how old baselines are. 
  // If older than 24 hours give a warning.
  string line;
  getline(infile,line);
  unsigned int filetime = koHelper::StringToInt(line);
  //filetime of form YYMMDDHH
  if(koHelper::CurrentTimeInt()-filetime > 100 && !bQuiet)  {
    stringstream warning;   	   
    warning<<"Warning: Module "<<fBID.id
	   <<" is using baselines that are more than a day old.";
    LogSendMessage(warning.str());
  }
  while(getline(infile,line)){
    int value=0;
    if(koHelper::ProcessLineHex(line,koHelper::IntToString(baselines.size()+1),
				value)!=0)
      break;
    baselines.push_back(value);
  }
  if(baselines.size()!=8)   {
    stringstream error;
    error<<"Warning from module "<<fBID.id<<". Error loading baselines.";
    LogSendMessage(error.str());
    LogError(error.str());
    infile.close();
    return -1;
  }
  infile.close();
  return 0;      
}

int CBV1724::InitForPreProcessing(){
  //Get the firmware revision (for data formats)                                    
  u_int32_t fwRev=0;
  ReadReg32(0x118C,fwRev);
  int fwVERSION = ((fwRev>>8)&0xFF); //0 for old FW, 137 for new FW  
  int retval = 0;
  if(fwVERSION!=0)
    retval += (WriteReg32(CBV1724_ChannelConfReg,0x310) + 
	       WriteReg32(CBV1724_DPPReg,0x1310000) + 
	       WriteReg32(CBV1724_BuffOrg,0xA) +
	       WriteReg32(CBV1724_CustomSize,0xC8) + 
	       WriteReg32( 0x811C, 0x840 ) + 
	       WriteReg32(0x8000,0x310));
  else
    retval += (WriteReg32(CBV1724_ChannelConfReg,0x10) +
	       WriteReg32(CBV1724_DPPReg,0x800000));

  retval += (WriteReg32(CBV1724_AcquisitionControlReg,0x0) +
	     WriteReg32(CBV1724_TriggerSourceReg,0x80000000));
  retval += (WriteReg32( 0xEF24, 0x1) + WriteReg32( 0xEF1C, 0x1) + 
	     WriteReg32( 0xEF00, 0x10) + WriteReg32( 0x8120, 0xFF ));
  if(retval<0) 
    retval = -1;
  return retval;
}
int CBV1724::DoNoiseSpectra(string mongo_addr, string mongo_coll, u_int32_t length){

  // ONLY compatible with mongodb
#ifdef HAVE_LIBMONGOCLIENT

  // Requires C++11
  vector<vector<int>> valuesPerChannel(8,vector<int>());
  
  // Assume already initialized
  if(InitForPreProcessing()!=0 || WriteReg32(CBV1724_CustomSize,length)!=0){
    LogError("Can't load registers for preprocessing");
    return -1;
  }

  // Load baselines from local file into board
  vector <int> DACValues;
  if(GetBaselines(DACValues,true)!=0) {
    DACValues.resize(8,0x1000);
  }
  if(LoadDAC(DACValues)!=0) {
    LogError("Can't load to DAC!");
    return -1;
  }

  // Initialize connection to mongodb
  mongo::DBClientConnection *mongo = NULL;
  try{
    mongo::client::initialize();
    mongo = new mongo::DBClientConnection();
    mongo->connect(mongo_addr);
  }
  catch( ... ){
    LogError("Failed to connect to mongodb in noise spectra");
    cout<<"Noise spectra failed, could not connect to MongoDB at address "
	<<mongo_addr<<endl;
    delete mongo;
    return -1;
  }

  // Get the firmware revision
  u_int32_t fwRev=0;
  ReadReg32(0x118C,fwRev);
  int fwVERSION = ((fwRev>>8)&0xFF); //0 for old FW, 137 for new FW        

  do{

    // Enable to board      
    WriteReg32(CBV1724_AcquisitionControlReg,0x4);
    usleep(100);
    //Set Software Trigger            
    WriteReg32(CBV1724_SoftwareTriggerReg,0x1);
    usleep(100);
    //Disable the board                                   
    WriteReg32(CBV1724_AcquisitionControlReg,0x0);  
    
    //Read the data
    unsigned int readout = 0, thisread =0, counter=0;
    do{
      thisread = 0;
      thisread = ReadMBLT();
      readout+=thisread;
      usleep(100);
      counter++;
    } while( counter < 1000 && (readout == 0));// || thisread != 0)); 
    // Either the timer times out or the readout is non zero 
    // but the current read is finished
    if(readout == 0){
      LogError("Read failed in noise spectra function.");
      cout<<"Read failed in noise function."<<endl;
      if(mongo!=NULL)
	delete mongo;
      return -1;
    }
    
    // Use main kodiaq parsing
    int rc=0;
    u_int32_t ht=0;
    vector <u_int32_t> *dsizes;
    vector<u_int32_t*> *buff= ReadoutBuffer(dsizes, rc, ht);
    vector <u_int32_t> *dchannels = new vector<u_int32_t>;
    vector <u_int32_t> *dtimes = new vector<u_int32_t>;
    
    bool berr; string serr;
    if(fwVERSION!=0)
      DataProcessor::SplitChannelsNewFW(buff,dsizes,
					dtimes,dchannels,berr,serr);
    else
      DataProcessor::SplitChannels(buff,dsizes,dtimes,dchannels,NULL,false);
    
    for(unsigned int x=0;x<buff->size(); x++){
      if((*dsizes)[x]>100) (*dsizes)[x]=100;
      int max = DataProcessor::GetBufferMax((*buff)[x], (*dsizes)[x]);
      valuesPerChannel[(*dchannels)[x]].push_back(max);
      delete [] (*buff)[x];
    }
    
    delete buff;
    delete dsizes;
    delete dchannels;
    delete dtimes;

  } while(valuesPerChannel[0].size() < 500 );

  // Write to mongodb
  string collection = "noise.dump";
  mongo::BSONObj obj = mongo->findOne(mongo_coll,
				      mongo::Query().sort("_id",-1));
  if(!obj.isEmpty())
    collection = (string)("noise.") + (string)(obj.getStringField("collection"));
  cout<<"Inserting into noise collection: "<<collection<<endl;
  // Find the collection to write to 
  for(unsigned int x=0; x<valuesPerChannel.size(); x++){
    mongo::BSONObjBuilder bson;
    bson.append("module",fBID.id);
    bson.append("channel", x);
    bson.append("data", valuesPerChannel[x]);
    stringstream name;
    name.str(std::string());
    name<<"Noise m_"<<fBID.id<<"_ch_"<<x;
    bson.append("name", name.str());
    mongo->insert(collection, bson.obj());
  }

  if(mongo!=NULL)
    delete mongo;
  return 0;
#else
  return 0;
#endif
}

int CBV1724::DetermineBaselines()
//Rewrite of baseline routine from Marc S
//Updates to C++, makes compatible with new FW
{

  // If there are old baselines we can use them as a starting point
  vector <int> DACValues;  
  if(GetBaselines(DACValues,true)!=0) {
    DACValues.resize(8,0x1000);
  }

  //Load the old baselines into the board
  if(LoadDAC(DACValues)!=0) {
    LogError("Can't load to DAC!");
    return -1;
  }

  //Get the firmware revision (for data formats)                                    
  if(InitForPreProcessing()!=0){
    LogError("Can't load registers for baselines!");
    return -1;
  }
  //u_int32_t datasize = ( 524288 );  // 4byte/word, 8ch/digi, 16byte head
  u_int32_t fwRev=0;
  ReadReg32(0x118C,fwRev);
  int fwVERSION = ((fwRev>>8)&0xFF); //0 for old FW, 137 for new FW   

  //Do the magic
  double idealBaseline = 16000.;
  double maxDev = 2.;
  vector<bool> channelFinished(8,false);
  
  int maxIterations = 35;
  int currentIteration = 0;

  while(currentIteration<=maxIterations){    
    currentIteration++;

    //get out if all channels done
    bool getOut=true;
    for(unsigned int x=0;x<channelFinished.size();x++){
      if(channelFinished[x]==false) getOut=false;
    }
    if(getOut) break;
    
    // Enable to board
    WriteReg32(CBV1724_AcquisitionControlReg,0x4);
    usleep(5000); //
    //Set Software Trigger
    WriteReg32(CBV1724_SoftwareTriggerReg,0x1);
    usleep(5000); //
    //Disable the board
    WriteReg32(CBV1724_AcquisitionControlReg,0x0);
    usleep(5000); //        

    //Read the data                    
    unsigned int readout = 0, thisread =0, counter=0;
    do{
      thisread = 0;
      thisread = ReadMBLT();
      readout+=thisread;
      usleep(1000);
      counter++;
    } while( counter < 1000 && (readout == 0 || thisread != 0));
    // Either the timer times out or the readout is non zero but 
    //the current read is finished  
    if(readout == 0){
      LogError("Read failed in baseline function.");
      return -1;
    }

    // Use main kodiaq parsing     
    int rc=0;
    u_int32_t ht=0;
    vector <u_int32_t> *dsizes;
    vector<u_int32_t*> *buff= ReadoutBuffer(dsizes, rc, ht);

    vector <u_int32_t> *dchannels = new vector<u_int32_t>;
    vector <u_int32_t> *dtimes = new vector<u_int32_t>;

    bool berr; string serr;
    if(fwVERSION!=0)
      DataProcessor::SplitChannelsNewFW(buff,dsizes,
					dtimes,dchannels,berr,serr);
    else
      DataProcessor::SplitChannels(buff,dsizes,dtimes,dchannels,NULL,false);

    
    //loop through channels
    for(unsigned int x=0;x<dchannels->size();x++){
      if(channelFinished[(*dchannels)[x]] || (*dsizes)[x]==0) {
	delete[] (*buff)[x];
	continue;
      }

      //compute baseline
      double baseline=0.,bdiv=0.;
      unsigned int maxval=0.,minval=17000.;

      // Loop through data
      for(unsigned int y=0;y<(*dsizes)[x]/4;y++){
	// Second loop for first/second sample in word
	for(int z=0;z<2;z++){
	  u_int32_t dbase=0;
	  if(z==0) 
	    dbase=(((*buff)[x][y])&0xFFFF);
	  else 
	    dbase=(((*buff)[x][y]>>16)&0xFFFF);
	  baseline+=dbase;
	  bdiv+=1.;
	  if(dbase>maxval) 
	    maxval=dbase;
	  if(dbase<minval) 
	    minval=dbase;
	}      
      }
      baseline/=bdiv;
      if(maxval-minval > 500) {
	//stringstream error;
	//error<<"Channel "<<(*dchannels)[x]<<" signal in baseline?";
	//LogMessage( error.str() );	
	delete[] (*buff)[x];
	continue; //signal in baseline?
      }

      // shooting for 16000. best is if we UNDERshoot 
      // and then can more accurately adjust DAC
      double discrepancy = baseline-idealBaseline;      
      if(fabs(discrepancy)<=maxDev) { 
	channelFinished[(*dchannels)[x]]=true;
	delete[] (*buff)[x];
	continue;
      }
      
      if(discrepancy<0) //baseline is BELOW ideal
	DACValues[(*dchannels)[x]] = (int)DACValues[(*dchannels)[x]]
	  -((0xFFFF/2)*((-1*discrepancy)/(16383.)));
      else if(discrepancy<300) // find adj
	DACValues[(*dchannels)[x]] = (int)DACValues[(*dchannels)[x]]
	  +((0xFFFF/2)*((discrepancy/(16383.))));
      else //coarse adj
	DACValues[(*dchannels)[x]] = (int)DACValues[(*dchannels)[x]]+300;
      if(DACValues[(*dchannels)[x]]>0xFFFF) DACValues[(*dchannels)[x]]=0xFFFF;
      if(DACValues[(*dchannels)[x]]<0) DACValues[(*dchannels)[x]]=0;
      delete[] (*buff)[x];
    } //end loop through channels
    LoadDAC(DACValues);
    
    delete buff;
    delete dsizes;
    delete dchannels;
    delete dtimes;
    
  }//end while through iterations
  
  //write baselines to file
  ofstream outfile;
  stringstream filename; 
  filename<<"baselines/XeBaselines_"<<fBID.id<<".ini";                         
  outfile.open(filename.str().c_str());
  outfile<<koHelper::CurrentTimeInt()<<endl;
  for(unsigned int x=0;x<DACValues.size();x++)  {   
    outfile<<x+1<<"  "<<hex<<setw(4)<<setfill('0')<<
      ((DACValues[x])&0xFFFF)<<endl;                  
  }     
  outfile.close();  

  int retval=0;
  for(unsigned int x=0;x<channelFinished.size();x++){
    if(channelFinished[x]=false) {
      stringstream errstream;
      errstream<<"Didn't finish channel "<<x;
      LogError(errstream.str());
      retval=-1;
    }
  }

  return retval;

}

int CBV1724::LoadDAC(vector<int> baselines){

  if(baselines.size()!=8) {
    stringstream errorstr;
    errorstr<<"Baseline size incorrect, only "<<baselines.size()<<endl;
    LogError(errorstr.str());
    return -1;
  }

  for(unsigned int x=0;x<baselines.size();x++){
    usleep(100);
    
    int counter = 0;
    int othercounter = 0;
    while( counter < 100 && othercounter < 10000 ){
      
      u_int32_t data = 0x4;

      // Check DAC status to see if it's OK to write
      if( ReadReg32( (0x1088)+(0x100*x), data ) !=0 ){
	//stringstream errorst;
	//errorst<<"Error reading channel status register "<<hex
	//<<((0x1088)+(0x100*x))<<dec;
	//LogError(errorst.str());
	//return -1;
	usleep(100);
	othercounter++;
	continue;
      }
      
      if( data&0x4 ){
	counter++;
	usleep(1000);
	continue;
      }
      break;
    }
    
    if( counter >= 100 ){
      stringstream errorstr;
      errorstr<<"Timed out waiting for DAC to clear in channel "<<x;
      LogError(errorstr.str());
      return -1;
    }
    if( othercounter >= 10000 ){
      stringstream errorstre;
      errorstre<<"Timed out waiting for read to DAC status register in channel "<<x;
      LogError(errorstre.str());
      return -1;
    }

    if(WriteReg32((0x1098)+(0x100*x),baselines[x])!=0){      
      stringstream errors;
      errors<<"Error loading baseline "<<hex<<baselines[x]<<" to register "
	    <<((0x1098)+(0x100*x))<<dec;
      LogError(errors.str());
      return -1;
    }

    // Post check to make sure thing applies
    counter = 0;
    while( counter < 100 ){
      u_int32_t data = 0x4;

      if( ReadReg32( (0x1088)+(0x100*x), data ) !=0 ){
	usleep(1000);
	counter++;
	continue;
      }
      if( data&0x4 ){
	counter++;
	usleep(1000);
	continue;
      }
      break;
      stringstream goodstr;
      goodstr<<"Baseline for channel "<<x<<" written";
      LogMessage(goodstr.str());
    }
    
  } //end for
  return 0;
}

 int CBV1724::LoadVMEOptions( koOptions *options )
 {
   int retVal = 0;
   // Reload VME options    
   for(int x=0;x<options->GetVMEOptions();x++)  {
     if((options->GetVMEOption(x).board==-1 ||
	 options->GetVMEOption(x).board==fBID.id)){
       int success = WriteReg32(options->GetVMEOption(x).address,
				options->GetVMEOption(x).value);
       if( options->GetVMEOption(x).address == 0xEF24 ) // give time to reset
	 usleep(1000);
       if( success != 0 ){
	 retVal=success;
	 stringstream errorstr;
	 errorstr<<"Couldn't write VME option "<<hex
		 <<options->GetVMEOption(x).address<<" with value "
		 <<options->GetVMEOption(x).value<<" to board "<<fBID.id;
	 LogError( errorstr.str() );
       }
     }

   }
   return retVal;
 }

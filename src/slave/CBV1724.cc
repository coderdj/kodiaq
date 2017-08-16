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
#include <sys/types.h>
#include <sys/wait.h>

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
   fBufferOccCount = 0;
   fReadoutThresh=10;
   pthread_mutex_init(&fDataLock,NULL);
   pthread_mutex_init(&fWaitLock,NULL);
   pthread_cond_init(&fReadyCondition,NULL);
   i_clockResetCounter = 0;
   fBadBlockCounter = 0;
   i64_blt_first_time = i64_blt_second_time = i64_blt_last_time = 0;
   bOver15 = false;
   fIdealBaseline = 16000;
   fLastReadout=koLogger::GetCurrentTime();
   fReadoutTime = 1;
   m_lastprocessPID=0;
   fReadMeOut=false;
   m_tempBuff = NULL;
   bThreadOpen = false;
   m_temp_blt_bytes=0;
   bExists=false;
   bProfiling=false;
   fError=false;
   fErrorText="";
   bBoardBusy = false;
   fReadBusyLast = false;
}

CBV1724::~CBV1724()
{
  bExists=false;
  if(bThreadOpen)
    pthread_join(m_copyThread,NULL);
  bThreadOpen=false;

   ResetBuff();
   pthread_mutex_destroy(&fDataLock);
   pthread_mutex_destroy(&fWaitLock);
   pthread_cond_destroy(&fReadyCondition);
}

CBV1724::CBV1724(board_definition_t BoardDef, koLogger *kLog, bool profiling)
        :VMEBoard(BoardDef,kLog)
{
  fBLTSize=fBufferSize=0;
  bActivated=false;
  fBuffers=NULL;
  fSizes=NULL;
  fReadoutThresh=10;
  pthread_mutex_init(&fDataLock,NULL);
  pthread_mutex_init(&fWaitLock,NULL);
  pthread_cond_init(&fReadyCondition,NULL);
  i64_blt_first_time = i64_blt_second_time = i64_blt_last_time = 0;
  fBufferOccSize = 0;
  fBufferOccCount = 0;
  fBadBlockCounter = 0;
  bOver15 = false;
  fIdealBaseline = 16000;
  fLastReadout=koLogger::GetCurrentTime();
  fReadoutTime =  1;
  m_lastprocessPID=0;
  fReadMeOut=false;
  m_tempBuff = NULL;
  bThreadOpen=false;
  m_temp_blt_bytes = 0;
  bExists=false;
  bProfiling = profiling;
  fError=false;
  fErrorText="";  
  bBoardBusy = false;
  fReadBusyLast = false;
}

int CBV1724::Initialize(koOptions *options)
{
  // Initialize and ready the board according to the options object
  bExists=true;
  fBadBlockCounter = 0;
  fError = false;
  bBoardBusy = false;
  fErrorText = "";

  // Set private members
  int retVal=0;
  i_clockResetCounter=0;
  bActivated=false;
  UnlockDataBuffer();
  fBLTSize=options->GetInt("blt_size");
  fBufferSize = fBLTSize;
  if(options->HasField("read_busy_last") && options->GetInt("read_busy_last")==1)
    fReadBusyLast = true;
  else
    fReadBusyLast = false;
  fReadoutThresh = options->GetInt("processing_readout_threshold");
  if(options->HasField("baseline_level"))
    fIdealBaseline = options->GetInt("baseline_level");

  // Data is stored in these two vectors
  fBuffers = new vector<u_int32_t*>();
  fSizes   = new vector<u_int32_t>();

  // Determine baselines if required
  if(options->HasField("baseline_mode") && 
     options->GetInt("baseline_mode")==1)    {	
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
  while( LoadVMEOptions( options, false ) != 0 && tries < 5) {
    tries ++;
    usleep(100);
    if( tries == 5 )
      retVal = -1;
  }

  // Load baselines
  if(!options->HasField("baseline_mode") ||
     options->GetInt("baseline_mode") != 2){
    LoadBaselines();
    m_koLog->Message("Baselines loaded from file");
  }
  fBufferOccSize = 0;
  fBufferOccCount = 0;
  fReadoutReports.clear();

  // Load user registers for boards with a unique ID only 
  // this is to avoid user settings to be overwritten
  tries = 0;
  while( LoadVMEOptions( options,true ) != 0 && tries < 5) {
     tries ++;
     usleep(100);
     if( tries == 5 )                         
     retVal = -1;
  }

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

  i_clockResetCounter=0;
  i64_blt_last_time = 0;
   return retVal;
}

int CBV1724::GetBufferSize(int &count, vector<string> &reports){
  LockDataBuffer();
  count = fBufferOccCount;
  reports = fReadoutReports;
  fReadoutReports.clear();
  UnlockDataBuffer();
  return fBufferOccSize; 
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

      // Log the entire buffer contents
      unsigned int bindex = 0;
      stringstream ess;
      while(bindex < blt_bytes/sizeof(u_int32_t)){
	ess<<buff[bindex]<<endl;	
      }
      LogError(ess.str());
	
      delete[] buff;
      return 0;
    }

    blt_bytes+=nb;
    if(blt_bytes>fBufferSize){
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
   
  // New: If the BLT is less than 6 words then count it and dump it
  if(blt_bytes < 24 && blt_bytes!=0) {
    fBadBlockCounter++;
    if(fBadBlockCounter%1000==0){
      stringstream ss;
      ss<<"Board "<<fBID.id<<" found a total of "<<fBadBlockCounter<<
	" blocks under 24 bytes."<<endl;
      if(fBadBlockCounter > 1000000){
	// Kill it
	fError = true;
	fErrorText = "Found 1,000,000 bad CAEN blocks. You're paying them too much";
	fBadBlockCounter=0; // Give time for run to stop
      }
      m_koLog->Error(ss.str());
    }
    // Still increment counter
    fBufferOccCount++;
    

    // This triggers a deletion of the buffer without recording    
    blt_bytes = 0;
  }

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
    fBufferOccCount++;

    if(bProfiling && m_profilefile.is_open())
      m_profilefile<<"READ "<<koLogger::GetTimeMus()<<" "<<blt_bytes<<" "<<
	fBufferOccSize<<" "<<fBuffers->size()<<endl;
   
    
    // Priority. If we defined a time stamp frequency, signal the readout
    time_t current_time=koLogger::GetCurrentTime();
    double tdiff = difftime(current_time, fLastReadout);
    
    // If we have enough BLTs (user option) signal that board can be read out
    if(fBuffers->size()>fReadoutThresh || fabs(tdiff) > fReadoutTime){
      fReadMeOut=true;
      if(bProfiling && m_profilefile.is_open())
	m_profilefile<<"SIGNAL "<<koLogger::GetTimeMus()<<" "<<blt_bytes<<" "<<
	  fBufferOccSize<<" "<<fBuffers->size()<<" "<<tdiff<<endl;
    }
    UnlockDataBuffer();
  }

  delete[] buff;
  return blt_bytes;
}

void CBV1724::SetActivated(bool active)
// Set this board to active and ready to go
{
  if(active)
    i_clockResetCounter=0;
   bActivated=active;
   if(active==false){
     cout<<"Signaling final read"<<endl;

      if(pthread_mutex_trylock(&fDataLock)==0){	      
	//pthread_cond_signal(&fReadyCondition);
	fReadMeOut=true;
	pthread_mutex_unlock(&fDataLock);
      }
      cout<<"Done"<<endl;

      if(m_profilefile.is_open())
	m_profilefile.close();
   }      
   if(bProfiling && !m_profilefile.is_open())
     m_profilefile.open("profiling/profile_digi_"+koHelper::IntToString(fBID.id)+".txt", std::fstream::app);
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
   fBufferOccSize = 0;
   fBufferOccCount = 0;
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
  if(fReadMeOut){
    
    
    if(fReadBusyLast){
      // Check here if board is using more than 1000 blocks
      // if so set the board busy flag. This board should be
      // cleared LAST (or at least *a* busy board should be cleared
      // last) giving larger blocks of live time. 
      std::vector<u_int32_t> channelBufferRegs = {0x1094, 0x1194, 0x1294, 0x1394, 
						  0x1494, 0x1594, 0x1694, 0x1794};
      
      if(!bBoardBusy){      
	for(unsigned int x=0;x<channelBufferRegs.size();x++){
	  u_int32_t ch=0;
	  ReadReg32(channelBufferRegs[x],ch);	
	  if(ch >= 1023){
	    bBoardBusy = true;
	    return -1;
	  }
	}
      }
    }
    
    int error = pthread_mutex_trylock(&fDataLock);
    bBoardBusy = false;
    if(error!=0) return -1;
    return 0;
  }
  return -1;
}

vector<u_int32_t*>* CBV1724::ReadoutBuffer(vector<u_int32_t> *&sizes, 
					   unsigned int &resetCounter, 
					   u_int32_t &headerTime,
					   int m_ID)
// Note this PASSES OWNERSHIP of the returned vectors to the 
// calling function! They must be cleared by the caller!
// The reset counter is computed (did the clock reset during this buffer?) and
// updated if needed. The value of this counter at the BEGINNING of the buffer
// is passed by reference to the caller
// THE MUTEX MUST BE LOCKED IF THIS FUNCTION IS CALLED
{
  fReadMeOut=false;
  headerTime = 0;
  fLastReadout=koLogger::GetCurrentTime();

  // Memory management, pass pointer to caller *with ownsership*                      
  vector<u_int32_t*> *retVec = fBuffers;
  fBuffers = new vector<u_int32_t*>();
  sizes = fSizes;
  fSizes = new vector<u_int32_t>();
  long int occSize = fBufferOccSize;
  fBufferOccSize = 0;
    
  if(retVec->size()!=0 ) {
    i64_blt_first_time = koHelper::GetTimeStamp((*retVec)[0]);
    
    // CAEN farts. I don't necessarily want the first time, I want the first
    // one that isn't garbage. If all are garbage skip checking I guess.
    unsigned int iPosition = 1;
    if(i64_blt_first_time == 0xFFFFFFFF){
      
      // Keep searching for a good one
      for(unsigned int x=1; x<retVec->size(); x++){
	i64_blt_first_time = koHelper::GetTimeStamp((*retVec)[x]);
	if(i64_blt_first_time != 0xFFFFFFFF){
	  iPosition = x;
	  LogError("CAEN fart on header time for BLT[0] but found a good one at position " + koHelper::IntToString(x));
	  break;
	}
      }
      
    }

    // If it still sucks dump it to threads and see what they make of it
    if(i64_blt_first_time == 0xFFFFFFFF){
      headerTime = i64_blt_last_time;
      i64_blt_first_time = headerTime;
      resetCounter = i_clockResetCounter;
      LogError("Irrecoverable CAEN fart with "+koHelper::IntToString(int(retVec->size()))+" buffers.");
      UnlockDataBuffer();
      return retVec;
    }


    headerTime = i64_blt_first_time;
    resetCounter = i_clockResetCounter;

    // If the clock reset before this buffer, then we can 
    // record a clock reset here
    // CASE 1: CLOCK RESET AT T0
    if(i64_blt_last_time > i64_blt_first_time && 
       fabs(long(i64_blt_first_time)-long(i64_blt_last_time))>5E8){
      i_clockResetCounter ++;
      resetCounter = i_clockResetCounter;

      //stringstream st;
      /*st<<"Clock reset condition 1: board "<<fBID.id<<
	": first time ("<<i64_blt_first_time
        <<") last time ("<<i64_blt_last_time<<") reset counter ("
	<<i_clockResetCounter<<") size of buffer vector ("<<retVec->size()<<")";
      LogError(st.str());
      */

    }
    
    // Now check for resets within the buffer
    i64_blt_last_time = i64_blt_first_time;
    for(unsigned int x=iPosition; x<retVec->size(); x++){
      u_int64_t thisTime = koHelper::GetTimeStamp((*retVec)[x]);
      if(thisTime == 0xFFFFFFFF){
	cout<<"Junk data!"<<endl;
	continue;
      }
      if(thisTime < i64_blt_last_time && 
	 fabs(long(thisTime)-long(i64_blt_last_time))>5E8){
	i_clockResetCounter++;
	/*stringstream st;
	st<<"Clock reset condition 2: board "<<fBID.id<<
	  ": this time ("<<thisTime
	  <<") last time ("<<i64_blt_last_time<<") reset counter ("
	  <<i_clockResetCounter<<") size of buffer vector ("<<retVec->size()<<")";
	LogError(st.str());
	*/
      }
      i64_blt_last_time = thisTime;
    }

    // REMOVE THIS IF LATER
    /*if(reset != i_clockResetCounter){
      stringstream st;
      st<<"Clock reset board "<<fBID.id<<": first time ("<<i64_blt_first_time
	<<") second time ("<<i64_blt_second_time<<") last time ("<<
	i64_blt_last_time<<") reset counter ("<<resetCounter
	<<") size of buffer vector ("<<retVec->size()<<")";
      LogError(st.str());
      } */ 

    // Now we have another issue. It can be that we have very unphysical data
    // where the clock resets many, many times in a buffer. If we get one 
    // of these buffers dump the ENTIRE BINARY HEADER to a special file.
    // It will be a lot
    /*if( i_clockResetCounter - reset > 5){ // 5 resets seems suitably crappy
      ofstream outfile;
      stringstream filename;
      filename<<"crapData_"<<fBID.id<<"_"<<i_clockResetCounter<<".txt";
      outfile.open(filename.str());
      
      // Print each word      
      outfile<<koLogger::GetTimeString()<<" Record for digi "<<fBID.id
	     <<" with "<<i_clockResetCounter-reset<<
	" clock resets and a buffer size of "<<(*fSizes)[0]<<" and "
	     <<fBuffers->size()<<" buffers."<<endl;
      for(unsigned int bidx=0; bidx<fBuffers->size(); bidx++){	
	for(unsigned int idx = 0; idx < (*fSizes)[bidx]/4; idx++){
	  outfile<<hex<<(*fBuffers)[bidx][idx]<<endl;
	}
      }
      outfile.close();
    }// end remove later
    */
  }
  
  // PROFILING                  
  if(m_ID != -1){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    unsigned long time_us = 1000000 * tv.tv_sec + tv.tv_usec;
    stringstream ss;
    ss<<fBID.id<<" "<<m_ID<<" "<<time_us<<" "<<occSize<<" "<<retVec->size();
    fReadoutReports.push_back(ss.str());
  }
  
  /*// Memory management, pass pointer to caller *with ownsership*
   vector<u_int32_t*> *retVec = fBuffers;
   fBuffers = new vector<u_int32_t*>();
   sizes = fSizes;
   fSizes = new vector<u_int32_t>();
  */

// Turn this on if you want a line printed for each readout
   //cout<<"READING OUT "<<fBID.id<<" with old size: "<<retVec->size()<<" and new size "<<retVec->size()<<endl;
   if(bProfiling && m_profilefile.is_open())
     m_profilefile<<"CLEAR "<<koLogger::GetTimeMus()<<" "<<retVec->size()<<" "
		  <<m_ID<<endl;

   UnlockDataBuffer();

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
  cout<<"Preprocess for FW version " << hex << fwVERSION << dec <<endl;

  if(fwVERSION!=0)
    retval += (WriteReg32(CBV1724_ChannelConfReg,0x310) + 
	       WriteReg32(CBV1724_DPPReg,0x1310000) + 
	       WriteReg32(CBV1724_BuffOrg,0xA) +
	       //WriteReg32(CBV1724_CustomSize,0xC8) + 
	       WriteReg32(CBV1724_CustomSize, 0x1F4) +
	       WriteReg32( 0x811C, 0x840 ) + 
	       WriteReg32(0x8000,0x310)
	       );
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

int CBV1724::DetermineBaselines()
//Rewrite of baseline routine from Marc S
//Updates to C++, makes compatible with new FW
{
  // First thing's first: Reset the board
  WriteReg32(CBV1724_BoardResetReg, 0x1);

  // If there are old baselines we can use them as a starting point
  vector <int> DACValues;  
  //if(GetBaselines(DACValues,true)!=0) {
  DACValues.resize(8,0xFFFF - fIdealBaseline);
    //}

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
  LogMessage("Baselines for firmware version " + koHelper::IntToString(fwVERSION) );

  //Do the magic
  double idealBaseline = (double)fIdealBaseline;
  double maxDev = 5.;
  //vector<bool> channelFinished(8,false);
  vector<int> channelFinished(8, 0);
  
  int maxIterations = 1000;
  int currentIteration = 0;

  while(currentIteration<=maxIterations){    
    currentIteration++;

    //get out if all channels done
    bool getOut=true;
    for(unsigned int x=0;x<channelFinished.size();x++){
      //if(channelFinished[x]==false) getOut=false;
      if(channelFinished[x]<5) getOut=false;
    }
    if(getOut) break;
    
    // Enable to board
    WriteReg32(CBV1724_AcquisitionControlReg,0x24);
    //usleep(5000); //
    //Set Software Trigger
    WriteReg32(CBV1724_SoftwareTriggerReg,0x1);
    usleep(50); //

    //Disable the board
    WriteReg32(CBV1724_AcquisitionControlReg,0x0);
    //usleep(5000);

    //Read the data                    
    unsigned int readout = 0, thisread =0, counter=0;
    do{
      thisread = 0;
      thisread = ReadMBLT();
      readout+=thisread;
      usleep(10);
      counter++;
    } while( counter < 1000 && (readout == 0 || thisread != 0));
    // Either the timer times out or the readout is non zero but 
    //the current read is finished  
    if(readout == 0){
      LogError("Read failed in baseline function.");
      continue;
      //return -1;
    }

    // Use main kodiaq parsing     
    unsigned int rc=0;
    u_int32_t ht=0;
    vector <u_int32_t> *dsizes;
    LockDataBuffer();
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
      if(channelFinished[(*dchannels)[x]]>=5 || (*dsizes)[x]==0) {
	delete[] (*buff)[x];
	continue;
      }

      //compute baseline
      double baseline=0.,bdiv=0.;
      int maxval=-1,minval=17000;

      // Loop through data
      for(unsigned int y=0;y<(*dsizes)[x]/4;y++){
	// Second loop for first/second sample in word
	for(int z=0;z<2;z++){
	  int dbase=0;
	  if(z==0) 
	    dbase=(((*buff)[x][y])&0xFFFF);
	  else 
	    dbase=(((*buff)[x][y]>>16)&0xFFFF);
	  if(dbase == 0 || dbase == 4) 
	    continue;
	  baseline+=dbase;
	  bdiv+=1.;
	  if(dbase>maxval) 
	    maxval=dbase;
	  if(dbase<minval) 
	    minval=dbase;
	}      
      }
      baseline/=bdiv;
      if(abs(maxval-minval) > 100) {
	//stringstream error;
	//error<<"Channel "<<(*dchannels)[x]<<" signal in baseline?";
	//LogMessage( error.str() );	
	
	LogMessage("maxval - minval for about " + 
		   koHelper::IntToString(fBID.id) + " is " + 
		   koHelper::IntToString(abs(maxval-minval)) + ", max " + 
		   koHelper::IntToString(maxval) + " min " 
		   + koHelper::IntToString(minval) + 
		   " maybe there's a signal in the baseline." + 
		   " Event length " + koHelper::IntToString((*dsizes)[x]/4) 
		   + " words.");
	
	delete[] (*buff)[x];
	continue; //signal in baseline?
      }

      // shooting for 16000. best is if we UNDERshoot 
      // and then can more accurately adjust DAC
      double discrepancy = baseline-idealBaseline;      
      //LogMessage("Discrepancy is " + koHelper::IntToString(discrepancy));
      if(abs(discrepancy)<=maxDev) { 

	if(channelFinished[(*dchannels)[x]]>=5){
	  stringstream message;
	  message<<"Board "<<fBID.id<< " Channel "<< (*dchannels)[x]
		 <<" finished with value "<<baseline
		 <<" discrepancy: "<<discrepancy<<" and value "
		 <<DACValues[(*dchannels)[x]]<<endl;
	  LogMessage(message.str());
	}
	
	//channelFinished[(*dchannels)[x]]=true;
	channelFinished[(*dchannels)[x]]+=1;
	delete[] (*buff)[x];
	continue;
      }
      channelFinished[(*dchannels)[x]]=0;


      // Have a range of 0xFFFF
      u_int32_t offset = 1000;
      if(abs(discrepancy)<1000)
	offset = fabs(discrepancy);
      if(abs(discrepancy) < 500)
	offset = 100;
      if(abs(discrepancy) < 100)
	offset = 10;
      if(abs(discrepancy) < 50)
	offset = 5;
      if(abs(discrepancy) < 20)
	offset = 2;
      if(abs(discrepancy) < 5)
	offset = 1;
      
      if(discrepancy < 0)
	DACValues[(*dchannels)[x]] -= offset;
      else 
	DACValues[(*dchannels)[x]] += offset;
      
      // Check out of bounds
      if(DACValues[(*dchannels)[x]] <= 0)
	DACValues[(*dchannels)[x]] = 0x0;
      if(DACValues[(*dchannels)[x]] >= 0xFFFF)
	DACValues[(*dchannels)[x]] = 0xFFFF;

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
    //if(channelFinished[x]=false) {
    if(channelFinished[x]<5){
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
      goodstr<<"Baseline for channel "<<x<<" written as "<<hex<<data<<dec;
      LogMessage(goodstr.str());
    }
    if(counter==100){
      LogError("Failed to set baseline for channel " + koHelper::IntToString(x));
      return -1;
    }
    
  } //end for
  return 0;
}

 int CBV1724::LoadVMEOptions( koOptions *options, bool unique )
 {
   int retVal = 0;
   // Reload VME options    
   for(int x=0;x<options->GetVMEOptions();x++)  {
     if( ( (options->GetVMEOption(x).board==-1 && unique == false) ||
	 options->GetVMEOption(x).board==fBID.id)){
 
       // only reload registers for DAC
       if( (options->GetVMEOption(x).address & 0x98)!=0x98)
	       continue;	      
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


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

CBV1724::CBV1724()
{   
   fBLTSize=fBufferSize=0;
   bActivated=false;
   fBuffers=NULL;
   fSizes=NULL;
   fReadoutThresh=10;
   pthread_mutex_init(&fDataLock,NULL);
   pthread_cond_init(&fReadyCondition,NULL);
   i_clockResetCounter = 0;
   i64_blt_first_time = i64_blt_second_time = i64_blt_last_time = 0;
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
}

int CBV1724::Initialize(koOptions *options)
{
   int retVal=0;
   i_clockResetCounter=0;
   for(int x=0;x<options->GetVMEOptions();x++)  {
     if((options->GetVMEOption(x).board==-1 || 
	 options->GetVMEOption(x).board==fBID.id)){
       int success = WriteReg32(options->GetVMEOption(x).address,
				options->GetVMEOption(x).value);    
       retVal=success;	 
     }      
   }
   
   //Reset all values
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
   fBLTSize=options->blt_size;
   fBufferSize = data*eventSize*(u_int32_t)4+(fBLTSize);
   fBufferSize = eventSize*data + fBLTSize;
   fReadoutThresh = options->processing_readout_threshold;
   
   fBuffers = new vector<u_int32_t*>();
   fSizes   = new vector<u_int32_t>();

   if(options->baseline_mode==1)    {	
      cout<<"Determining baselines ";
      int tries = 0;
      int ret=-1;
      while((ret=DetermineBaselines())!=0 && tries<5){
	 cout<<" .";
	 tries++;
	 if(ret==-2){
	    cout<<"Failed"<<endl;
	    return -2;
	 }	 
      }
      if(ret==0)
	cout<<" . . . done!"<<endl;
      else
	cout<<" . . . failed!"<<endl;
   }
   else cout<<"Didn't determine baselines."<<endl;
   
   cout<<"Loading baselines"<<endl;
   LoadBaselines();
   cout<<"Done with baselines."<<endl;
   return retVal;
}

unsigned int CBV1724::ReadMBLT()
{
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
	 ss<<"Board "<<fBID.id<<" reports insufficient BLT buffer size. ("<<blt_bytes<<" > "<<fBufferSize<<")"<<endl;	 
	 cout<<ss.str()<<endl;
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
{
   bActivated=active;
   if(active==false){
      sleep(1);
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
					   int &resetCounter)
// Note this PASSES OWNERSHIP of the returned vectors to the 
// calling function! They must be cleared by the caller!
// The reset counter is computed (did the clock reset during this buffer?) and
// updated if needed. The value of this counter at the BEGINNING of the buffer
// is passed by reference to the caller
{
  if(fBuffers->size()!=0)
    i64_blt_second_time = koHelper::GetTimeStamp((*fBuffers)[fBuffers->size()-1]);
  else i64_blt_second_time = i64_blt_first_time;

  resetCounter = i_clockResetCounter;
  //Q1: Did the counter reset between the last BLT and now?
  if(i64_blt_last_time>i64_blt_first_time){
    i_clockResetCounter++;
    resetCounter = i_clockResetCounter;
    //    i64_blt_second_time += ((unsigned long) 1 << 31);
  }
  //Q2: Did the counter reset during this BLT?
  else if(i64_blt_first_time>i64_blt_second_time){
    i_clockResetCounter++;
    //    i64_blt_second_time += ((unsigned long) 1 << 31);
  }
 
   vector<u_int32_t*> *retVec = fBuffers;
   fBuffers = new vector<u_int32_t*>();
   sizes = fSizes;
   fSizes = new vector<u_int32_t>();
   i64_blt_last_time = i64_blt_first_time = i64_blt_second_time;

   return retVec;
}

int CBV1724::LoadBaselines()
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
   //Get date and determine how old baselines are. If older than 24 hours give a warning.
   string line;
   getline(infile,line);
   unsigned int filetime = koHelper::StringToInt(line);
   //filetime of form YYMMDDHH
   if(koHelper::CurrentTimeInt()-filetime > 100 && !bQuiet)  {
      stringstream warning;   	   
      warning<<"Warning: Module "<<fBID.id<<" is using baselines that are more than a day old.";
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
  if(LoadDAC(DACValues)!=0) return -1;

  // Record all register values before overwriting (will put back later)
  u_int32_t reg_DPP,reg_ACR,reg_SWTRIG,reg_CConf,reg_BuffOrg,reg_CustomSize,
    reg_PT;

  //Get the firmware revision (for data formats)                                     
  u_int32_t fwRev=0;
  //ReadReg32(0x8124,fwRev);
  ReadReg32(0x118C,fwRev);
  int fwVERSION = ((fwRev>>8)&0xFF); //0 for old FW, 137 for new FW
  int fwVERSIONOTHER = (fwRev)&0xFF;
  cout<<fwVERSION<<" VERSION "<<fwVERSIONOTHER<<endl;
  //Channel configuration 0x8000 - turn off ZLE/VETO
  ReadReg32(CBV1724_ChannelConfReg,reg_CConf);
  if(fwVERSION!=0)
    WriteReg32(CBV1724_ChannelConfReg,0x310);  
  else 
    WriteReg32(CBV1724_ChannelConfReg,0x10);
  //Acquisition control register 0x8100 - turn off S-IN
  ReadReg32(CBV1724_AcquisitionControlReg,reg_ACR);
  WriteReg32(CBV1724_AcquisitionControlReg,0x0);
  //Trigger source reg - turn off TRIN, turn on SWTRIG
  ReadReg32(CBV1724_TriggerSourceReg,reg_SWTRIG);
  WriteReg32(CBV1724_TriggerSourceReg,0x80000000);
  //Turn off DPP (for old FW should do nothing [no harm either])
  ReadReg32(0x1080,reg_DPP);
  if(fwVERSION!=0)
    WriteReg32(CBV1724_DPPReg,0x1310000);  
  else 
    WriteReg32(CBV1724_DPPReg,0x800000);
  //Change buffer organization so that custom size works 
  ReadReg32(CBV1724_BuffOrg,reg_BuffOrg);
  if(fwVERSION!=0)
    WriteReg32(CBV1724_BuffOrg,0xA);
  //Make the acquisition window a reasonable size (400 samples == 4 mus)
  ReadReg32(CBV1724_CustomSize,reg_CustomSize);
  if(fwVERSION!=0)
    WriteReg32(CBV1724_CustomSize,0xC8);
  //PTWindow can be little
  ReadReg32(0x1038,reg_PT);
  if(fwVERSION!=0)
    WriteReg32(0x8038,0x10);
  

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
    
    //Read the data in 
    int ret=0,nb=0;
    u_int32_t blt_bytes=0;
    vector<u_int32_t*> *buff=new vector<u_int32_t*>;
    u_int32_t *tempBuff = new u_int32_t[fBufferSize];
    buff->push_back(tempBuff);

    do{
      ret = CAENVME_FIFOBLTReadCycle(fCrateHandle,fBID.vme_address,((unsigned char*)(*buff)[0])+blt_bytes,fBLTSize,cvA32_U_BLT,cvD32,&nb);
      if(ret!=cvSuccess && ret!=cvBusError) {
	cout<<"CAENVME Read error, baselines, "<<ret<<endl;
	delete[] buff;
	return -2;
      }
      blt_bytes+=nb;
      if(blt_bytes>fBufferSize) 
	continue;
    }while(ret!=cvBusError);
    if(blt_bytes==0)
      continue;
    //Use dataprocessor methods to parse data
    vector <u_int32_t> *dsizes = new vector<u_int32_t>;
    dsizes->push_back(blt_bytes);
    vector <u_int32_t> *dchannels = new vector<u_int32_t>;
    vector <u_int32_t> *dtimes = new vector<u_int32_t>;
    if(fwVERSION!=0) DataProcessor::SplitChannelsNewFW(buff,dsizes,dtimes,dchannels);
    else DataProcessor::SplitChannels(buff,dsizes,dtimes,dchannels,NULL,false);
        //loop through channels
    for(unsigned int x=0;x<dchannels->size();x++){
      if(channelFinished[(*dchannels)[x]] || (*dsizes)[x]==0) {
	delete[] (*buff)[x];
	continue;
      }
      //compute baseline
      double baseline=0.,bdiv=0.;
      unsigned int maxval=0.,minval=17000.;
      //cout<<"Channel: "<<(*dchannels)[x]<<endl;
      for(unsigned int y=0;y<(*dsizes)[x]/4;y++){
	for(int z=0;z<2;z++){
	  u_int32_t dbase=0;
	  if(z==0) dbase=(((*buff)[x][y])&0xFFFF);
	  else dbase=(((*buff)[x][y]>>16)&0xFFFF);
	  //	  cout<<hex<<dbase<<endl;
	  baseline+=dbase;
	  bdiv+=1.;
	  if(dbase>maxval) maxval=dbase;
	  if(dbase<minval) minval=dbase;
	}      
      }
      baseline/=bdiv;
      if(maxval-minval > 500) {
	cout<<"Channel "<<(*dchannels)[x]<<" signal in baseline?"<<endl;
	continue; //signal in baseline?
      }
      //shooting for 16000. best is if we UNDERshoot and then can more accurately adjust DAC
      double discrepancy = baseline-idealBaseline;      
      //cout<<"Discrepancy "<<discrepancy<<" old BL: "<<DACValues[(*dchannels)[x]]<<endl;
      if(fabs(discrepancy)<=maxDev) { 
	channelFinished[(*dchannels)[x]]=true;
	continue;
      }
      if(discrepancy<0) //baseline is BELOW ideal
	DACValues[(*dchannels)[x]] = (int)DACValues[(*dchannels)[x]]-((0xFFFF/2)*((-1*discrepancy)/(16383.)));
      else if(discrepancy<300) // find adj
	DACValues[(*dchannels)[x]] = (int)DACValues[(*dchannels)[x]]+((0xFFFF/2)*((discrepancy/(16383.))));
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
    outfile<<x+1<<"  "<<hex<<setw(4)<<setfill('0')<<((DACValues[x])&0xFFFF)<<endl;                                                                                  
  }                                                                                 
  outfile.close();  
  //Put everyhting back to how it was
  WriteReg32(CBV1724_ChannelConfReg,reg_CConf);
  WriteReg32(CBV1724_AcquisitionControlReg,reg_ACR);
  WriteReg32(CBV1724_TriggerSourceReg,reg_SWTRIG);
  WriteReg32(CBV1724_DPPReg,reg_DPP);
  WriteReg32(CBV1724_BuffOrg,reg_BuffOrg);
  WriteReg32(CBV1724_CustomSize,reg_CustomSize);
  WriteReg32(0x8038,reg_PT);
  int retval=0;
  for(unsigned int x=0;x<channelFinished.size();x++){
    if(channelFinished[x]=false) retval=-1;
  }

  return retval;

}

int CBV1724::LoadDAC(vector<int> baselines){
  if(baselines.size()!=8) return -1;
  for(unsigned int x=0;x<baselines.size();x++){
    if(WriteReg32((0x1098)+(0x100*x),baselines[x])!=0){
      LogSendMessage("Error loading old baselines");
      return -1;
    }
  }
  return 0;
}


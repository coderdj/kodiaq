
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

#define AcquisitionControlReg 0x8100
#define SoftwareTriggerReg    0x8108
#define ZLEReg                0x8000

CBV1724::CBV1724()
{   
   fBLTSize=fBufferSize=0;
   bActivated=false;
   fBuffers=NULL;
   fSizes=NULL;
   fReadoutThresh=10;
}

CBV1724::~CBV1724()
{
   ResetBuff();
   pthread_mutex_destroy(&fDataLock);
   pthread_cond_destroy(&fReadyCondition);
}

int CBV1724::Initialize(XeDAQOptions *options)
{
   int retVal=0;
   cout<<"Initializing"<<endl;
   for(int x=0;x<options->GetVMEOptions();x++)  {
      if((options->GetVMEOption(x).BoardID==-1 || options->GetVMEOption(x).BoardID==fBID.BoardID)
	 && (options->GetVMEOption(x).CrateID==-1 || options->GetVMEOption(x).CrateID==fBID.CrateID)
	 && (options->GetVMEOption(x).LinkID==-1 || options->GetVMEOption(x).LinkID==fBID.LinkID)){
	 int success = WriteReg32(options->GetVMEOption(x).Address,options->GetVMEOption(x).Value);    
	 retVal=success;	 
      }      
   }
   
   //Reset all values
   bActivated=false;
   int a = pthread_mutex_init(&fDataLock,NULL);   
   if(a<0) return -1;
   a = pthread_cond_init(&fReadyCondition,NULL);
   if(a<0) return -1;
   UnlockDataBuffer();
   
   //Get size of BLT read using board data and options
   u_int32_t data;
   ReadReg32(V1724_BoardInfoReg,data);
   u_int32_t memorySize = (u_int32_t)((data>>8)&0xFF);
   ReadReg32(V1724_BlockOrganizationReg,data);
   u_int32_t eventSize = (u_int32_t)((((memorySize*pow(2,20))/
					  (u_int32_t)pow(2,data))*8+16)/4);
   ReadReg32(V1724_BltEvNumReg,data);
   fBLTSize=options->GetRunOptions().BLTBytes;
   fBufferSize = data*eventSize*(u_int32_t)4+(fBLTSize);
   fBufferSize = eventSize*data + fBLTSize;
   fReadoutThresh = options->GetReadoutThreshold();
   
   fBuffers = new vector<u_int32_t*>();
   fSizes   = new vector<u_int32_t>();

   if(options->GetBaselineMode()==1)    {	
      int tries = 0;
      while(DetermineBaselines()!=0 && tries<5)
	tries++;
   }
   else cout<<"Didn't determine baselines."<<endl;
   
   cout<<"Loading baselines"<<endl;
   cout<<LoadBaselines()<<endl;
   cout<<"Done with baselines."<<endl;
   return retVal;
}

unsigned int CBV1724::ReadMBLT()
{
   unsigned int blt_bytes=0;
   int nb=0,ret=-5;   
  
   u_int32_t *buff = new u_int32_t[fBufferSize];       //should not survive this function
   do{
      ret = CAENVME_FIFOBLTReadCycle(fCrateHandle,fBID.VMEAddress,
				     ((unsigned char*)buff)+blt_bytes,
				     fBLTSize,cvA32_U_BLT,cvD32,&nb);
      if((ret!=cvSuccess) && (ret!=cvBusError)){
	 stringstream ss;
	 ss<<"Board "<<fBID.BoardID<<" reports read error "<<dec<<ret<<endl;
	 gLog->Error(ss.str());
	 delete[] buff;
	 return 0;
      }
      blt_bytes+=nb;
      if(blt_bytes>fBufferSize)	{
	 stringstream ss;
	 ss<<"Board "<<fBID.BoardID<<" reports insufficient BLT buffer size."<<endl;	 
	 gLog->Error(ss.str());
	 delete[] buff;
	 return 0;
      }
   }while(ret!=cvBusError);
   
   if(blt_bytes>0){
      u_int32_t *writeBuff = new u_int32_t[blt_bytes/(sizeof(u_int32_t))]; //must be freed after writing
      memcpy(writeBuff,buff,blt_bytes);
      //copy(buff,buff+(blt_bytes/(sizeof(u_int32_t))),writeBuff);
      LockDataBuffer();
      fBuffers->push_back(writeBuff);
      fSizes->push_back(blt_bytes);
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

vector<u_int32_t*>* CBV1724::ReadoutBuffer(vector<u_int32_t> *&sizes)
//Note this PASSES OWNERSHIP of the returned vectors to the 
//calling function!
{
   vector<u_int32_t*> *retVec = fBuffers;
   fBuffers = new vector<u_int32_t*>();
   sizes = fSizes;
   fSizes = new vector<u_int32_t>();
   return retVec;
}

int CBV1724::LoadBaselines()
{
   vector <int> baselines;
   if(GetBaselines(baselines)!=0) {
      gLog->SendMessage("Error loading baselines.");
      return -1;
   }   
   if(baselines.size()==0)
     return -1;
   for(unsigned int x=0;x<baselines.size();x++)  {
      if(WriteReg32((0x1098)+(0x100*x),baselines[x])!=0){	   
	 gLog->SendMessage("Error loading baselines");
	 return -1;
      } 
      stringstream sstr;
      sstr<<"Loaded baseline "<<hex<<baselines[x]<<dec<<" to channel "<<x;
      gLog->SendMessage(sstr.str());
      cout<<sstr.str()<<endl;
   }
   return 0;
}

int CBV1724::GetBaselines(vector <int> &baselines, bool bQuiet)
{
   stringstream filename; 
   filename<<"baselines/XeBaselines_"<<fBID.BoardID<<".ini";
   ifstream infile;
   infile.open(filename.str().c_str());
   if(!infile)  {
      stringstream error;
      error<<"No options found for board "<<fBID.BoardID;
      gLog->Error(error.str());
      gLog->SendMessage(error.str());
      return -1;
   }
   baselines.clear();
   //Get date and determine how old baselines are. If older than 24 hours give a warning.
   string line;
   getline(infile,line);
   unsigned int filetime = XeDAQHelper::StringToInt(line);
   //filetime of form YYMMDDHH
   if(XeDAQHelper::CurrentTimeInt()-filetime > 100 && !bQuiet)  {
      stringstream warning;   	   
      warning<<"Warning: Module "<<fBID.BoardID<<" is using baselines that are more than a day old.";
      gLog->SendMessage(warning.str());
   }
   while(getline(infile,line)){
      int value=0;
      if(XeDAQOptions::ProcessLineHex(line,XeDAQHelper::IntToString(baselines.size()+1),value)!=0)
	break;
      baselines.push_back(value);
   }
   if(baselines.size()!=8)   {
      stringstream error;
      error<<"Warning from module "<<fBID.BoardID<<". Error loading baselines.";
      gLog->SendMessage(error.str());
      gLog->Error(error.str());
      infile.close();
      return -1;
   }
   infile.close();
   return 0;      
}

int CBV1724::DetermineBaselines()
//Port of Marc S. routine to C++
//Automatically determines baselines. 
{
   vector <int> oldBaselines;
   vector <u_int32_t> newBaselines(8,0);
   if(GetBaselines(oldBaselines,true)!=0) {
      //there are no baselines. start wit some defaults
      for(unsigned int x=0;x<8;x++)
	oldBaselines.push_back(0x1300); //seems like a good middle value
   }   
   if(oldBaselines.size()!=8) return -1;
 
   //load the old baselines
   for(unsigned int x=0;x<oldBaselines.size();x++)    {	
      if(WriteReg32((0x1098)+(0x100*x),oldBaselines[x])!=0){	     
	 gLog->SendMessage("Error loading baselines");
	 return -1;
      }
   }            
   
   u_int32_t blt_bytes;
   u_int32_t *buff;
   int maxIterations=35;
   int samples = 8192;
   u_int32_t newDAC=0.;
   double MaxDev = 2.0;
   double idealBaseline = 16000.;
   int retval=0;
   
   vector<double> channelMeans(8,0.);
   buff=new u_int32_t[fBufferSize];
   vector<bool> channelFinished(8,false);
   
   u_int32_t ACR,STR,ZLE;
   ReadReg32(AcquisitionControlReg,ACR);
   ReadReg32(SoftwareTriggerReg,STR);
   ReadReg32(ZLEReg,ZLE);
   
   int iteration=0;
   while(iteration<=maxIterations)  {
      iteration++;
      
      u_int32_t data; 
      //Enable board
//      ReadReg32(AcquisitionControlReg,data);
      usleep(50000);
      data=0x4;
      WriteReg32(AcquisitionControlReg,data);
      usleep(1000);
      //Set software trigger
      data=0x1;
      WriteReg32(SoftwareTriggerReg,data);
      
      //Disable board
//      ReadReg32(AcquisitionControlReg,data);
  //    data&=0xFFFFFFFB;
      data=0x0;
      WriteReg32(AcquisitionControlReg,data);

      data=0xD0; //turn ZLE off, simple data format
      WriteReg32(ZLEReg,data);
      
      //Read MBLT
      int ret,nb,pnt=0;
      blt_bytes=0;
      do {
	 ret=CAENVME_FIFOBLTReadCycle(fCrateHandle,fBID.VMEAddress,
				      ((unsigned char*)buff)+blt_bytes,
				      fBLTSize,cvA32_U_BLT,cvD32,&nb);
	 if(ret!=cvSuccess && ret!=cvBusError)  {
	    cout<<"CAENVME Read Error"<<endl;
	    continue;
	 }
	 blt_bytes+=nb;
	 if(blt_bytes>fBufferSize) {
	    continue;
	 }	 
      }while(ret!=cvBusError);

      if(blt_bytes==0)	{
	 gLog->Message("Baseline calibration: read nothing from board.");
	 continue;
      }
      
      //Process what you just read
      int minval=0,maxval=0,mean=0;
      if((buff[0]>>20)!=0xA00)	{
	 gLog->Message("Baseline calibration: unexpected buffer format");      
	 continue;
      }
      samples = (buff[0]&0xFFFFFF);
      cout<<"BUFFER SIZE "<<samples<<endl;
      samples-=4; //subtract header
      samples/=8; //get lines per channel
      samples*=2; //2 samples per line
      pnt=4;
      int val;
      for(int channel=0;channel<8;channel++)	{
	 minval=16384; maxval=0; mean=0;
	 for(int j=0;j<(int)(samples/2);j++)  {
	    for(int k=0;k<2;k++)  {
	       if(k==0) val=buff[pnt+j]&0xFFFF;
	       else val=(buff[pnt+j]>>16)&0xFFFF;
	       if(val>maxval) maxval=val;
	       if(val<minval) minval=val;
	       mean+=(double)val;
	    }	    
	 }
	 pnt=pnt+(int)(samples/2);
	 if(channelFinished[channel]) continue;
	 
	 cout<<"Channel: "<<dec<<channel<<endl;
	 cout<<"Tot "<<dec<<mean<<" nsamples "<<samples;
	 mean=mean/(double)samples;
	 cout<<" mean "<<dec<<mean<<" pnt "<<pnt<<endl;
	 	 	 
	 //Read DAC register. 
	 ReadReg32(V1724_DACReg+(channel*0x100),data);
	 newDAC=data;
	 double diff=0;
	 if((maxval-mean)<500 && (mean-minval)<500)  { //baseline is flat 
	    diff=double(idealBaseline-mean);
	    cout<<"Diff "<<diff<<" DAQ "<<hex<<newDAC<<dec<<endl;
	    if(diff<=MaxDev && diff>=MaxDev*(-1.))  {   //Existing baseline is still OK
	       channelFinished[channel]=true;
	       newBaselines[channel]=data;
	       cout<<"Baseline of "<<hex<<data<<dec<<
		 " retained for channel "<<channel<<" on board "<<fBID.BoardID<<endl;	       
	       continue;
	    }
	    else  {       //Baseline must be adjusted
	       if(channel==7) cout<<"diff "<<diff<<" maxdev "<<MaxDev<<" mean "<<mean<<" oldDAC "<<hex<<data<<endl;
	       if(diff>MaxDev)	 { if(channel==7) cout<<"DIFFGTMD"<<endl;
		  if(diff>8){
		     if(diff>50) { if(channel==7) cout<<"GT50 "<<newDAC<<" "<<diff/-0.265<<endl;
			newDAC=(data+(int)(diff/(-.264))); }//coarse adjust
		     else newDAC=data-30;                     //a bit less coarse
		  }
		  else newDAC=data-15;                        //fine adjust		  
	       }//end if diff>MaxDev
	       else  if(diff<MaxDev*(-1.)){ 
		  if(diff<-8)  {
		     if(diff<-50) newDAC=(data+(int)(diff/(-.264))); //coarse adjust
		     else newDAC=data+30;                      //a bit finer
		  }		  
		  else newDAC=data+15;                         //fine adjust
	       }	       	       
	       else newDAC=data;                               //no adjust
	    }	    
	    if(iteration==maxIterations){
	       cout<<" To do "<<channel<<endl;
	       channelFinished[channel]=true;
	       newBaselines[channel]=newDAC;//data;
	       retval=1; //flag that at least one channel didn't finish
	    }
	    cout<<"New DAC "<<hex<<newDAC<<endl;	    
	    WriteReg32(V1724_DACReg+(0x100*channel),(newDAC&0xFFFF)); //write updated DAC
	 }//end if baseline is flat	 
	 //else
	 //  gLog->SendMessage("Problem during baseline determination. Is it not flat?");
	 
      }//end for through channels            
   }//end while

   //write baselines to file
   ofstream outfile; 
   stringstream filename;
   filename<<"baselines/XeBaselines_"<<fBID.BoardID<<".ini";
   outfile.open(filename.str().c_str());
   outfile<<XeDAQHelper::CurrentTimeInt()<<endl;
   for(unsigned int x=0;x<newBaselines.size();x++)  {
      outfile<<x+1<<"  "<<hex<<setw(4)<<setfill('0')<<((newBaselines[x])&0xFFFF)<<endl;
   }
   outfile.close();

   WriteReg32(AcquisitionControlReg,ACR);
   WriteReg32(SoftwareTriggerReg,STR);
   WriteReg32(ZLEReg,ZLE);
   
   delete buff;
   return retval;
}

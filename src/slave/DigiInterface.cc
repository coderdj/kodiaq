
// ********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : DigiInterface.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 12.07.2013
// 
// Brief    : Class for managing electronics
// 
// 
// *********************************************************
#include <iostream>
#include "DigiInterface.hh"

DigiInterface::DigiInterface()
{
   fRunStartModule=0;
   fDAQRecorder=NULL;
   fWriteMode=0;
   fReadThread.IsOpen=false;
}

DigiInterface::~DigiInterface()
{     
   Close();
}

int DigiInterface::Initialize(XeDAQOptions *options)
{
   Close();
  
   //First define crates   
   for(int x=0;x<options->GetLinks();x++)  {
      VMECrate *crate = new VMECrate();
      if(crate->Define(options->GetLink(x))==0)
	fCrates.push_back(crate);
      else{	         
	 delete crate;
	 gLog->Error("DigiInterface::InitializeElectronics - Error in crate definitions.");
	 return -1;
      }      
   } 
   
   //Set up threads
   pthread_mutex_init(&fRateLock,NULL);
   fReadSize=0;
   fReadFreq=0;
   if(options->GetProcessingThreads()>0)
     fProcessingThreads.resize(options->GetProcessingThreads());
   else fProcessingThreads.resize(1);
   
   for(unsigned int x=0;x<fProcessingThreads.size();x++)  {
      fProcessingThreads[x].IsOpen=false;
      fProcessingThreads[x].XeP=NULL;
   }
   fReadThread.IsOpen=false;
   
   //Now define modules
   for(int x=0;x<options->GetBoards();x++)  {
      for(unsigned int y=0;y<fCrates.size();y++)	{
	 if(fCrates[y]->Info().CrateID!=options->GetBoard(x).CrateID ||
	    fCrates[y]->Info().LinkID!=options->GetBoard(x).LinkID)
	   continue;
	 fCrates[y]->AddModule(options->GetBoard(x));
      }      
   }   
   
   //Define how run is started
   if(options->GetRunOptions().RunStart==1)
     fRunStartModule=GetModuleByID(options->GetRunOptions().RunStartModule);
   else 
     fRunStartModule=0;
   

   
   //Load options to individual modules
   for(unsigned int x=0;x<fCrates.size();x++){	
      if(fCrates[x]->InitializeModules(options)!=0)
	return -1;
   }

   //Designate sum modules if any
   for(int x=0;x<options->SumModules();x++)    {	
      VMEBoard *module;
      if((module=GetModuleByID(options->GetSumModule(x)))!=NULL)
	module->DesignateSumModule();
   }
   
   //Set up daq recorder
   fDAQRecorder = new XeMongoRecorder();      	
   fDAQRecorder->Initialize(options);
   
   return 0;
}

void DigiInterface::UpdateRecorderCollection(XeDAQOptions *options)
{
   fDAQRecorder->UpdateCollection(options);
}


VMEBoard* DigiInterface::GetModuleByID(int ID)
{
   for(unsigned int x=0;x<fCrates.size();x++)  {
      for(int y=0;y<fCrates[x]->GetModules();y++)	{
	 if(fCrates[x]->GetModule(y)->GetID().BoardID==ID)
	   return fCrates[x]->GetModule(y);
      }      
   }   
   return NULL;
}

void DigiInterface::Close()
{
   pthread_mutex_destroy(&fRateLock);
   
   for(unsigned int x=0;x<fCrates.size();x++){	
      fCrates[x]->Close();
      delete fCrates[x];
   }   
   if(fDAQRecorder!=NULL) delete fDAQRecorder;
   fDAQRecorder=NULL;
   fCrates.clear();
   CloseThreads(true);
     
   return;
}

void* DigiInterface::ReadThreadWrapper(void* data)
{
   DigiInterface *digi = static_cast<DigiInterface*>(data);
   digi->ReadThread();
   return (void*)data;
}

void DigiInterface::ReadThread()
{
   bool ExitCondition=false;
   while(!ExitCondition)  {
      ExitCondition=true;
      unsigned int rate=0,freq=0;
      for(unsigned int x=0; x<fCrates.size();x++)  {
	 if(fCrates[x]->IsActive()) ExitCondition=false; //at least one crate is active
	 unsigned int tf=0;
	 unsigned int ratecycle=fCrates[x]->ReadCycle(tf);	 	 
	 rate+=ratecycle;
	 freq+=tf;
      }  
      LockRateMutex();
      fReadSize+=rate;
      fReadFreq+=freq;
      UnlockRateMutex();
   }
   
   return;
}
 
int DigiInterface::StartRun()
{   
   //Reset timer on DAQ recorder
   fDAQRecorder->ResetTimer();
   
   //Tell Boards to start acquisition
   if(fRunStartModule!=0){
      for(unsigned int x=0;x<fCrates.size();x++)
	fCrates[x]->SetActive();
      fRunStartModule->SendStartSignal();
   }
   else {
      for(unsigned int x=0;x<fCrates.size();x++)
	fCrates[x]->StartRunSW();
   }
   
   //Spawn read, write, and processing threads
   for(unsigned int x=0; x<fProcessingThreads.size();x++)  {
      if(fProcessingThreads[x].IsOpen) {
	 CloseThreads();
	 return -1;
      }
      usleep(100);
      if(fProcessingThreads[x].XeP!=NULL) delete fProcessingThreads[x].XeP;
      fProcessingThreads[x].XeP=new XeProcessor(this,fDAQRecorder);
  //    if(fWriteMode==2)
	pthread_create(&fProcessingThreads[x].Thread,NULL,XeProcessor::WProcessMongoDB,
		       static_cast<void*>(fProcessingThreads[x].XeP));
//      else 
//	pthread_create(&fProcessingThreads[x].Thread,NULL,XeProcessor::WProcessStd,
//		       static_cast<void*>(fProcessingThreads[x].XeP));      
      fProcessingThreads[x].IsOpen=true;
   }
   
   if(fReadThread.IsOpen)  {
      CloseThreads();
      return -1;
   }
   
   //Create read thread even if not writing
   pthread_create(&fReadThread.Thread,NULL,DigiInterface::ReadThreadWrapper,
		  static_cast<void*>(this));
   fReadThread.IsOpen=true;
   
   return 0;
}

int DigiInterface::StopRun()
{
   if(fRunStartModule!=0){      
     fRunStartModule->SendStopSignal();
      for(unsigned int x=0;x<fCrates.size();x++)
	fCrates[x]->SetInactive();
   }   
   else   {
      for(unsigned int x=0;x<fCrates.size();x++)	{
	 fCrates[x]->StopRunSW();
      }      
   }   
   CloseThreads();
   fDAQRecorder->ShutdownRecorder();
   return 0;
}

unsigned int DigiInterface::GetDigis()
{
   int ret=0;
   for(unsigned int x=0;x<fCrates.size();x++)
     ret+=fCrates[x]->GetDigitizers();
   return ret;
}

void DigiInterface::CloseThreads(bool Completely)
{
   if(fReadThread.IsOpen)  {
      fReadThread.IsOpen=false;
      pthread_join(fReadThread.Thread,NULL);      
   }
   for(unsigned int x=0;x<fProcessingThreads.size();x++)  {
      if(fProcessingThreads[x].IsOpen==false) continue;
      fProcessingThreads[x].IsOpen=false;
      pthread_join(fProcessingThreads[x].Thread,NULL);
   }
   
   if(Completely)  {
      for(unsigned int x=0;x<fProcessingThreads.size();x++)	{
	 if(fProcessingThreads[x].XeP!=NULL)  {
	    delete fProcessingThreads[x].XeP;
	    fProcessingThreads[x].XeP=NULL;
	 }
      }
      fProcessingThreads.clear();		 
   }           
   return;
}

bool DigiInterface::LockRateMutex()
{
   int error = pthread_mutex_lock(&fRateLock);
   if(error==0) return true;
   return false;
}

bool DigiInterface::UnlockRateMutex()
{
   int error = pthread_mutex_unlock(&fRateLock);
   if(error==0) return true;
   return false;
}

u_int32_t DigiInterface::GetRate(u_int32_t &freq)
{
   if(!LockRateMutex()) return 0;
   freq=fReadFreq;
   u_int32_t retRate=fReadSize;
   fReadFreq=fReadSize=0;
   UnlockRateMutex();
   return retRate;
}

bool DigiInterface::RunError(string &err)
{
   //check mongo and processors for an error and report it
   string error;
   if(fDAQRecorder!=NULL){	
      if(fDAQRecorder->QueryError(err)) return true;
   }   
   for(unsigned int x=0;x<fProcessingThreads.size();x++)  {
      if(fProcessingThreads[x].XeP==NULL) continue;
      if(fProcessingThreads[x].XeP->QueryError(err)) return true;
   }   
   return false;   
}

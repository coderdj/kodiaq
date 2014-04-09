
// ********************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : DigiInterface.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 12.07.2013
// Update   : 31.03.2014
// 
// Brief    : Class for managing electronics
// 
// 
// *********************************************************
#include <iostream>
#include "DigiInterface.hh"

DigiInterface::DigiInterface()
{
   m_ReadThread.IsOpen=false;
   m_WriteThread.IsOpen=false;
   m_koLog = NULL;
   m_DAQRecorder=NULL;
   Close();
}

DigiInterface::~DigiInterface()
{     
   Close();
}

DigiInterface::DigiInterface(koLogger *logger)
{
   m_ReadThread.IsOpen  = false;
   m_WriteThread.IsOpen = false;
   m_koLog              = logger;
   m_DAQRecorder        = NULL;
   Close();
}

int DigiInterface::Initialize(koOptions *options)
{
   Close();
  
   m_koOptions = options;
   
   //First define crates   
   for(int x=0;x<options->GetLinks();x++)  {
      VMECrate *crate = new VMECrate(m_koLog);
      if(crate->Define(options->GetLink(x))==0)
	m_vCrates.push_back(crate);
      else{	         
	 delete crate;
	 if(m_koLog!=NULL)
	   m_koLog->Error("DigiInterface::InitializeElectronics - Error in crate definitions.");
	 return -1;
      }      
   } 
   
   //Set up threads
   pthread_mutex_init(&m_RateMutex,NULL);
   m_iReadSize=0;
   m_iReadFreq=0;
   if(options->GetProcessingOptions().NumThreads>0)
     m_vProcThreads.resize(options->GetProcessingOptions().NumThreads);
   else m_vProcThreads.resize(1);
   
   for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
      m_vProcThreads[x].IsOpen=false;
      m_vProcThreads[x].Processor=NULL;
   }
         
   //Now define modules
   for(int x=0;x<options->GetBoards();x++)  {
      for(unsigned int y=0;y<m_vCrates.size();y++)	{
	 if(m_vCrates[y]->Info().CrateID!=options->GetBoard(x).CrateID ||
	    m_vCrates[y]->Info().LinkID!=options->GetBoard(x).LinkID)
	   continue;
	 m_vCrates[y]->AddModule(options->GetBoard(x));
      }      
   }   
   
   //Define how run is started
   if(options->GetRunOptions().RunStart==1)
     m_RunStartModule=GetModuleByID(options->GetRunOptions().
				    RunStartModule);
   else 
     m_RunStartModule=NULL;
   

   
   //Load options to individual modules
   for(unsigned int x=0;x<m_vCrates.size();x++){	
      if(m_vCrates[x]->InitializeModules(options)!=0)
	return -1;
   }

   //Designate sum modules if any
   for(int x=0;x<options->SumModules();x++)    {	
      VMEBoard *module;
      if((module=GetModuleByID(options->GetSumModule(x)))!=NULL)
	module->DesignateSumModule();
   }
   
   //Set up daq recorder
   m_DAQRecorder = NULL;
   if(options->GetRunOptions().WriteMode==WRITEMODE_FILE)
     m_DAQRecorder = new DAQRecorder_protobuff(m_koLog);
#ifdef HAVE_LIBMONGOCLIENT
   else if(options->GetRunOptions().WriteMode==WRITEMODE_MONGODB)
     m_DAQRecorder = new DAQRecorder_mongodb(m_koLog);
#endif
   if(m_DAQRecorder!=NULL)
     m_DAQRecorder->Initialize(options);
   
   return 0;
}

void DigiInterface::UpdateRecorderCollection(koOptions *options)
{
#ifdef HAVE_LIBMONGOCLIENT
   DAQRecorder_mongodb *dr = dynamic_cast<DAQRecorder_mongodb*>(m_DAQRecorder);
   dr->UpdateCollection(options);
#endif
}


VMEBoard* DigiInterface::GetModuleByID(int ID)
{
   for(unsigned int x=0;x<m_vCrates.size();x++)  {
      for(int y=0;y<m_vCrates[x]->GetModules();y++)	{
	 if(m_vCrates[x]->GetModule(y)->GetID().BoardID==ID)
	   return m_vCrates[x]->GetModule(y);
      }      
   }   
   return NULL;
}

void DigiInterface::Close()
{
   pthread_mutex_destroy(&m_RateMutex);
   
   for(unsigned int x=0;x<m_vCrates.size();x++){	
      m_vCrates[x]->Close();
      delete m_vCrates[x];
   }   
   m_vCrates.clear();

   CloseThreads(true);

   //Didn't create these objects, so just reset pointers
   m_RunStartModule=NULL;
   m_koOptions=NULL;
   
   //Created the DAQ recorder, so must destroy it
   if(m_DAQRecorder!=NULL) delete m_DAQRecorder;
   m_DAQRecorder=NULL;
   
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
      for(unsigned int x=0; x<m_vCrates.size();x++)  {
	 if(m_vCrates[x]->IsActive()) ExitCondition=false; //at least one crate is active
	 unsigned int tf=0;
	 unsigned int ratecycle=m_vCrates[x]->ReadCycle(tf);	 	 
	 rate+=ratecycle;
	 freq+=tf;
      }  
      LockRateMutex();
      m_iReadSize+=rate;
      m_iReadFreq+=freq;
      UnlockRateMutex();
   }
   
   return;
}
 
int DigiInterface::StartRun()
{   
   //Reset timer on DAQ recorder
   if(m_DAQRecorder!=NULL)
     m_DAQRecorder->ResetTimer();

   
   //Tell Boards to start acquisition
   if(m_RunStartModule!=NULL)    {	
      for(unsigned int x=0;x<m_vCrates.size();x++)
	m_vCrates[x]->SetActive();
      cout<<"SENDING START SIGNAL"<<endl;
      m_RunStartModule->SendStartSignal();
   }   
   else   {	
      for(unsigned int x=0;x<m_vCrates.size();x++)
	m_vCrates[x]->StartRunSW();
   }
   
   
   //Spawn read, write, and processing threads
   for(unsigned int x=0; x<m_vProcThreads.size();x++)  {
      if(m_vProcThreads[x].IsOpen) {
	 CloseThreads();
	 return -1;
      }
      usleep(100);
      if(m_vProcThreads[x].Processor!=NULL) delete m_vProcThreads[x].Processor;
      
      // Spawning of processing threads. depends on readout options.
      if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_NONE) {	   
	 m_vProcThreads[x].Processor = new DataProcessor_dump(this,m_DAQRecorder,
							     m_koOptions);
	 pthread_create(&m_vProcThreads[x].Thread,NULL,DataProcessor_dump::WProcess,
			static_cast<void*>(m_vProcThreads[x].Processor));
	 m_vProcThreads[x].IsOpen = true;
      }      
      else if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_FILE){	   
	 m_vProcThreads[x].Processor = new DataProcessor_protobuff(this,
								   m_DAQRecorder,
								   m_koOptions);
	 pthread_create(&m_vProcThreads[x].Thread,NULL,DataProcessor_protobuff::WProcess,
			static_cast<void*>(m_vProcThreads[x].Processor));
	 m_vProcThreads[x].IsOpen = true;
      }      
      else if(m_koOptions->GetRunOptions().WriteMode == WRITEMODE_MONGODB){
#ifdef HAVE_LIBMONGOCLIENT
	 m_vProcThreads[x].Processor = new DataProcessor_mongodb(this,
								 m_DAQRecorder,
								 m_koOptions);
	 pthread_create(&m_vProcThreads[x].Thread,NULL,DataProcessor_mongodb::WProcess,
			static_cast<void*>(m_vProcThreads[x].Processor));
	 m_vProcThreads[x].IsOpen=true;
#else
	 if(m_koLog!=NULL) 
	   m_koLog->Error("DigiInterface::StartRun - Asked for mongodb output but didn't compile with mongodb support!");
	 StopRun();
	 return -1;
#endif
      }
      else	{
	 if(m_koLog!=NULL) 
	   m_koLog->Error("DigiInterface::StartRun - Undefined write mode.");
	 StopRun();
	 return -1;
      }
   }
   
   if(m_ReadThread.IsOpen)  {
      CloseThreads();
      if(m_koLog!=NULL) 
	m_koLog->Error("DigiInterface::StartRun - Read thread was already open.");
      return -1;
   }
   
   //Create read thread even if not writing
   pthread_create(&m_ReadThread.Thread,NULL,DigiInterface::ReadThreadWrapper,
		  static_cast<void*>(this));
   m_ReadThread.IsOpen=true;
      
   return 0;
}

int DigiInterface::StopRun()
{
   if(m_RunStartModule!=0){      
     m_RunStartModule->SendStopSignal();
      for(unsigned int x=0;x<m_vCrates.size();x++)
	m_vCrates[x]->SetInactive();
   }   
   else   {
      for(unsigned int x=0;x<m_vCrates.size();x++)	{
	 m_vCrates[x]->StopRunSW();
      }      
   }   
   CloseThreads();
   if(m_DAQRecorder != NULL)
     m_DAQRecorder->Shutdown();

   return 0;
}

unsigned int DigiInterface::GetDigis()
{
   int ret=0;
   for(unsigned int x=0;x<m_vCrates.size();x++)
     ret+=m_vCrates[x]->GetDigitizers();
   return ret;
}

void DigiInterface::CloseThreads(bool Completely)
{
   if(m_ReadThread.IsOpen)  {
      m_ReadThread.IsOpen=false;
      pthread_join(m_ReadThread.Thread,NULL);      
   }
   for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
      if(m_vProcThreads[x].IsOpen==false) continue;
      m_vProcThreads[x].IsOpen=false;
      pthread_join(m_vProcThreads[x].Thread,NULL);
   }
   
   if(Completely)  {
      for(unsigned int x=0;x<m_vProcThreads.size();x++)	{
	 if(m_vProcThreads[x].Processor != NULL)  {
	    delete m_vProcThreads[x].Processor;
	    m_vProcThreads[x].Processor = NULL;
	 }
      }
      m_vProcThreads.clear();		 
   }           
   return;
}

bool DigiInterface::LockRateMutex()
{
   int error = pthread_mutex_lock(&m_RateMutex);
   if(error==0) return true;
   return false;
}

bool DigiInterface::UnlockRateMutex()
{
   int error = pthread_mutex_unlock(&m_RateMutex);
   if(error==0) return true;
   return false;
}

u_int32_t DigiInterface::GetRate(u_int32_t &freq)
{
   if(!LockRateMutex()) return 0;
   freq=m_iReadFreq;
   u_int32_t retRate=m_iReadSize;
   m_iReadFreq=m_iReadSize=0;
   UnlockRateMutex();
   return retRate;
}

bool DigiInterface::RunError(string &err)
{
   //check mongo and processors for an error and report it
   string error;
   if(m_DAQRecorder!=NULL){	
      if(m_DAQRecorder->QueryError(err)) return true;
   }   
   for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
      if(m_vProcThreads[x].Processor == NULL) continue;
      if(m_vProcThreads[x].Processor->QueryError(err)) return true;
   }   
   return false;   
}

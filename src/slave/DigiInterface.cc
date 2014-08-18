
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
   m_RunStartModule=NULL;
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
   m_RunStartModule     = NULL;
   Close();
}

int DigiInterface::Initialize(koOptions *options)
{
   Close();
  
   m_koOptions = options;
   
   //Define electronics and initialize
   for(int ilink=0;ilink<options->GetLinks();ilink++)  {
      int tempHandle=-1;
      link_definition_t Link = options->GetLink(ilink);
      CVBoardTypes BType;
      if(Link.type=="V1718")
	BType = cvV1718;
      else if(Link.type=="V2718")
	BType = cvV2718;
      else  	{	   
	 if(m_koLog!=NULL)
	   m_koLog->Error("VMECrate::Define - Invalid link type, check file definition.");
	 return -1;
      }
      
      int cerr=-1;
      if((cerr=CAENVME_Init(BType,Link.id,Link.crate,
				&tempHandle))!=cvSuccess){
	 //throw exception
	 return -1;
      }      
      m_vCrateHandles.push_back(tempHandle);
      
      //define modules corresponding to this crate (inefficient
      //double for loops, but small crate/module vector size)
      for(int imodule=0; imodule<options->GetBoards(); imodule++)	{
	 board_definition_t Board = options->GetBoard(imodule);
	 if(Board.link!=Link.id || Board.crate!=Link.crate)
	   continue;
	 cout<<"Found a board with link "<<Board.link<<" and crate "<<Board.crate<<endl;
	 if(Board.type=="V1724"){	      
	    CBV1724 *digitizer = new CBV1724(Board,m_koLog);
	    m_vDigitizers.push_back(digitizer);
	    digitizer->SetCrateHandle(tempHandle);
	 }	 
	 else if(Board.type=="V2718"){	      
	    CBV2718 *digitizer = new CBV2718(Board, m_koLog);
	    m_RunStartModule=digitizer;
	    digitizer->SetCrateHandle(tempHandle);
	    if(digitizer->Initialize(options)!=0)
	      return -1;
	 }	 
	 else   {
	    if(m_koLog!=NULL)
	      m_koLog->Error("Undefined board type in .ini file.");
	    continue;
	 }
	 
      }
   }
   StopRun();
   // initialize digitizers
   for(unsigned int x=0; x<m_vDigitizers.size();x++)  {
      if(m_vDigitizers[x]->Initialize(options)!=0)
	return -1;
   }
   
   
   
   //Set up threads
   pthread_mutex_init(&m_RateMutex,NULL);
   m_iReadSize=0;
   m_iReadFreq=0;
   if(options->processing_num_threads>0)
     m_vProcThreads.resize(options->processing_num_threads);
   else m_vProcThreads.resize(1);
   
   for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
      m_vProcThreads[x].IsOpen=false;
      m_vProcThreads[x].Processor=NULL;
   }
            
   //Set up daq recorder
   m_DAQRecorder = NULL;

   if(options->write_mode==WRITEMODE_FILE){
#ifdef HAVE_LIBPBF
     m_DAQRecorder = new DAQRecorder_protobuff(m_koLog);
#else
      //throw exception
      m_koOptions->write_mode = WRITEMODE_NONE;
#endif
   }   
#ifdef HAVE_LIBMONGOCLIENT
   else if(options->write_mode==WRITEMODE_MONGODB)
     m_DAQRecorder = new DAQRecorder_mongodb(m_koLog);
#else
   //throw exception
   m_koOptions->write_mode = WRITEMODE_NONE;
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


void DigiInterface::Close()
{
   StopRun();
   
   pthread_mutex_destroy(&m_RateMutex);
   
   //deactivate digis
   for(unsigned int x=0;x<m_vDigitizers.size();x++)
     m_vDigitizers[x]->SetActivated(false);
   
   CloseThreads(true);
   
   for(unsigned int x=0;x<m_vCrateHandles.size();x++){
      if(CAENVME_End(m_vCrateHandles[x])!=cvSuccess)	{
	 //throw error
      }      
   }   
   m_vCrateHandles.clear();
   
   for(unsigned int x=0;x<m_vDigitizers.size();x++)  
      delete m_vDigitizers[x];
   m_vDigitizers.clear();
   if(m_RunStartModule!=NULL)  {
      delete m_RunStartModule;
      m_RunStartModule=NULL;
   }      
   
   //Didn't create these objects, so just reset pointers
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
      for(unsigned int x=0; x<m_vDigitizers.size();x++)  {
	 if(m_vDigitizers[x]->Activated()) ExitCondition=false; //at least one crate is active
	 unsigned int ratecycle=m_vDigitizers[x]->ReadMBLT();	 	 
	 rate+=ratecycle;
	 if(ratecycle!=0)
	   freq++;
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
   //Tell Boards to start acquisition
   if(m_RunStartModule!=NULL)    {	
      for(unsigned int x=0;x<m_vDigitizers.size();x++)
	m_vDigitizers[x]->SetActivated(true);
      m_RunStartModule->SendStartSignal();
   }   
   else   {	//start by write to software register
      for(unsigned int x=0;x<m_vDigitizers.size();x++){
	 u_int32_t data;
	 m_vDigitizers[x]->ReadReg32(CBV1724_AcquisitionControlReg,data);
	 data |= 0x4;
	 m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,data);
	 m_vDigitizers[x]->SetActivated(true);
      }
   }
   
   //Spawn read, write, and processing threads
   for(unsigned int x=0; x<m_vProcThreads.size();x++)  {
     if(m_vProcThreads[x].IsOpen) {
       StopRun();
       return -1;
     }
     if(m_vProcThreads[x].Processor!=NULL) delete m_vProcThreads[x].Processor;
     
     // Spawning of processing threads. depends on readout options.
     m_vProcThreads[x].Processor = new DataProcessor(this,m_DAQRecorder,
						     m_koOptions);
     pthread_create(&m_vProcThreads[x].Thread,NULL,DataProcessor::WProcess,
		    static_cast<void*>(m_vProcThreads[x].Processor));
     m_vProcThreads[x].IsOpen=true;
   }
   
   if(m_ReadThread.IsOpen)  {
      StopRun();
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
   if(m_RunStartModule!=NULL){      
     m_RunStartModule->SendStopSignal();
      for(unsigned int x=0;x<m_vDigitizers.size();x++)
	m_vDigitizers[x]->SetActivated(false);
   }   
   else   {
      for(unsigned int x=0;x<m_vDigitizers.size();x++)	{
	 u_int32_t data;
	 m_vDigitizers[x]->ReadReg32(CBV1724_AcquisitionControlReg,data);
	 data &= 0xFFFFFFFB;
	 m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,data);
	 m_vDigitizers[x]->SetActivated(false);
      }      
   }   
   CloseThreads();
   if(m_DAQRecorder != NULL)
     m_DAQRecorder->Shutdown();

   return 0;
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

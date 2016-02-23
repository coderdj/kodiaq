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
   m_DB_USER=m_DB_PASSWORD="";
   pthread_mutex_init(&m_RateMutex,NULL);
   m_koOptions = NULL;
}

DigiInterface::~DigiInterface()
{     
  pthread_mutex_destroy(&m_RateMutex);
  Close();
}

DigiInterface::DigiInterface(koLogger *logger, int ID, 
			     string DB_USER, string DB_PASSWORD)
{
   m_ReadThread.IsOpen  = false;
   m_WriteThread.IsOpen = false;
   m_koLog              = logger;
   m_DAQRecorder        = NULL;
   m_RunStartModule     = NULL;
   m_slaveID            = ID;
   m_DB_USER            = DB_USER;
   m_DB_PASSWORD        = DB_PASSWORD;
   pthread_mutex_init(&m_RateMutex,NULL);
   m_koOptions = NULL;
}

int DigiInterface::Arm(koOptions *options){
  //
  // Arm Function
  //
  // Beginning State: DAQ is idle. No threads open. No electronics declared.
  //
  // End State:    Use options file to declare all electronics modules
  //               Open read, write, and processing threads
  //               Declare a proper DAQRecorder and attach to processors
  //               Put boards into 'ready' state (in case of start with Sin)
  
  
  cout<<"Arming!"<<endl;

  m_koOptions = options;

  // Ensure mutiple 'Arm' commands in sequence don't declare too many 
  // objects by closing first.
  cout<<"Clearing"<<endl;
  Close();
  cout<<"Finished clear"<<endl;

  // Initialize boards. This reads options, builds electronics objects, 
  // and runs CAEN initialization procedure.
  if(InitializeHardware(options) != 0){
    Close();
    return -1;
  }
  cout<<"Hardware initialized"<<endl;
  
  // Have to activate boards before spawning processing threads
  // otherwise threads will immediately exit.
  for(unsigned int x=0;x<m_vDigitizers.size();x++)
    m_vDigitizers[x]->SetActivated(true);
  cout<<"Digitizers activated"<<endl;
  
  // If we are starting with S-IN we arm the acquisition control
  // with 0b101 = activated, start on NIM high on S-IN.
  if(options->GetInt("run_start") == 1){                                       
    for(unsigned int x=0;x<m_vDigitizers.size();x++){
      u_int32_t data;
      data = 0x5; 
      m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,data); 
    }
  }
  cout<<"Run started"<<endl;

  //Set up processing threads containers
  m_iReadSize=0;
  m_iReadFreq=0;
  if(options->GetInt("processing_num_threads")>0)
    m_vProcThreads.resize(options->GetInt("processing_num_threads"));
  else
    m_vProcThreads.resize(1);
  for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
    m_vProcThreads[x].IsOpen=false;
    m_vProcThreads[x].Processor=NULL;
  }


  // Set up daq recorder. Use preprocessor logic to determine if a
  // user compiled with the options he is requesting in his .ini file.
  cout<<"Setting up recorder with write mode "<<
    options->GetInt("write_mode")<<endl;
  m_DAQRecorder = NULL;
  if(options->GetInt("write_mode")==WRITEMODE_FILE){
#ifdef HAVE_LIBPBF
    m_DAQRecorder = new DAQRecorder_protobuff(m_koLog);
#else
    if( m_koLog != NULL )
      m_koLog->Error("DigitInterface::Initialize - Your chosen write mode is not available in this installation");
    options->SetInt("write_mode", WRITEMODE_NONE);
#endif
  }
  else if ( options->GetInt("write_mode") == WRITEMODE_MONGODB ){
#ifdef HAVE_LIBMONGOCLIENT
    m_DAQRecorder = new DAQRecorder_mongodb(m_koLog, m_DB_USER, m_DB_PASSWORD);
#else
    if( m_koLog != NULL )
      m_koLog->Error("DigitInterface::Initialize - Your chosen write mode is not available in this installation");
    options->SetInt("write_mode", WRITEMODE_NONE);
#endif
  }
  else
    options->SetInt("write_mode", WRITEMODE_NONE);
  
  // Initialize recorder
  if(m_DAQRecorder!=NULL){
    int tret =  m_DAQRecorder->Initialize(options);
    if( tret !=0 ){
      if(m_koLog!=NULL)
        m_koLog->Error("DigiInterface::Initialize - Couldn't initialize DAQ recorder");
      Close();
      return -1;
    }
  }

  // Spawn the actual threads
  cout<<"Spawning threads"<<endl;
  for(unsigned int x=0; x<m_vProcThreads.size();x++)  {

    // All threads should be closed. Otherwise close them and fail
    if(m_vProcThreads[x].IsOpen) {
      Close();
      return -1;
    }

    // Get rid of the processor if it still exists
    if(m_vProcThreads[x].Processor!=NULL) delete m_vProcThreads[x].Processor;

    // Spawning of processing threads. depends on readout options.
    m_vProcThreads[x].Processor = new DataProcessor(this,m_DAQRecorder,
                                                    m_koOptions, x);
    pthread_create(&m_vProcThreads[x].Thread,NULL,DataProcessor::WProcess,
                   static_cast<void*>(m_vProcThreads[x].Processor));
    m_vProcThreads[x].IsOpen=true;
  }

  // If the read thread is already open there must have been a problem 
  // Closing the last run. So fail send the stop command. 
  if(m_ReadThread.IsOpen)  {
    if(m_koLog!=NULL)
      m_koLog->Error("DigiInterface::StartRun - Read thread was already open.");
    Close();
    return -1;
  }

  //Create read thread and indicate that it's open 
  cout<<"Making read thread"<<endl;
  pthread_create(&m_ReadThread.Thread,NULL,DigiInterface::ReadThreadWrapper,
                 static_cast<void*>(this));
  m_ReadThread.IsOpen=true;

  // Reset the clocks!
  for(unsigned int x=0;x<m_vDigitizers.size();x++){
    m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,0x5);
    m_vDigitizers[x]->WriteReg32(0xEF28, 0x1);
  }

  
  cout<<"DONE WITH ARM PROCEDURE"<<endl;
  return 0;
}

int DigiInterface::InitializeHardware(koOptions *options)
{  
  // This function defines all electronics and calls their 
  // initialization procedures.
  m_koOptions = options;
  
  //Define electronic links from options object
  for(int ilink=0;ilink<options->GetLinks();ilink++)  {
    
    int tempHandle=-1;
    link_definition_t Link = options->GetLink(ilink);
    if(Link.node != m_slaveID && m_slaveID!=-1){
      continue;
    }
    CVBoardTypes BType;
    if(Link.type=="V1718")
      BType = cvV1718;
    else if(Link.type=="V2718")
      BType = cvV2718;
    else  	{	   
      if(m_koLog!=NULL)
	m_koLog->Error(("DigiInterface::Initialize - Invalid link type, " + Link.type + "check file definition."));
      return -1;
    }
    
    int cerror=-1;
    cout<<"Running CAENVME_Init for "<<Link.id<<"."<<Link.crate<<endl;
    if((cerror=CAENVME_Init(BType,Link.id,Link.crate,
			    &tempHandle))!=cvSuccess){
      stringstream therror;
      therror<<"DigiInterface::Initialize - Error in CAEN initialization link "
	     <<Link.id<<" crate "<<Link.crate<<": "<<cerror;
      if(m_koLog!=NULL)
	m_koLog->Error(therror.str());      
      return -1;
    }
 
    // LOG FW
    /*char *fw = (char*)malloc (100);
      CAENVME_BoardFWRelease( tempHandle, fw );
      stringstream logm;
      logm<<"Found V2718 with firmware "<<hex<<fw<<dec;
      m_koLog->Message( logm.str() );
      free(fw);
      //FOR DAQ TEST ONLY
      CAENVME_SystemReset( tempHandle );
    */
    //sleep(1);
    //CAENVME_WriteRegister( tempHandle, cvVMEControlReg, 0x1c);
    
    // Success. Put into log.
    stringstream logmess;
    logmess<<"Initialized link ID: "<<Link.id<<" Crate: "<<
      Link.crate<<" with handle: "<<tempHandle;
    m_koLog->Message(logmess.str());
      
    m_vCrateHandles.push_back(tempHandle);
    
    // define modules corresponding to this crate 
    for(int imodule=0; imodule<options->GetBoards(); imodule++)	{
      
      board_definition_t Board = options->GetBoard(imodule);
      
      // Make sure this board corresponds to this PC and this link
      if(Board.node != m_slaveID && m_slaveID != -1)
	continue;
      if(Board.link!=Link.id || Board.crate!=Link.crate)
	continue;

      // Success. Log it.
      logmess.str(std::string());
      logmess<<"Found a board with link "<<Board.link<<
	" and crate "<<Board.crate;
      m_koLog->Message(logmess.str());
      
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
      else if(Board.type=="V1495"){
         CBV1495 *gpBoard = new CBV1495(Board, m_koLog);
         m_vGeneralPurposeBoards.push_back(gpBoard);
         gpBoard->SetCrateHandle(tempHandle);
         gpBoard->SetActivated(true);
         if(gpBoard->Initialize(options)!=0)
           return -1;
      }	 
      else   {
	if(m_koLog!=NULL)
	  m_koLog->Error("Undefined board type in .ini file.");
	continue;
      }
    }
  }
  cout<<"Links done"<<endl;

  // Initialize digitizers. This sets up their internal data structures
  // and loads all registers to the boards. 
  for(unsigned int x=0; x<m_vDigitizers.size();x++)  {
    cout<<"Initializing digitizer "<<m_vDigitizers[x]->GetID().id<<endl;
    if(m_vDigitizers[x]->Initialize(options)!=0){
      stringstream err;
      err<<"Digtizer "<<m_vDigitizers[x]->GetID().id<<
	" failed initialization!";
      m_koLog->Error( err.str() );
      return -1;
    }
    else 
      m_koLog->Message("Successfully initialized digitizer " + 
		       koHelper::IntToString(m_vDigitizers[x]->GetID().id) +
		       ".");
  }
  cout<<"Init done"<<endl;
  return 0;
}


int DigiInterface::GetBufferOccupancy( vector<int> &digis, vector<int> &sizes)
{
  digis.resize(0);
  sizes.resize(0);
  int totalSize = 0;
  for(unsigned int x=0; x<m_vDigitizers.size(); x++){
    digis.push_back(m_vDigitizers[x]->GetID().id);
    int bSize = m_vDigitizers[x]->GetBufferSize();
    sizes.push_back(bSize);
    totalSize += bSize;
  }
  return totalSize;
}

void DigiInterface::Close()
{
   StopRun();
      
   // Close crates
   for(unsigned int x=0;x<m_vCrateHandles.size();x++){
      if(CAENVME_End(m_vCrateHandles[x])!=cvSuccess)	{
	cout<<"Failed to end crate "<<x<<endl;
	 //throw error
      }            
      else 
	m_koLog->Message("Ended crate " + 
			 koHelper::IntToString(m_vCrateHandles[x]));
   }   
   m_vCrateHandles.clear();
   
   // Clear digitizers
   for(unsigned int x=0;x<m_vDigitizers.size();x++)  
      delete m_vDigitizers[x];
   m_vDigitizers.clear();
   if(m_RunStartModule!=NULL)  {
      delete m_RunStartModule;
      m_RunStartModule=NULL;
   }   

   // clear the general purpose boards
   for(unsigned int x=0;x<m_vGeneralPurposeBoards.size();x++)  
      delete m_vGeneralPurposeBoards[x];
   m_vGeneralPurposeBoards.clear(); 
      
   //Created the DAQ recorder, so must destroy it
   if(m_DAQRecorder!=NULL) delete m_DAQRecorder;
   m_DAQRecorder = NULL;
   //m_koOptions   = NULL;
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
  bool  RanOnce = false;
  bool ExitCondition=false;
  int counter = 0;

  // Use this to make sure you read each digitizer once 
  // even once the run finishes. This way the object is not clears
  // while there's still data in the board's buffer.
  vector<bool> EndOfRunClear( m_vDigitizers.size(), false );

   while(!ExitCondition)  {
      ExitCondition=true;
      unsigned int rate=0,freq=0;
      for(unsigned int x=0; x<m_vDigitizers.size();x++)  {

	// avoid hammering the vme bus. Debatable if this is needed but
	// it doesn't hurt.
	// usleep(100);

	// First check if the digitizer is active. If at least
	// one digitizer is active then keep the thread alive
	if(m_vDigitizers[x]->Activated()) {
	   ExitCondition=false; 
	   RanOnce = true;
	   EndOfRunClear[x] = false;
	}
	else {
	  // Read once to clear any last data in buffer
	  if( EndOfRunClear[x] )
	    continue;
	  EndOfRunClear[x] = true;
	}

	// Read from the digitizer and adjust the rates
	 unsigned int ratecycle=m_vDigitizers[x]->ReadMBLT();	 	 

	 rate+=ratecycle;
	 if(ratecycle!=0)
	   freq++;
      }  

      LockRateMutex();
      m_iReadSize+=rate;      
      m_iReadFreq+=freq;
      UnlockRateMutex();

      // End of run logic. If no digitizers seem to be activated
      // Try a few times (100) to see if any are active. If not end run.
      if( !RanOnce ){
	usleep(1000);
	counter++;
	if( counter < 100 )
	  ExitCondition = false;
      }
      else
	counter=0;
   }
   
   
   return;
}
 
int DigiInterface::StartRun()
{ 

  // Tell Boards to start acquisition   
  // First case for S-IN start
  if(m_koOptions->GetInt("run_start") == 1){
        
    // Send s-in    
    if( m_RunStartModule != NULL ){
      cout<<"Sending S-IN!"<<endl;
      m_koLog->Message( "Sent start signal to run start module ");      
      m_RunStartModule->SendStartSignal();
    }
    else
      m_koLog->Message("No run start module found. Maybe it's on a different reader" );
  }
  else   {
    //start by write to software register   
    m_koLog->Message("Starting boards with SW register");
    // Write to registers and activate  
    for(unsigned int x=0;x<m_vDigitizers.size();x++){
      u_int32_t data;
      m_vDigitizers[x]->ReadReg32(CBV1724_AcquisitionControlReg,data);
      data = 0x4;
      m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,data);
      m_vDigitizers[x]->SetActivated(true);
    }
  }
  
  return 0;
}

 int DigiInterface::StopRun()
 {
     
   cout<<"Entering stoprun"<<endl;
   if(m_RunStartModule!=NULL)  
     m_RunStartModule->SendStopSignal();

   if(m_koOptions!=NULL || !m_koOptions->Loaded())
     return 0;

   if(m_koOptions->GetInt("run_start") == 1){
     for(unsigned int x=0;x<m_vDigitizers.size();x++)   
       m_vDigitizers[x]->SetActivated(false);
   }
   else{
     usleep(1000);
   
     for(unsigned int x=0;x<m_vDigitizers.size();x++)	{
       u_int32_t data;
       m_vDigitizers[x]->ReadReg32(CBV1724_AcquisitionControlReg,data);
       cout<<"READ REG AT STOP "<<hex<<data<<endl;
       data &= 0xFFFFFFFB;
       m_vDigitizers[x]->WriteReg32(CBV1724_AcquisitionControlReg,data);
       cout<<"WRITE REG AT STOP "<<hex<<data<<endl;
       m_vDigitizers[x]->SetActivated(false);
     }      
   }
      
   cout<<"Deactivated digitizers. Closing threads."<<endl;
   CloseThreads();
   cout<<"Shutting down recorder."<<endl;
   if(m_DAQRecorder != NULL)
     m_DAQRecorder->Shutdown();
   cout<<"Leaving stoprun"<<endl;
   return 0;
}


void DigiInterface::CloseThreads(bool Completely)
{
  cout<<"Entering closethreads"<<endl;
   if(m_ReadThread.IsOpen)  {
     cout<<"Closing read thread...";
      m_ReadThread.IsOpen=false;
      pthread_join(m_ReadThread.Thread,NULL);      
      cout<<" done!"<<endl;
   }
   for(unsigned int x=0;x<m_vProcThreads.size();x++)  {
     cout<<"Closing processing thread "<<x<<"..."<<flush;
      if(m_vProcThreads[x].IsOpen==false) continue;
      m_vProcThreads[x].IsOpen=false;
      pthread_join(m_vProcThreads[x].Thread,NULL);
      cout<<" done!"<<endl;
   }
   
   if(Completely)  {
     cout<<"Destroying proc threads...";
      for(unsigned int x=0;x<m_vProcThreads.size();x++)	{
	 if(m_vProcThreads[x].Processor != NULL)  {
	    delete m_vProcThreads[x].Processor;
	    m_vProcThreads[x].Processor = NULL;
	 }
      }
      m_vProcThreads.clear();		
      cout<<" done!"<<endl;
   }           
   cout<<"Leaving closethreads"<<endl;
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

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 16.05.2014
// File    : DAQMonitor.cc
// 
// Brief   : Class for managing DAQ for dispatcher
// 
// ****************************************************

#include "DAQMonitor.hh"

DAQMonitor::DAQMonitor()
{
   m_Log       =NULL;
   m_Mongodb   =NULL;
   koHelper::InitializeStatus(m_DAQStatus);
   koHelper::InitializeRunInfo(m_RunInfo);
   pthread_mutex_init(&m_DAQStatusMutex,NULL);
   m_bReady=false;
   m_detector = "";
}

DAQMonitor::DAQMonitor(vector<int> ports, vector<int> dports, vector<string> tags,
		       koLogger *logger,
		       MasterMongodbConnection *mongodb, string detector,
		       string ini_file)
{
  if(ports.size() == dports.size()){
    for(unsigned int x=0;x<ports.size();x++){
      koNetServer *new_network = new koNetServer(logger);
      new_network->Initialize(ports[x], dports[x], tags[x]);
      m_DAQNetworks.push_back(new_network);
    }
  }
  m_Log        = logger;
  m_Mongodb    = mongodb;
  koHelper::InitializeStatus(m_DAQStatus);
  koHelper::InitializeRunInfo(m_RunInfo);
  pthread_mutex_init(&m_DAQStatusMutex,NULL);
  m_bReady=false;
  m_detector = detector;
}

DAQMonitor::~DAQMonitor()
{
  pthread_mutex_destroy(&m_DAQStatusMutex);
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    delete m_DAQNetworks[x];
  m_DAQNetworks.clear();
}

void DAQMonitor::ProcessCommand(string command, string user, 
				   string comment, koOptions *options)
{         
  if(command=="Connect" && !m_DAQStatus.NetworkUp){
    if(Connect()!=0)
      m_Mongodb->SendLogMessage(("Error connecting DAQ network. Check settings. User: "+user),KOMESS_ERROR);
    else
      m_Mongodb->SendLogMessage(("DAQ network put up by "+user),KOMESS_NORMAL);
  }
  else if(command=="Disconnect" && m_DAQStatus.NetworkUp){
    if(Disconnect()!=0)
      m_Mongodb->SendLogMessage(("Error disconnecting DAQ network. Attempt made by "+user),KOMESS_ERROR);
    else
      m_Mongodb->SendLogMessage(("DAQ network disconnected by user "+user),KOMESS_NORMAL);
  }
  else if(command=="Stop"){
    Stop(user,comment);
    Shutdown();
  }
}

int DAQMonitor::ValidateStartCommand(string user, string comment, 
				     koOptions *options, string &message)
{
  int reply = 0; // 0, OK, 1 - warning, 2- no way
  //string message;
  // This function should check if the DAQ can be started with these options.
  // Failure due to i.e., DAQ already running, no source in for calib mode, etc.
  // A response is made to the UI via a DB entry.
  
  // CHECK SC

  if(m_DAQStatus.DAQState!=KODAQ_IDLE){
    reply=2;
    message="Dispatcher refuses to start the DAQ. It is not in 'idle' state.";
  }
  if(m_DAQStatus.NetworkUp!=true){
    reply=2;
    message="Dispatcher cannot start the DAQ. DAQ network is down.";
  }
  if(m_DAQStatus.Slaves.size()==0){
    reply=2;
    message="Dispatcher refuses to start the DAQ with no readers connected.";
  }

  if(reply==0) return 0;
  return -1;
}

int DAQMonitor::Connect()
{
  //Put up listening interface
  for(unsigned int x=0;x<m_DAQNetworks.size();x++){
    if(m_DAQNetworks[x]->PutUpNetwork()!=0){
      m_Mongodb->SendLogMessage("DAQMonitor::Connect could not put up interface",KOMESS_ERROR);
      return -1;
    }
  
    //Loop to add new slaves
    double timer=0.;
    int cID=-1;
    string cName="",cIP="";  
    int nSlaves=0;
    while(timer<2.)   { //wait 5 seconds for slaves to connect            
      if(m_DAQNetworks[x]->AddConnection(cID,cName,cIP)==0)  {
	stringstream errstring;
	errstring<<"Connected to slave "<<cName<<"("<<cID<<") at IP "<<cIP<<".";
	m_Mongodb->SendLogMessage(errstring.str(),KOMESS_NORMAL);	
	nSlaves++;
      }
      usleep(1000);
      timer+=0.001;    
    }
  
    //Finished. Take down listening interface (stop listening for new slaves)
    stringstream errstring;
    errstring<<"DAQ network online with "<<nSlaves<<" slaves.";
    m_Log->Message(errstring.str());
    //m_Mongodb->SendLogMessage(errstring.str(),KOMESS_NORMAL);
    m_DAQNetworks[x]->TakeDownNetwork();
    m_DAQStatus.NetworkUp=true;
  }
  //Start network checking loop
  pthread_create(&m_NetworkUpdateThread,NULL,DAQMonitor::UpdateThreadWrapper,
		 static_cast<void*>(this));
  return 0;
}

int DAQMonitor::LockStatus(){
  pthread_mutex_lock(&m_DAQStatusMutex);
  return 0;
}
int DAQMonitor::UnlockStatus(){
  pthread_mutex_unlock(&m_DAQStatusMutex);
  return 0;
}
void* DAQMonitor::UpdateThreadWrapper(void* monitor)
{
  DAQMonitor *thismonitor = static_cast<DAQMonitor*>(monitor);
  thismonitor->PollNetwork();
  return (void*)monitor;
}

void DAQMonitor::PollNetwork()
{
  time_t fPrevTime = koLogger::GetCurrentTime();
  while(m_DAQStatus.NetworkUp){    
    time_t fCurrentTime = koLogger::GetCurrentTime();                            
    for(unsigned int x=0;x<m_DAQNetworks.size();x++){
    
      // Poll Network
      pthread_mutex_lock(&m_DAQStatusMutex);
      while(m_DAQNetworks[x]->WatchDataPipe(m_DAQStatus)==0) m_bReady=true;
      pthread_mutex_unlock(&m_DAQStatusMutex);
    }
    usleep(500);

    // Check to see if any slaves are timing out    
    // or if a slave is in error
    for(unsigned int x=0;x<m_DAQStatus.Slaves.size();x++){
      if(difftime(fCurrentTime,m_DAQStatus.Slaves[x].lastUpdate)>60.){
	stringstream errtxt;
	errtxt<<"Dispatcher hasn't received a response from node "<<m_DAQStatus.Slaves[x].name<<" for at least 60 seconds.";
	m_DAQStatus.Slaves.erase(m_DAQStatus.Slaves.begin()+x);
	ThrowFatalError(true,errtxt.str());
      }
      // This block for catching caen errors and stopping daq
      //if(m_DAQStatus.Slaves[x].status == KODAQ_ERROR){
      //m_DAQStatus.Slaves.erase(m_DAQStatus.Slaves.begin()+x);
      //ThrowWarning(true, "Stopping the DAQ because slave " + 
      //koHelper::IntToString(x)+ " seems to have died.");
      //}
    }
    
    if(difftime(fCurrentTime,fPrevTime)>100.){
      for(unsigned int x=0;x<m_DAQNetworks.size();x++){
	m_DAQNetworks[x]->SendCommand("KEEPALIVE");
      }
      fPrevTime=fCurrentTime;
    }
  }
}

void DAQMonitor::ThrowFatalError(bool killDAQ, string errTxt)
{
  if(m_Mongodb!=NULL)
    m_Mongodb->SendLogMessage(errTxt,KOMESS_ERROR);
  if(killDAQ){
    Stop("dispatcher","Auto stop due to error");
    Shutdown();
  }    
}

void DAQMonitor::ThrowWarning(bool killDAQ, string errTxt)
{
  if(m_Mongodb!=NULL)
    m_Mongodb->SendLogMessage(errTxt,KOMESS_WARNING);
  if(killDAQ){
    if(m_Mongodb!=NULL)
      m_Mongodb->SendStopCommand("dispatcher", "Auto stop due to error",
				 m_detector);
    else{
      Stop("dispatcher","Auto stop due to error");
      Shutdown();
    }
  }
}


int DAQMonitor::Disconnect()
{
  for(unsigned int x=0;x<m_DAQNetworks.size();x++){
    m_DAQNetworks[x]->Disconnect();
  }
  koHelper::InitializeStatus(m_DAQStatus);
  sleep(2);
  pthread_join(m_NetworkUpdateThread,NULL);
  return 0;
}

int DAQMonitor::PreProcess(koOptions* mode){
  // 
  // Send preprocess
  m_DAQStatus.RunMode = m_DAQStatus.RunModeLabel = mode->GetString("name");
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    m_DAQNetworks[x]->SendCommand("PREPROCESS");

  // Send options to slaves           
  stringstream *optionsStream = new stringstream();
  mode->ToStream(optionsStream);
  for(unsigned int x=0;x<m_DAQNetworks.size();x++){
    if(m_DAQNetworks[x]->SendOptionsStream(optionsStream)!=0){
      delete optionsStream;
      m_Mongodb->SendLogMessage("Error sending options to clients in preprocess.",
				KOMESS_WARNING);
      return -1;
    }
  }
  delete optionsStream;
  return 0;
}

int DAQMonitor::Start(string run_name, string user, 
		      string comment, koOptions *options)
{
  //Check current state. DAQ must be armed
  int timer=0;
  while(m_DAQStatus.DAQState!=KODAQ_ARMED){
    sleep(1);
    timer++;
    if(timer==15){
      m_Mongodb->SendLogMessage("Timed out waiting for boards to arm",KOMESS_WARNING);
      return -1;
    }
  }
      
  /*koHelper::UpdateRunInfo(m_DAQStatus.RunInfo,user,m_detector);
  if(options->dynamic_run_names && options->write_mode == WRITEMODE_MONGODB){
    options->mongo_collection = koHelper::MakeDBName(m_DAQStatus.RunInfo,
						     options->mongo_collection);
    m_DAQNetwork->SendCommand("DBUPDATE");
    m_DAQNetwork->SendCommand(options->mongo_collection);
    }*/
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    m_DAQNetworks[x]->SendCommand("START");  
  stringstream mess;
  m_DAQStatus.RunInfo.RunNumber = run_name;
  mess<<"Run <b>"<<m_DAQStatus.RunInfo.RunNumber<<"</b> started by "<<user;
  m_DAQStatus.RunInfo.StartedBy = user;
  if( comment.length() > 0 )
    mess<<" : "<<comment;  
  if(m_Mongodb!=NULL){
    m_Mongodb->SendLogMessage(mess.str(),KOMESS_STATE);
    //m_Mongodb->Inme(user,options->name,m_DAQStatus.RunInfo.RunNumber, 
    //			  comment, m_detector, options);
  }
  return 0;
  /*
   // Get the next run number 
   koHelper::UpdateRunInfo(m_RunInfo,user);
   return -1;*/
}

int DAQMonitor::Stop(string user,string comment)
{
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    m_DAQNetworks[x]->SendCommand("STOP");
  stringstream mess;
  mess<<"Run <b>"<<m_DAQStatus.RunInfo.RunNumber<<"</b> stopped by "<<user;
  if(comment.length() > 0 )
    mess<<" : "<<comment;
  if(m_Mongodb!=NULL && m_DAQStatus.DAQState==KODAQ_RUNNING){
    m_Mongodb->SendLogMessage(mess.str(),KOMESS_STATE);
    m_Mongodb->UpdateEndTime(m_detector);
  }
  return 0;
}
int DAQMonitor::Shutdown()
{
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    m_DAQNetworks[x]->SendCommand("SLEEP");
  m_DAQStatus.RunMode="None";
  return 0;
}

int DAQMonitor::Arm(koOptions *mode, string run_name)
/*
  Arm the DAQ and configure the DDC-10 HE veto module
*/
{
  if(m_DAQStatus.DAQState!=KODAQ_IDLE)
      return -1;
  
  // Configure DDC10 if available
#ifdef WITH_DDC10
  //  ddc_10 heveto;
  //if(heveto.Initialize(mode->GetDDCStream())!=0)
  // m_Mongodb->SendLogMessage("DDC10 veto module could not be initialized.",
  //			      KOMESS_WARNING);
#endif

  // Send arm command
  m_DAQStatus.RunMode = m_DAQStatus.RunModeLabel = mode->GetString("name");

  // Get time
  m_DAQStatus.RunInfo.StartDate = koLogger::GetTimeString();
  for(unsigned int x=0;x<m_DAQNetworks.size();x++)
    m_DAQNetworks[x]->SendCommand("ARM");

  // WANT
  // IF MONGO
  //    IF RUN NAME != "" UPDATE RUN NAME
  //    IF READER in MONGO_OPTS[hosts] UPDATE MONGO_OPTS[address]
  // ELSE
  //    JUST SEND OPTIONS
  stringstream *optionsStream=NULL;
  
  // IF NOT MONGO, JUST SEND OPTIONS
  if(run_name == "" || mode->GetInt("write_mode") != WRITEMODE_MONGODB){
    optionsStream = new stringstream();
    mode->ToStream(optionsStream);
  }
  for(unsigned int x=0; x<m_DAQNetworks.size(); x++){

    // If we're not using mongo just send the options we created earlier
    if(run_name == "" || mode->GetInt("write_mode") != WRITEMODE_MONGODB){
      cout<<"Sending standalone options to reader " << m_DAQNetworks[x]->GetTag() << endl;
      if(m_DAQNetworks[x]->SendOptionsStream(optionsStream)!=0){
	delete optionsStream;
	m_Mongodb->SendLogMessage("Error sending options to clients.",
				  KOMESS_WARNING);
	return -1;
      }
      continue;
    }

    // If we are using mongo we might have to do something trickier
    // Different clients might get different hosts. So add that logic here.
    mongo_option_t mongo_opts = mode->GetMongoOptions();    
    if(mongo_opts.hosts.size()!=0){
      optionsStream = new stringstream();

      // Check if the current reader is in the options
      string hostname = m_DAQNetworks[x]->GetTag();
      if(mongo_opts.hosts.find(hostname) != mongo_opts.hosts.end()){
	cout<<"Updating options for host "<<hostname<<endl;
	mode->ToStream_MongoUpdate(run_name, optionsStream, 
				  mongo_opts.hosts[hostname]);
      }
      else{
	cout<<"Didn't find host "<<hostname<<endl;
	mode->ToStream_MongoUpdate(run_name, optionsStream);
      }
    }
    else if(run_name != ""){
      optionsStream = new stringstream();
      mode->ToStream_MongoUpdate(run_name, optionsStream);
    }
    else{ // We should have caught this case above, throw error
      m_Mongodb->SendLogMessage
	("DAQMonitor::Arm: ERROR sending options. Faulty logic.",
	 KOMESS_WARNING);
      return -1;
    }
    if(m_DAQNetworks[x]->SendOptionsStream(optionsStream)!=0){
      delete optionsStream;
      m_Mongodb->SendLogMessage("Error sending options to clients.",
				KOMESS_WARNING);
      return -1;
    }
    delete optionsStream;
    optionsStream = NULL;
  }

  if(optionsStream != NULL)
    delete optionsStream;
  
  return 0;
}

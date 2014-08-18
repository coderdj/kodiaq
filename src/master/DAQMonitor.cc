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
   m_DAQNetwork=NULL;
   m_Log       =NULL;
   m_Mongodb   =NULL;
   koHelper::InitializeStatus(m_DAQStatus);
   koHelper::InitializeRunInfo(m_RunInfo);
   pthread_mutex_init(&m_DAQStatusMutex,NULL);
   m_bReady=false;
}

DAQMonitor::DAQMonitor(koNetServer *DAQNetwork, koLogger *logger,
				   MasterMongodbConnection *mongodb)
{
   m_DAQNetwork = DAQNetwork;
   m_Log        = logger;
   m_Mongodb    = mongodb;
   koHelper::InitializeStatus(m_DAQStatus);
   koHelper::InitializeRunInfo(m_RunInfo);
   pthread_mutex_init(&m_DAQStatusMutex,NULL);
   m_bReady=false;
}


DAQMonitor::~DAQMonitor()
{
  pthread_mutex_destroy(&m_DAQStatusMutex);
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
  else if(command== "Start"){
    if(m_DAQStatus.NetworkUp && m_DAQStatus.DAQState==KODAQ_IDLE){
      if(Arm(options)==0)
	Start(user,comment,options);
      else Shutdown();
    }
  }
  else if(command=="Stop"){
    if(m_DAQStatus.DAQState==KODAQ_RUNNING)
      Stop(user,comment);
    Shutdown();
  }
}

int DAQMonitor::Connect()
{
  //Put up listening interface
  if(m_DAQNetwork->PutUpNetwork()!=0){
    m_Mongodb->SendLogMessage("DAQMonitor::Connect could not put up interface.",KOMESS_ERROR);
    return -1;
  }
  
  //Loop to add new slaves
  double timer=0.;
  int cID=-1;
  string cName="",cIP="";  
  int nSlaves=0;
  while(timer<5.)   { //wait 5 seconds for slaves to connect            
    if(m_DAQNetwork->AddConnection(cID,cName,cIP)==0)  {
      stringstream errstring;
      errstring<<"Connected to slave "<<cName<<"("<<cID<<") at IP "<<cIP<<".";
      cout<<errstring.str()<<endl;
      m_Mongodb->SendLogMessage(errstring.str(),KOMESS_NORMAL);	
      nSlaves++;
    }
    usleep(1000);
    timer+=0.001;    
  }
  
  //Finished. Take down listening interface (stop listening for new slaves)
  stringstream errstring;
  errstring<<"DAQ network online with "<<nSlaves<<" slaves.";
  m_Mongodb->SendLogMessage(errstring.str(),KOMESS_NORMAL);
  m_DAQNetwork->TakeDownNetwork();
  m_DAQStatus.NetworkUp=true;
  
  //Start network checking loop
  pthread_create(&m_NetworkUpdateThread,NULL,DAQMonitor::UpdateThreadWrapper,
		 static_cast<void*>(this));
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
    
    pthread_mutex_lock(&m_DAQStatusMutex);
    while(m_DAQNetwork->WatchDataPipe(m_DAQStatus)==0) m_bReady=true;
    pthread_mutex_unlock(&m_DAQStatusMutex);
    usleep(10000);
    
    if(difftime(fCurrentTime,fPrevTime)>100.){
      m_DAQNetwork->SendCommand("KEEPALIVE");
      fPrevTime=fCurrentTime;
    }
  }
}

int DAQMonitor::Disconnect()
{
  m_DAQNetwork->Disconnect();
  koHelper::InitializeStatus(m_DAQStatus);
  sleep(2);
  pthread_join(m_NetworkUpdateThread,NULL);
  return 0;
}

int DAQMonitor::Start(string user, string comment, koOptions *options)
{
  //Check current state. DAQ must be armed
  int timer=0;
  while(m_DAQStatus.DAQState!=KODAQ_ARMED){
    sleep(1);
    timer++;
    if(timer==5)
      return -1;
  }
  
  koHelper::UpdateRunInfo(m_DAQStatus.RunInfo,user);
  if(options->dynamic_run_names && options->write_mode == WRITEMODE_MONGODB){
    options->mongo_collection = koHelper::MakeDBName(m_DAQStatus.RunInfo,
						     options->mongo_collection);
    m_DAQNetwork->SendCommand("DBUPDATE");
    m_DAQNetwork->SendCommand(options->mongo_collection);
  }
  m_DAQNetwork->SendCommand("START");  
  if(m_Mongodb!=NULL)
    m_Mongodb->Initialize(user,options->name,m_DAQStatus.RunInfo.RunNumber,
			  options,false);
  return 0;
  /*
   // Get the next run number 
   koHelper::UpdateRunInfo(m_RunInfo,user);
   return -1;*/
}

int DAQMonitor::Stop(string user,string comment)
{
  m_DAQNetwork->SendCommand("STOP");
  stringstream mess;
  mess<<"DAQ stopped by "<<user;
  if(m_Mongodb!=NULL){
    m_Mongodb->SendLogMessage(mess.str(),KOMESS_STATE);
    m_Mongodb->UpdateEndTime();
  }
  return 0;
}
int DAQMonitor::Shutdown()
{
  m_DAQNetwork->SendCommand("SLEEP");
  m_DAQStatus.RunMode="None";
  return 0;
}
int DAQMonitor::Arm(koOptions *mode)
{
  if(m_DAQStatus.DAQState!=KODAQ_IDLE)
    return -1;
  m_DAQStatus.RunMode = m_DAQStatus.RunModeLabel = mode->name;
  m_DAQNetwork->SendCommand("ARM");
  stringstream *optionsStream = new stringstream();
  mode->ToStream(optionsStream);
  if(m_DAQNetwork->SendOptionsStream(optionsStream)!=0){
    delete optionsStream;
    m_Mongodb->SendLogMessage("Error sending options to clients.",KOMESS_WARNING);
    return -1;
  }
  delete optionsStream;
  return 0;
}

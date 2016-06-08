/*
 *
 * File: MasterControl.cc
 * Author: Daniel Coderre
 * Date: 15.07.2015
 *
 * Brief: General control class for dispatcher
 *
*/

#include "MasterControl.hh"
#include <fstream>

MasterControl::MasterControl(){
  mLog = new koLogger("log/master.log");
  mMongoDB = NULL;
}

MasterControl::~MasterControl(){
  
  delete mLog;
  for(auto iter: mDetectors)
    delete iter.second;
  mDetectors.clear();
  if(mMongoDB!=NULL)
    delete mMongoDB;
}

int MasterControl::Initialize(string filepath){
  
  ifstream infile;
  infile.open(filepath.c_str());
  if(!infile){
    if(mLog!=NULL)
      mLog->Error("Bad options file given to master: " + filepath);
    cout<<"Bad options file! Try again."<<endl;
    return -1;
  }

  // We want to fill these options
  string LOG_DB="", MONITOR_DB="", RUNS_DB="";
  string RUNS_DB_NAME="", LOG_DB_NAME="", MONITOR_DB_NAME="";
  string RUNS_COLLECTION="";
  string BUFFER_USER="", BUFFER_PASSWORD="";
  // Also detectors but will declare them inline

  // Declare mongodb
  mMongoDB = new MasterMongodbConnection(mLog);

  string line;
  vector <string> detectors;
  vector <vector <int> >ports;
  vector <vector <int> >dports;
  vector <string> inis;
  while(!infile.eof()){
    getline( infile, line );
    if ( line[0] == '#' ) continue; //ignore comments  
    
    //parse
    istringstream iss(line);
    vector<string> words;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(words));
    if(words.size()<2) continue;
    
    if(words[0] == "LOG_DB")
      LOG_DB=words[1];
    else if(words[0] == "MONITOR_DB")
      MONITOR_DB=words[1];
    else if(words[0] == "RUNS_DB") 
      RUNS_DB=words[1];
    else if(words[0] == "RUNS_DB_NAME")
      RUNS_DB_NAME=words[1];
    else if(words[0] == "MONITOR_DB_NAME")
      MONITOR_DB_NAME=words[1];
    else if(words[0] == "LOG_DB_NAME")
      LOG_DB_NAME=words[1];
    else if(words[0] == "RUNS_COLLECTION")
      RUNS_COLLECTION=words[1];
    else if(words[0]=="BUFFER_USER")
      BUFFER_USER=words[1];
    else if(words[0]=="BUFFER_PASSWORD")
      BUFFER_PASSWORD=words[1];

    else if(words[0] == "DETECTOR" && words.size()>=5){
      string name = words[1];
      int port=koHelper::StringToInt(words[2]);
      int dport = koHelper::StringToInt(words[3]);
      string ini = words[4];
      // See if we have this one already
      int det_index = -1;
      for(unsigned int x=0;x<detectors.size();x++){
	if(detectors[x]==name)
	  det_index=x;
      }      
      if(det_index == -1){
	mStatusUpdateTimes[name] = koLogger::GetCurrentTime();
	mRatesUpdateTimes[name] = koLogger::GetCurrentTime();
	vector<int> thisport, thisdport;
	thisport.push_back(port);
	thisdport.push_back(dport);
	ports.push_back(thisport);
	dports.push_back(thisdport);
	detectors.push_back(name);
	inis.push_back(ini);
      }
      else{
	ports[det_index].push_back(port);
	dports[det_index].push_back(dport);
      }
    }
  }
  for(unsigned int x=0;x<detectors.size();x++){
    DAQMonitor *monitor = new DAQMonitor(ports[x], dports[x], mLog, mMongoDB, 
					 detectors[x], inis[x]);
    mDetectors[detectors[x]]=monitor;
  }
  cout<<"Found "<<mDetectors.size()<<" detectors"<<endl;
  
  //Set mongo dbs (if any, can be empty) and return
  mMongoDB->SetDBs(LOG_DB, MONITOR_DB, RUNS_DB,
		   LOG_DB_NAME, MONITOR_DB_NAME, RUNS_DB_NAME,
		   RUNS_COLLECTION, BUFFER_USER, BUFFER_PASSWORD);//, DB_USER, DB_PASSWORD, DB_AUTH);
  return 0;     
}

vector<string> MasterControl::GetDetectorList(){
  vector<string> retvec;
  for(auto iterator:mDetectors)
    retvec.push_back(iterator.first);
  return retvec;
}

void MasterControl::Close(){
  Stop();
  Disconnect();

  for(auto iter: mDetectors)
    delete iter.second;
  mDetectors.clear();

  //for(auto iter: mMonitors)
  //delete iter.second;
  //mMonitors.clear();

  if(mMongoDB!=NULL)
    delete mMongoDB;
  mMongoDB = NULL;
}

void MasterControl::Connect(string detector){
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      iterator.second->ProcessCommand("Connect", "dispatcher_console");
  }  
}

void MasterControl::Disconnect(string detector){
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      iterator.second->ProcessCommand("Disconnect", "dispatcher_console");
  }
}

void MasterControl::Stop(string detector, string user, string comment){
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      iterator.second->ProcessCommand("Stop", user, comment);
  }

  mMongoDB->UpdateEndTime(detector);

  // If either fStartTimes or fExpireAfterSeconds exists for this detector 
  // then we have to change it 
  vector<string> detectorsToStop;
  if(detector == "all"){
    detectorsToStop.push_back("tpc");
    detectorsToStop.push_back("muon_veto");
  }
  else
    detectorsToStop.push_back(detector);
  for(unsigned int x=0; x<detectorsToStop.size(); x++){
    if( fExpireAfterSeconds.find(detectorsToStop[x]) != fExpireAfterSeconds.end() )
      fExpireAfterSeconds.erase(detectorsToStop[x]);
    if( fStartTimes.find(detectorsToStop[x]) != fStartTimes.end() )
      fStartTimes.erase(detectorsToStop[x]);
  }
}

void MasterControl::CheckRunQueue(){
  
  // Here's what we want. First, check if there are any runs going that should
  // be stopped. This means compare fStartTimes with fExpireAfterSeconds
  cout<<"Iterating start times"<<endl;
  for(auto iter : fStartTimes) {
    
    if( fExpireAfterSeconds.find(iter.first) != fExpireAfterSeconds.end() ){
      // Calculate how long run has been going   
      time_t currentTime = koLogger::GetCurrentTime();
      double dTime = difftime( currentTime, iter.second);
      
      cout<<"Found run that started "<<dTime<<" seconds ago with "<<fExpireAfterSeconds[iter.first] << "limit"<<endl;
      // Should we stop it? 
      if(dTime > fExpireAfterSeconds[iter.first] && 
	 fExpireAfterSeconds[iter.first]!=0)
        Stop(iter.first, "dispatcher_autostop", "Run automatically stopped after "
             + koHelper::IntToString(fExpireAfterSeconds[iter.first]) + "seconds.");
    }
  }
  cout<<"Done with start times"<<endl;

  // Second see if the next command in the queue can be run yet (if any).  
  // This should be detector-aware. Like if the muon veto is idle it can loop the 
  // queue to find the next run mentioning a muon veto (all or muon_veto). If
  // it's 'muon_veto' then it can start the run. If it's 'all' it should wait for
  // the TPC to finish.  

  // Sync run queue with remote                                                       
  fDAQQueue = mMongoDB->GetRunQueue();
  
  bool sawTPC=false, sawMV=false;
  for(unsigned int x=0; x<fDAQQueue.size(); x++){
    
    // Case 1: We find a command that wants to start both. This only works 
    // if there is no run going for the TPC or the muon veto and if we haven't
    // found a command for either yet
    bool abort = false;
    string doc_det = fDAQQueue[x].getStringField("detector");
    if(doc_det == "all" && !sawTPC && !sawMV){
      sawTPC =sawMV =true;
      // Now check if the detectors are both idle and that there are no start-time 
      // entries for them
      for(auto iter : mDetectors) {
        if(iter.second->GetStatus()->DAQState != KODAQ_IDLE ||
           fStartTimes.find(iter.first) != fStartTimes.end())
          abort = true;
      }
    }
    else if(mDetectors.find(doc_det) == mDetectors.end()){
      fDAQQueue.erase(fDAQQueue.begin()+x);
      x--;
      continue;
    }
    else if(doc_det == "tpc" &&  !sawTPC){
      sawTPC = true;
      // Just check if the TPC is idle                                                
      if(mDetectors["tpc"]->GetStatus()->DAQState != KODAQ_IDLE ||
	 fStartTimes.find("tpc") != fStartTimes.end())
        abort = true;
    }
    else if(doc_det == "muon_veto" && !sawMV){
      sawMV = true;
      
      // Just check if the MV is idle          
      if(mDetectors["muon_veto"]->GetStatus()->DAQState != KODAQ_IDLE ||
         fStartTimes.find("muon_veto") != fStartTimes.end())
        abort =true;
    }
    else 
      abort=true;
    if(abort)
      continue;
    mMongoDB->InsertOnline("monitor", "run.daq_control", fDAQQueue[x]);
    fDAQQueue.erase(fDAQQueue.begin() + x);
    mMongoDB->SyncRunQueue(fDAQQueue);
    break;
  }
  return;





}

int MasterControl::Start(string detector, string user, string comment, 
			 map<string,koOptions*> options, bool web, int expireAfterSeconds){
  // Return values:
  //                 0 - Success
  //                -1 - Failed, reset the DAQ
  //                -2 - Failed, no need to reset the DAQ
  //

  //if(options==NULL)
  //return -2;

  // We have a staged start. Let's move through the stages. 
  if(web)
    mMongoDB->SendRunStartReply(11, "Received command to start detector " + 
				detector + " from user " + user);
  
  // Validation step
  cout<<"Received start command. Validating..."<<flush;
  //if(web)
  //mMongoDB->SendRunStartReply(12, "Validating start command");
  int valid_success=0;
  string message="";
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      valid_success+=iterator.second->ValidateStartCommand(user, comment,
							   options[iterator.first], 
							   message);
  }
  if(valid_success!=0){
    cout<<"Error during command validation! Aborting run start.";
    if(web)
      mMongoDB->SendRunStartReply(18, "Error during command validation! Aborting. " + message);
    return -2;
  }
  cout<<"Success!"<<endl;
  


  // This might actually work. Let's assign a run name.
 
  string run_name = "xenon1t";
  for(auto iterator:options){
    if(options[iterator.first]->HasField("run_prefix"))
      run_name = options[iterator.first]->GetString("run_prefix");
  }
  run_name = koHelper::GetRunNumber(run_name);
  cout<<"This run will be called "<<run_name<<endl;
  //if(web)
  //mMongoDB->SendRunStartReply(12, "Assigning run name: " + run_name);
  
  // Arm the boards
  cout<<"Arming the digitizers..."<<flush;
  if(web)
    mMongoDB->SendRunStartReply(12, "Arming digitizers for run " + run_name);
  int arm_success=0;
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      arm_success+=iterator.second->Arm(options[iterator.first], run_name);
  }
  if(arm_success!=0){
    cout<<"Error arming the boards. Run "<<run_name<<" has been aborted."<<endl;
    if(web)
      mMongoDB->SendRunStartReply(18, "Error arming boards. Run " + run_name + 
				  " has been aborted.");
    return -1;
  }
  // Now you have to check that the boards actually go into arm state.
  // This should be done within a certain timeout.
  bool all_armed=true;
  int armedCounter = 0;
  do{
    all_armed=true;
    for(auto iterator:mDetectors){      
      if(iterator.first==detector || detector=="all"){
	iterator.second->LockStatus();
	if(iterator.second->GetStatus()->DAQState != KODAQ_ARMED)
	  all_armed = false;
	iterator.second->UnlockStatus();
      }
    }
    if(!all_armed)
      usleep(100000);    
    armedCounter++;
    if(armedCounter % 20 == 0)
      cout<<"Waiting for boards to arm..."<<armedCounter/10<<endl;
  }while(all_armed == false && armedCounter <200);
  if(!all_armed){
    cout<<"Error arming the boards. Run "<<run_name<<" has been aborted."<<endl;
    if(web)
      mMongoDB->SendRunStartReply(18, "Error arming boards. Run " + run_name +
                                  " has been aborted.");
    return -1;
  }
  cout<<"Success!"<<endl;

  // Insert the run doc and update the noise directory
  if(web){
    
    //if(((detector == "all" || detector == "tpc") && 
    //	options["tpc"]->GetInt("noise_spectra_enable")==1) || 
    // (detector != "all" && 
    //	options[detector]->GetInt("noise_spectra_enable")==1))
      //mMongoDB->UpdateNoiseDirectory(run_name);
    mMongoDB->SendRunStartReply(18, "Configuring databases for run " + run_name);

    mMongoDB->InsertRunDoc(user, run_name, comment, options, run_name);
  }
  // Start the actual run
  cout<<"Sending start command..."<<flush;
  //if(web)
  //mMongoDB->SendRunStartReply(13, "Sending start command");
  int start_success=0;
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      start_success+=iterator.second->Start(run_name, user, 
					    comment,options[iterator.first]);
  }
  if(start_success!=0){
    cout<<"Error starting the run. Run "<<run_name<<" has been aborted."<<endl;
    if(web)
      mMongoDB->SendRunStartReply(18, "Error starting run. Run " + run_name +
                                  " has been aborted.");
    return -1;
  }
  cout<<"Success!"<<endl;
  
  // Update when this detector started 
  if(expireAfterSeconds!=0){
    fStartTimes[detector] = koLogger::GetCurrentTime();
    fExpireAfterSeconds[detector] = expireAfterSeconds;
  }


  if(web)
    mMongoDB->SendRunStartReply(19, "Run " + run_name + " started by " + user);

  //    mMongoDB->SendRunStartReply(19, "Successfully completed run start procedure.");
  
  return 0;
}

void MasterControl::CheckRemoteCommand(){

  if(mMongoDB==NULL)
    return;

  //Get command from mongo
  string command,detector,user,comment;
  map<string, koOptions*> options;
  bool override=false;
  int expireAfterSeconds =0;
  mMongoDB->CheckForCommand(command,user,comment,detector,override,
			    options,expireAfterSeconds);

  // If command is stop
  if(command == "Stop")
    Stop(detector, user, comment);
  else if(command=="Start"){
    if(Start(detector,user,comment,options,true,expireAfterSeconds)==-1){
      cout<<"Sending stop command."<<endl;
      Stop(detector, "AUTO_DISPATCHER", "ABORTED: RUN START FAILED");
    }
    if( detector != "all" )
      delete options[detector];
    else{
      delete options["tpc"];
      delete options["muon_veto"];
    }
  }
  return;
}

void MasterControl::StatusUpdate(){

  for(auto iter : mDetectors) {
    if( iter.second->UpdateReady() ) {
      time_t CurrentTime = koLogger::GetCurrentTime();
      double dTimeStatus = difftime( CurrentTime, 
				     mStatusUpdateTimes[iter.first] );
      if ( dTimeStatus > 5. ) { //don't flood DB with updates
	mStatusUpdateTimes[iter.first] = CurrentTime;
	iter.second->LockStatus();
	mMongoDB->UpdateDAQStatus( iter.second->GetStatus(), 
				 iter.first );
	iter.second->UnlockStatus();
      }
      double dTimeRates = difftime( CurrentTime,
				    mRatesUpdateTimes[iter.first]);
      if ( dTimeRates > 10. ) { //send rates
	mRatesUpdateTimes[iter.first] = CurrentTime;
	iter.second->LockStatus();
	mMongoDB->AddRates( iter.second->GetStatus() );
	iter.second->UnlockStatus();
	//	mMongoDB->UpdateDAQStatus( iter.second->GetStatus(),
	//			 iter.first );
      }
    }
  } // End status update
}

string MasterControl::GetStatusString(){
  
  
  stringstream ss;
  ss<<"***************************************************************"<<endl;
  ss<<"DAQ Status Update: "<<koLogger::GetTimeString()<<endl;
  ss<<"***************************************************************"<<endl;
  for(auto iter : mDetectors) {
    iter.second->LockStatus();
    ss<<"Detector: "<<iter.first;
    if( iter.second->GetStatus()->DAQState == KODAQ_IDLE)
      ss<<" IDLE"<<endl;
    else if( iter.second->GetStatus()->DAQState == KODAQ_ARMED)
      ss<<" ARMED in mode "<<iter.second->GetStatus()->RunMode<<endl;
    else if( iter.second->GetStatus()->DAQState == KODAQ_RUNNING)
      ss<<" RUNNING in mode "<<iter.second->GetStatus()->RunMode<<endl;
    else if( iter.second->GetStatus()->DAQState == KODAQ_RDY)
      ss<<" READY in mode "<<iter.second->GetStatus()->RunMode<<endl;
    else if( iter.second->GetStatus()->DAQState == KODAQ_ERROR)
      ss<<" ERROR"<<endl;
    else
      ss<<" UNDEFINED"<<endl;
    koStatusPacket_t *status = iter.second->GetStatus();
    for(unsigned int x=0; x<status->Slaves.size(); x++){

      //string name = status->Slaves[x].name;
      //cout<<x<<endl;
      //cout<<name.size()<<endl;
      //cout<<name<<endl;

      
      // Check initializes
      if(status->Slaves[x].name.empty()){
	mLog->Message("Corrupted slave data");
	continue;
      }
      string a = status->Slaves[x].name;
      ss<<"        "<<status->Slaves[x].name<<": ";
      ss<<status->Slaves[x].nBoards<<" boards. Rate: ";
      ss<<status->Slaves[x].Rate<<" (MB/s) @ ";
      ss<<status->Slaves[x].Freq<<" Hz"<<endl;
    }
    iter.second->UnlockStatus();
  }
  ss<<"***************************************************************"<<endl;
  return ss.str();          
}

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
}


int MasterControl::Start(string detector, string user, string comment, 
			 map<string,koOptions*> options, bool web){
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
  if(web)
    mMongoDB->SendRunStartReply(12, "Validating start command");
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
  
  // Preprocess Step
  /*cout<<"Performing run preprocessing..."<<flush;
  if(web)
    mMongoDB->SendRunStartReply(12, "Performing run preprocessing.");
  int preprocess_success=0;
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      preprocess_success+=iterator.second->PreProcess(options[iterator.first]);
  }
  if(preprocess_success!=0){
    cout<<"Error during preprocessing! Aborting run start.";
    if(web)
      mMongoDB->SendRunStartReply(18, "Error during preprocessing! Aborting.");
    return -1;
  }
  cout<<"Success!"<<endl;
  
  cout<<"Waiting for ready condition."<<endl;
  time_t start_wait = koLogger::GetCurrentTime();
  bool ready=false;
  while(!ready){
    ready = true;
    for(auto iterator:mDetectors){
      if(iterator.first==detector || detector=="all"){
	if(iterator.second->GetStatus().DAQState != KODAQ_RDY)
	  ready=false;
      }
    }
    sleep(1);
    time_t current_time = koLogger::GetCurrentTime();
    double tdiff =difftime( current_time, start_wait );
    if(tdiff>30)
      break;
  }
  if(!ready){
    cout<<"Processing timed out! Aborting run start.";
    if(web)
      mMongoDB->SendRunStartReply(18, "Processing timed out! Aborting.");
    return -1;
  }
  */

  // This might actually work. Let's assign a run name.
  string run_name = "";
  //if(detector == "all" || detector == "tpc")
  //run_name = koHelper::GetRunNumber(options["tpc"]->GetString("run_prefix"));
  //  else 
  //run_name = koHelper::GetRunNumber(options[detector]->GetString("run_prefix"));
  run_name = koHelper::GetRunNumber();
  cout<<"This run will be called "<<run_name<<endl;
  if(web)
    mMongoDB->SendRunStartReply(12, "Assigning run name: " + run_name);
  
  // Arm the boards
  cout<<"Arming the digitizers..."<<flush;
  if(web)
    mMongoDB->SendRunStartReply(12, "Arming digitizers.");
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
    
    mMongoDB->InsertRunDoc(user, run_name, comment, options, run_name);
  }
  // Start the actual run
  cout<<"Sending start command..."<<flush;
  if(web)
    mMongoDB->SendRunStartReply(13, "Sending start command");
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
  
  if(web)
    mMongoDB->SendRunStartReply(19, "Successfully completed run start procedure.");
  
  return 0;
}

void MasterControl::CheckRemoteCommand(){

  if(mMongoDB==NULL)
    return;

  //Get command from mongo
  string command,detector,user,comment;
  map<string, koOptions*> options;
  bool override=false;
  mMongoDB->CheckForCommand(command,user,comment,detector,override,options);

  // If command is stop
  if(command == "Stop")
    Stop(detector, user, comment);
  else if(command=="Start"){
    if(Start(detector,user,comment,options,true)==-1){
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

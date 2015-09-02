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
  //for(auto iter: mMonitors)
  //delete iter.second; 
  //mMonitors.clear();
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
  // Also detectors but will declare them inline

  // Declare mongodb
  mMongoDB = new MasterMongodbConnection(mLog);

  string line;
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
    else if(words[0] == "DETECTOR" && words.size()>=5){
      string name = words[1];
      int port=koHelper::StringToInt(words[2]);
      int dport = koHelper::StringToInt(words[3]);
      string ini = words[4];
      
      mStatusUpdateTimes[name] = koLogger::GetCurrentTime();
      mRatesUpdateTimes[name] = koLogger::GetCurrentTime();

      // This object gets created here. The DAQ Monitor then owns it.
      //koNetServer *server = new koNetServer(mLog);
      mLog->Message("Passing log to server");
      //koNetServer server(mLog);
      //server.Initialize(port,dport);
      
      // Declare the detector
      DAQMonitor *monitor = new DAQMonitor(port, dport, mLog, mMongoDB, name, ini);
      //DAQMonitor *monitor = new DAQMonitor(server, mLog, mMongoDB, name, ini);
      mDetectors[name] = monitor;
      //mMonitors[name] = server;
    }
  }
  cout<<"Found "<<mDetectors.size()<<" detectors."<<endl;

  //Set mongo dbs (if any, can be empty) and return
  mMongoDB->SetDBs(LOG_DB, MONITOR_DB, RUNS_DB);
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
  for(auto iterator:mDetectors){
    if(iterator.first==detector || detector=="all")
      valid_success+=iterator.second->ValidateStartCommand(user, comment,
							   options[iterator.first]);
  }
  if(valid_success!=0){
    cout<<"Error during command validation! Aborting run start.";
    if(web)
      mMongoDB->SendRunStartReply(18, "Error during command validation! Aborting.");
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
  if(detector == "all" || detector == "tpc")
    run_name = koHelper::GetRunNumber(options["tpc"]->GetString("run_prefix"));
  else 
    run_name = koHelper::GetRunNumber(options[detector]->GetString("run_prefix"));
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
  bool all_armed=false;
  int armedCounter = 0;
  while(all_armed == false && armedCounter <200){
    for(auto iterator:mDetectors){
      if(iterator.first==detector || detector=="all"){
	if(iterator.second->GetStatus().DAQState != KODAQ_ARMED)
	  all_armed = false;
      }
    }
    if(!all_armed)
      usleep(100000);    
    armedCounter++;
    if(armedCounter % 20 == 0)
      cout<<"Waiting for boards to arm..."<<armedCounter/10<<endl;
  }
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
    if(((detector == "all" || detector == "tpc") && 
	options["tpc"]->GetInt("noise_spectra_enable")==1) || 
       (detector != "all" && 
	options[detector]->GetInt("noise_spectra_enable")==1))
      mMongoDB->UpdateNoiseDirectory(run_name);
    
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
	mMongoDB->UpdateDAQStatus( iter.second->GetStatus(), 
				 iter.first );
      }
      double dTimeRates = difftime( CurrentTime,
				    mRatesUpdateTimes[iter.first]);
      if ( dTimeRates > 10. ) { //send rates
	mRatesUpdateTimes[iter.first] = CurrentTime;
	mMongoDB->AddRates( iter.second->GetStatus() );
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
    ss<<"Detector: "<<iter.first;
    if( iter.second->GetStatus().DAQState == KODAQ_IDLE)
      ss<<" IDLE"<<endl;
    else if( iter.second->GetStatus().DAQState == KODAQ_ARMED)
      ss<<" ARMED in mode "<<iter.second->GetStatus().RunMode<<endl;
    else if( iter.second->GetStatus().DAQState == KODAQ_RUNNING)
      ss<<" RUNNING in mode "<<iter.second->GetStatus().RunMode<<endl;
    else if( iter.second->GetStatus().DAQState == KODAQ_RDY)
      ss<<" READY in mode "<<iter.second->GetStatus().RunMode<<endl;
    else if( iter.second->GetStatus().DAQState == KODAQ_ERROR)
      ss<<" ERROR"<<endl;
    else
      ss<<" UNDEFINED"<<endl;
    cout<<iter.second->GetStatus().Slaves.size()<<" nslaves"<<endl;
    for(unsigned int x=0; x<iter.second->GetStatus().Slaves.size(); x++){
      ss<<"        "<<iter.second->GetStatus().Slaves[x].name<<": "<<
	iter.second->GetStatus().Slaves[x].nBoards<<" boards. Rate: "<<
	iter.second->GetStatus().Slaves[x].Rate<<" (MB/s) @ "<<
	iter.second->GetStatus().Slaves[x].Freq<<" Hz"<<endl;
    }
  }
  ss<<"***************************************************************"<<endl;
  return ss.str();          
}

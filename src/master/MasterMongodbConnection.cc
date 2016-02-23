// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : MasterMongodbConnection.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
// 
// Brief   : Facilitates connection between master and
//           mongodb database in order to update general
//           run information documents.
// 
// ****************************************************

#include "MasterMongodbConnection.hh"
#include "mongo/util/net/hostandport.h"

MasterMongodbConnection::MasterMongodbConnection()
{
   fLog=NULL;
   fOptions=NULL;
   fLogDB = NULL;
   fMonitorDB = NULL;
   fRunsDB = NULL;
   fLogDBName=fMonitorDBName=fRunsDBName="run";
   fBufferUser=fBufferPassword="";
   fRunsCollection="runs";   
}

MasterMongodbConnection::MasterMongodbConnection(koLogger *Log)
{
   fLog=Log;
   fOptions=NULL;
   fLogDB = NULL;
   fMonitorDB = NULL;
   fRunsDB = NULL;
   fLogDBName=fMonitorDBName=fRunsDBName="run";
   fBufferUser=fBufferPassword="";
   fRunsCollection="runs";
}

MasterMongodbConnection::~MasterMongodbConnection()
{
}

int MasterMongodbConnection::SetDBs(string logdb, string monitordb, 
				    string runsdb, string logname, 
				    string monitorname, string runsname, 
				    string runscollection, string bufferUser, 
				    string bufferPassword){//, string user,
  //				    string password, string dbauth){
  string errmsg="";
  fLogString = logdb;
  fMonitorString = monitordb;
  fRunsString = runsdb;

  fBufferUser = bufferUser;
  fBufferPassword=bufferPassword;
  fLogDBName=logname;
  fMonitorDBName=monitorname;
  fRunsDBName=runsname;
  fRunsCollection=runscollection;
  return Connect();
}

int MasterMongodbConnection::Connect(){

  // If we didn't set these yet then quit
  //if(fLogString="" || fMonitorString="" || fRunsString="")
  //return -1;

  // Will check success here
  bool logconnected = true, monitorconnected = true, runsconnected = true;

  // If previously defined then close the old connections
  if(fLogDB != NULL)
    delete fLogDB;
  if(fMonitorDB != NULL)
    delete fMonitorDB;
  if(fRunsDB != NULL )
    delete fRunsDB;

  // Connect logdb
  try{
    fLogDB = new mongocxx::client{fLogDB};            
  }
    catch(const mongocxx::exception &e){
      delete fLogDB;
      fLogDB = NULL;
      if(fLog!=NULL) 
	fLog->Error("Problem connecting to log mongo. Caught exception " + 
		    string(e.what()));        
      logconnected = false;
    }  
  
  // Connect monitor db
    try{
      fLogDB = new mongocxx::client{fMonitorString};
    }
    catch( const mongocxx::exception &e ){
      delete fMonitorDB;
      fMonitorDB = NULL;
      if(fLog!=NULL) 
	fLog->Error("Problem connecting to monitor mongo. Caught exception " +
		    string(e.what()));
      monitorconnected = false;
    }
  
  // Connect to runs db
    try{
      fLogDB = new mongocxx::client{fRunsString};
    }
    catch( const mongocxx::exception &e ){
      delete fRunsDB;
      fRunsDB = NULL;
      if(fLog!=NULL) 
	fLog->Error("Problem connecting to runs mongo. Caught exception " +
		    string(e.what()) );
      runsconnected = false;
    }
    if( logconnected && monitorconnected && runsconnected )     
      return 0;
    return -1;
}
 
void MasterMongodbConnection::InsertOnline(string DB, 
					   string collection,
					   bsoncxx::document bson){
  string db = collection.substr(0, collection.find(".");
  string coll = collection.substr(collection.find("."), collection.size()); 

  // Choose proper DB
  mongocxx::client *thedb = NULL;
  if( DB == "monitor")
    thedb = fMonitorDB;  
  else if( DB=="runs")
    thedb = fRunsDB;  
  else if( DB=="log")
    thedb = fLogDB;
  
  auto col = c[db][coll];

  if(thedb != NULL)
    optional<result::insert_one> res = col->insert_one(bson);

}

int MasterMongodbConnection::InsertRunDoc(string user, string name, 
					  string comment, 
					  map<string,koOptions*> options_list,
					  string collection)
/*
  At run start create a new run document and put it into the online.runs database.
  The OID of this document is saved as a private member so the document can be 
  updated when the run ends. 
  
  Fields important for trigger:
                              
                  runtype :   tells trigger how run should be processed. Hardcoded
                              at the moment but will be part of koOptions once we
                              have different run types
                  starttime:  NOT the UTC time (this is in runstart) but should be
                              the first CAEN digitizer time in the run (usually 0)
		  	     
*/
{
  // Super useful
  using builder::stream::open_document;
  using builder::stream::close_document;
  using builder::stream::open_array;
  using builder::stream::close_array;

  for ( auto iterator: options_list){
    
    // Our options object
    koOptions *options = iterator.second;

    //Create a bson object with the run information
    auto obj = bsoncxx::builder::stream::document{};
    // Get the run number for TPC runs
    int run_number = 0;    
    if(iterator.first == "tpc"){
      mongocxx::collection coll = fRunsDB[fRunsDBName][fRunsCollection];
      
      // Grab the most recent runs doc and get the run number
      optional<bsoncxx::document::view> doc 
	= coll.find_one(document{}<<"detector"<<"tpc"<<finalize,
			document{}<<"number"<<-1<<finalize);
      
      if(!doc.empty())
	run_number = doc["number"].get_int32()+1;
    }
    
    // Top level stuff
    time_t currentTime;
    time(&currentTime);
    obj << "name" << name << "user" << user << "detector" << iterator.first <<
    "number" << run_number << "start" << currentTime;

    // Reader doc
    bool DPP=true;
    for(int x=0;x<options->GetVMEOptions();x++){
      if(options->GetVMEOption(x).address == 0x8080
         && options->GetVMEOption(x).value&(1 << 24))
        DPP=false;
    }
    obj << "reader" << open_document << "ini" << options->ExportBSON() 
	<< "self_triggger" << DPP << close_document;
    
    // Data field                                                                   
    if(options->GetInt("write_mode")==2)
      obj << "data" << open_array << open_document << "type" << "untriggered" 
	  << "status" << "tranferring" << "location" 
	  << options->GetString("mongo_address") << "collection " << name 
	  << "compressed" << options->GetInt("compression") << close_document 
	  << close_array;

    // Trigger field
    obj << "trigger" << open_document << "mode" 
	<< options->GetString("trigger_mode") << "ended" << false;
    if(options->GetString("trigger_mode") != "ignore")
      obj << "status" <<  "waiting_to_be_processed";
    obj << close_document;

    // Source field   
    string type = options->GetString("source_type");
    obj << "source" << open_document << "type" << type;
    if(type=="LED")
      obj << "frequency" << options->GetInt("pulser_freq");
    obj << close_document;

    // Comment field
    if(comment != "")
      obj << "comments" << open_array << open_document << "text" << comment
	  << "date" << currentTime << "user" << user << close_document 
	  << close_array;

    // Insert the run doc
    InsertOnline("runs",fRunsDBName+"."+fRunsCollection,obj);

    // Now create the collection on the buffer DB and index
    if( options->GetInt("write_mode") == 2 ){ // write to mongo

      // Add user and PW back to string
      string connstring = options->GetString("mongo_address");
      if(fBufferUser!="" && fBufferPassword!="")
	connstring = "mongodb://" + fBufferUser + ":" + fBufferPassword + "@" +
	  connstring.substr(10, connstring.size()-10);
      mongocxx::client buffer{connstring};
      
      bsoncxx::builder::stream::document index;
      index << "time" << 1 << "endtime" << 1;
      
      db = buffer[options->GetString("mongo_database")];
      db[options->GetString("mongo_collection")].create_index(index.view(), {});
      
    }
        
    // store OID so you can update the end time
    fLastDocOIDs[iterator.first]=obj.get_oid();
  }
  return 0;   
}


int MasterMongodbConnection::UpdateEndTime(string detector)
/*
  When a run is ended we update the run document to indicate that we are 
  finished writing.
  
  Fields updated: 
  
  "endtimestamp": UTC time of end of run
  "data_taking_ended": bool to indicate reader is done with the run
 */
{
  if(fRunsDB == NULL )
    return -1;
  
  try  {

    for( auto iterator : fLastDocOIDs ){
      cout<<"Searching for detector "<<detector<<" in "<<iterator.first<<endl;
      if( iterator.first != detector && detector != "all")
	continue;
      cout<<"Matched!"<<endl;
      if( !iterator.second.isSet() ){
	cout<<"No OID set for detector "<<iterator.first<<endl;
	if(fLog!=NULL) 
	  fLog->Error("MasterMongodbConnection::UpdateEndTime - Want to stop run but don't have the _id field of the run info doc");
	continue;
      }
      cout<<"OID is set"<<endl;

      // Get the time in GMT
      time_t nowTime;
      time(&nowTime);
      
      // End time
      bsoncxx::builder::stream::document filter_builder, update_builder;
      filter_builder << "_id" << iterator.second;
      update_builder << "end" << nowTime;
      fRunsDB[fRunsCollection].update_one(filter_builder.view(),
					  update_builder.view());
      
      // data field
      bsoncxx::builder::stream::document filter_builder_2, update_builder_2;
      filter_builder_2 << "_id" << iterator.second << "$exists" 
		       << open_document << "data.0.status" << true 
		       << close_document;
      update_builder_2 << "data.0.status" << "transferred";
      fRunsDB[fRunsCollection].update_one(filter_builder_2.view(),
					  update_builder_2.view());

      // Set to a new random oid so we don't update end times twice
      bsoncxx::oid::oid newoid;
      fLastDocOIDs[iterator.first] = newoid;
    }
  }
  return 0;
}

void MasterMongodbConnection::SendLogMessage(string message, int priority)
/*
  Log messages are saved into the database. An enum indicates the priority of the message.
  For messages flagged with KOMESS_WARNING or KOMESS_ERROR an alert is created requiring
  a user to mark the alert as solved before starting a new run.
 */
{      

  time_t currentTime;
  time(&currentTime);

  int ID=-1;
  // For WARNINGS and ERRORS we put an additional entry into the alert DB
  // users then get an immediate alert
  if((priority==KOMESS_WARNING || priority==KOMESS_ERROR) 
     && fMonitorDB!=NULL ){

    // Grab the most recent alert
    optional<bsoncxx::document::view> doc
      = coll.find_one({}, document{}<<"idnum"<<-1<<finalize);

    if(obj.empty())
      ID=0;
    else
      ID = obj["idnum"].get_int32()+1;

    bsoncxx::builder::stream::document alert;
    alert << "idnum" << ID << "priority" << priority << "timestamp" << 
      currentTime << "sender" << "dispatcher" << "message" << message <<
      "addressed" << false;
    InsertOnline("monitor", fMonitorDBName + "." + "alerts", alert);
  }

  stringstream messagestream;
  string savemessage;
  if(priority==KOMESS_WARNING){
    messagestream<<"The dispatcher has issued a warning with ID "
		 <<ID<<" and text: "<<message;
    savemessage=messagestream.str();
  }
  else if(priority == KOMESS_ERROR){
    messagestream<<"The dispatcher has issued an error with ID "
		 <<ID<<" and text: "<<message;
    savemessage=messagestream.str();
  }
  else savemessage=message;

  bsoncxx::builder::stream::document b;
  b << "message" << message << "priority" << priority << "time" << 
    currentTime << "sender" << "dispatcher";
  InsertOnline("log", fLogDBName+".log",b);
  
}

void MasterMongodbConnection::AddRates(koStatusPacket_t *DAQStatus)
/*
  Update the rates in the online db. 
  Idea: combine all rates into one doc to cut down on queries?
  The online.rates collection should be a TTL collection.
*/
{   
  for(unsigned int x = 0; x < DAQStatus->Slaves.size(); x++)  {

      time_t currentTime;
      time(&currentTime);
      if(DAQStatus->Slaves[x].name.empty()){
	fLog->Message("Corrupted slave data");
	continue;
      }

      bsoncxx::builder::stream::document b;
      b << "createdAt" << currentTime << "node" << DAQStatus->Slaves[x].name <<
	"bltrate" << DAQStatus->Slaves[x].Freq << "datarate" << 
	DAQStatus->Slaves[x].Rate << "runmode" << DAQStatus->RunMode <<
	"nboards" << DAQStatus->Slaves[x].nBoards << "timeseconds" <<
	(int)currentTime;
      
      InsertOnline("monitor", fMonitorDBName+".daq_rates",b);
   }     
}

void MasterMongodbConnection::UpdateDAQStatus(koStatusPacket_t* DAQStatus,
					      string detector)
/*
  Insert DAQ status doc. The online.daqstatus should be a TTL collection.
 */
{
   time_t currentTime;
   time(&currentTime);
   
   bsoncxx::builder::stream::document b;
   b << "createdAt" << currentTime << "timeseconds" << (int)currentTime 
     << "detector" << detector << "mode" << DAQStatus->RunMode;
   if(DAQStatus->DAQState==KODAQ_ARMED)
     b << "state" << "Armed";
   else if(DAQStatus->DAQState==KODAQ_RUNNING)
     b << "state" << "Running";
   else if(DAQStatus->DAQState==KODAQ_IDLE)
     b << "state" << "Idle";
   else if(DAQStatus->DAQState==KODAQ_ERROR)
     b << "state" << "Error";
   else
     b << "state" << "Undefined";
   b << "network" << DAQStatus->NetworkUp << "currentRun" << 
     DAQStatus->RunInfo.RunNumber << "startedBy" << DAQStatus->RunInfo.StartedBy;
   
   string datestring = DAQStatus->RunInfo.StartDate;
   if(datestring.size()!=0){
     datestring.pop_back();
     datestring+="+01:00";
   }
   b << "startTime" << datestring << "numSlaves" << (int)DAQStatus->Slaves.size();
   InsertOnline("monitor", fMonitorDBName+".daq_status",b);
}

int MasterMongodbConnection::CheckForCommand(string &command, string &user, 
					     string &comment, string &detector,
					     bool &override, 
					     map<string, koOptions*> &options)
//string &detector,
//					     koOptions &options)
/*
  DAQ commands can be written to the online.daqcommand db. 
  These usually come from the web interface but they can actually 
  come from anywhere. The only commands recognized are 
  "Start" and "Stop". The run mode (for "Start") and comments 
  (for both) are also pulled from the doc.
 */
{
  if(fMonitorDB == NULL )
    return -1;

  // See if something is in the DB
  auto coll = fMonitorDB[fMonitorDBName]["daq_control"];
  if(coll->count("run.daq_control") ==0)
    return -1;
  
  bsoncxx::document::view command;

  // Want only the newest command in case of spam
  auto cursor = coll.find({});
  for (auto&& doc : cursor) 
    command = doc;
  
  // Strip data from the doc
  auto com = command["command"];
  if(com)
    return -1;

  // If this is a run start we can get the run modes
  string modeTPC="", modeMV="";
  bool override = false;
  if( com == "Start" ){
    auto mTPC = command["run_mode_tpc"];
    auto mMV = command["run_mode_mv"];
    auto oride = command["override"];

    if(mTPC || mMV || oride)
      return -1;
    modeTPC = mTPC.get_string();
    modeMV = mMV.get_utf8();
    override = oride.get_bool();
  }
  auto com = command["comment"];
  auto det = command["detector"];
  auto usr = command["user"];
  if(com || det || usr)
    return -1;
  string comment = com.get_utf8();
  string detector = det.get_utf8();
  string user = usr.get_utf8();
  
  cout<<"Run mode TPC: "<<modeTPC<<" "<<b.getStringField("run_mode_tpc")<<endl;
  cout<<"Detector: "<<detector<<endl;

  // Remove commands in buffer. We don't want to get confused
  coll.drop();
  
  // If start then get the modes. This code feels so outdated now.
  if(command=="Start"){
    // Get the run modes
    if( detector == "all" ){
      options["tpc"] = new koOptions();
      options["muon_veto"] = new koOptions();
      if(PullRunMode(modeTPC, *(options["tpc"]))!=0 ||     
	 PullRunMode(modeMV, *(options["muon_veto"])) !=0){
	delete options["tpc"];
	delete options["muon_veto"];
	return -1;
      }
    }
    else{
      options[detector] = new koOptions();
      if(detector=="tpc" && PullRunMode(modeTPC, (*options[detector])) !=0){
	cout<<"Failed to pull tpc run mode "<<modeTPC<<endl;
	delete options[detector];
	return -1;	 
      }
      else if(detector=="muon_veto" &&
	      PullRunMode(modeMV, (*options[detector]))!=0){
	cout<<"Failed to pull mv run mode "<<modeMV<<endl;
	delete options[detector];
	return -1;
      }
      stringstream pp;
       options[detector]->ToStream(&pp);
       cout<<pp.str()<<endl;
    }
  }  
  return 0;
}

void MasterMongodbConnection::SendRunStartReply(int response, string message)
/*
  When starting a run a plausibility check is performed. The result of this 
  check is sent using this function. The web interface will wait for an 
  entry in this DB and notify the user if something goes wrong.
 */
// replyenum : 11 - ack 12 - action 13 - start 19 - done 18 - err
{
  bsoncxx::builder::stream::document reply;
  reply<<"message"<<message<<"replyenum"<<response;
  InsertOnline("monitor", fMonitorDBName+".dispatcherreply",reply);
}

void MasterMongodbConnection::ClearDispatcherReply()
/* 
Clears the dispatcher reply db. Run when starting a new run
 */
{
  if(fMonitorDB == NULL) return;
  auto collection = fMonitorDB[fMonitorDBName]["dispatcherreply"];
  collection.drop();
  return;
}

int MasterMongodbConnection::PullRunMode(string name, koOptions &options)
/*
  Get a run mode from the options DB. Return 0 on success (and set the options
  argument). Return -1 on failure.
 */
{
  if(fMonitorDB == NULL) return -1;
  auto collection = fMonitorDB[fMonitorDBName]["run_modes"];
  if(collection.count() ==0){
    cout<<"No run modes in online db"<<endl;
    return -1;
  }
  
  // Find the doc with this name
  optional<bsoncxx::document::view> doc
    = collection.find_one(document{}<<"name"<<name<<finalize);
  
  if(!doc.empty()){
    options.SetBSON(res);
    return 0;
  }

  cout<<"Empty run mode obj"<<endl;
  return -1;
}

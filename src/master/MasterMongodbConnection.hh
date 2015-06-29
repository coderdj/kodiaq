#ifndef MASTERMONGODBCONNECTION_HH_
#define MASTERMONGODBCONNECTION_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : MasterMongodbConnection.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
// 
// Brief   : Facilitates connection between master and
//           mongodb database in order to update general
//           run information documents.
// 
// ****************************************************

#include <koOptions.hh>
#include <koLogger.hh>
#include <koHelper.hh>
#include "mongo/client/dbclient.h"

class MasterMongodbConnection
{

public: 
   
               MasterMongodbConnection();
  explicit     MasterMongodbConnection(koLogger *Log);
  virtual     ~MasterMongodbConnection();
   
  // 
  // Name    : InsertRunDoc
  // Purpose : Insert a run doc for the new run
  //           Also create the buffer DB with indices on time and module
  // 
  int          InsertRunDoc(string user, string runMode, string name,
			    string comment, string detector, vector<string> detlist,
			    koOptions *options);
  
  int          SetDBs(string logdb, string monitordb, string runsdb);
  //
  // Name    : UpdateEndTime
  // Purpose : Updates run control document with the time the run ended.
  //           Since this field is created by this function this can also be 
  //           seen as confirmation from the DAQ that the user stopped the run.
  //           Also updates end time in mongodb
  // 
  int          UpdateEndTime(string detector);
  
   
  //
  // Name    : SendLogMessage
  // Purpose : Puts a log message into the mongodb log server for backup and
  //           remote viewing
  // 
  void         SendLogMessage(string message, int priority);
  
  //
  // Names    : AddRates
  //            UpdateDAQStatus
  // Purpose  : Update rate and status information in online DB. 
  //
  void         AddRates(koStatusPacket_t *DAQStatus);   
  void         UpdateDAQStatus(koStatusPacket_t *DAQStatus, 
			       string detector="all");

  //
  // Name    : CheckForCommand
  // Purpose : Checks the mongodb command db for remote commands (from web)
  int          CheckForCommand(string &command, string &user,
			       string &comment, string &detector,
			       bool &override, map<string,koOptions*> &options);
  
  //
  // Name    : PullRunMode
  // Purpose : Pulls a run mode file from the mongodb modes DB and saves it 
  //           to the XeDAQOptions object. Returns 0 on success.
  int          PullRunMode(string name, koOptions &options);

  void InsertOnline(string name, string collection,mongo::BSONObj bson);
  void Start(koOptions *options,string user,string comment=""){return;};
  void EndRun(string user,string comment=""){return;};
  int  UpdateNoiseDirectory(string run_name);
  void SendRunStartReply(int response, string message);//, string mode, string comment);

 private:
  koOptions                 *fOptions;
  map <string,mongo::OID>    fLastDocOIDs;
  koLogger                  *fLog;

  mongo::DBClientConnection *fLogDB, *fMonitorDB, *fRunsDB;
};

#endif

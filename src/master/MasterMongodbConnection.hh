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

class MasterMongodbConnection;

struct collection_thread_t
{
  pthread_t thread;
  bool open;
  bool run;
};

struct collection_thread_packet_t
{
  MasterMongodbConnection *mongoconnection;
  mongo_option_t options;
  vector<string> boardList;
  string detector;
  string collection;
};

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
  int          InsertRunDoc(string user, string name,
			    string comment, map<string,koOptions*> options,
			    string collection="DEFAULT");
  
  int          SetDBs(string logdb, string monitordb, string runsdb,
		      string logname, string monitorname, string runsname,
		      string runscollection, string bufferUser, 
		      string bufferPassword);
		      //string user, string password, string dbauth);
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
			       bool &override, map<string,koOptions*> &options,
			       int &expireAfterSeconds);
  
  //
  // Name    : PullRunMode
  // Purpose : Pulls a run mode file from the mongodb modes DB and saves it 
  //           to the XeDAQOptions object. Returns 0 on success.
  int          PullRunMode(string name, koOptions &options);

  void InsertOnline(string name, string collection,mongo::BSONObj bson);
  void Start(koOptions *options,string user,string comment=""){return;};
  void EndRun(string user,string comment=""){return;};
  //int  UpdateNoiseDirectory(string run_name);
  void SendRunStartReply(int response, string message);//, string mode, string comment);
  void ClearDispatcherReply();
  vector<mongo::BSONObj> GetRunQueue();
  void SyncRunQueue(vector<mongo::BSONObj> queue);

  void InsertCommand(mongo::BSONObj b);

  bool IsRunning(string detector);
  static void* CollectionThreadWrapper(void* data);

 private:
  int MakeMongoCollection(mongo_option_t mongo_opts, string collection,
                          vector<string> boardList, int time_cycle=-1);
  map <string, collection_thread_t> m_collectionThreads;

  
  vector<string> GetHashTags(string comment);
  int Connect();

  koOptions                 *fOptions;
  map <string,mongo::OID>    fLastDocOIDs;
  koLogger                  *fLog;
  mongo::ConnectionString    fLogString, fMonitorString, fRunsString;
  mongo::DBClientBase       *fLogDB, *fMonitorDB, *fRunsDB;
  string                     fLogDBName,fMonitorDBName,fRunsDBName,fRunsCollection;
  string                     fBufferUser, fBufferPassword;
};

#endif

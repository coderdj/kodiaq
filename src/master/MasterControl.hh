/*
 *
 * File: MasterControl.hh
 * Author: Daniel Coderre
 * Date: 15.07.2015
 *
 * Brief: General control class for dispatcher
 *
*/

#ifndef _MASTERCONTROL_HH_
#define _MASTERCONTROL_HH_

#include <string>
#include <map>
#include <koLogger.hh>
#include "DAQMonitor.hh"
#include "MasterMongodbConnection.hh"

class MasterControl{

public:
  MasterControl();
  ~MasterControl();

  int Initialize(string filepath);

  vector<string> GetDetectorList();

  void Connect(string detector="all");
  void Disconnect(string detector="all");
  void Close();

  int  Start(string detector, string user, string comment,
	     map<string,koOptions*> options, bool web, int expireAfterSeconds);
  void Stop(string detector="all", string user="dispatcher_console", 
	    string comment="");

  void CheckRemoteCommand();
  void CheckRunQueue();
  void StatusUpdate();

  string GetStatusString();
  int ModifyRunQueue(string detector, int index=-1);

private:
  vector<mongo::BSONObj> fDAQQueue;
  map<string, time_t> fStartTimes;
  map<string, time_t> fExpireAfterSeconds;

  map<string, DAQMonitor*> mDetectors;
  //map<string, *koNetServer> mMonitors;
  MasterMongodbConnection *mMongoDB;
  koLogger *mLog;

  map<string,time_t> mStatusUpdateTimes, mRatesUpdateTimes;
};

#endif

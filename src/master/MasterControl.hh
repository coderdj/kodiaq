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
	     map<string,koOptions*> options, bool web);
  void Stop(string detector="all", string user="dispatcher_console", 
	    string comment="");

  void CheckRemoteCommand();
  void StatusUpdate();

  string GetStatusString();

private:
  map<string, DAQMonitor*> mDetectors;
  //map<string, *koNetServer> mMonitors;
  MasterMongodbConnection *mMongoDB;
  koLogger *mLog;

  map<string,time_t> mStatusUpdateTimes, mRatesUpdateTimes;
};

#endif

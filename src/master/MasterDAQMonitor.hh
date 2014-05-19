#ifndef MASTERDAQMONITOR_HH_
#define MASTERDAQMONITOR_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 16.05.2014
// File    : MasterDAQMonitor.hh
// 
// Brief   : Class for managing DAQ for dispatcher
// 
// ****************************************************

#include <koLogger.hh>
#include <koNetServer.hh>
#include "MasterMongodbConnection.hh"


class MasterDAQMonitor
{
 public:
   MasterDAQMonitor();
   virtual ~MasterDAQMonitor();
   MasterDAQMonitor(koNetServer *DAQNetwork, koLogger *logger, 
		    MasterMongodbConnection *mongodb);
   
   void ProcessCommand(string command, string second="");
   void ProcessWebCommand(string command);
   
   vector<string> GetUserLog(int maxLines);
   koRunInfo_t GetRunInfo();
   koStatusPacket_t GetStatus();
   
   bool UpdateReady();
  
   
 private:
   
   //DAQ Control
   void StartDAQ();
   void StopDAQ(); //should also shutdown
   int  ArmDAQ();
   void ReconnectDAQNetwork();
   void DisconnectDAQNetwork();
   
   koStatusPacket_t m_DAQStatus;
   koRunInfo_t      m_RunInfo;
   
   koNetServer             *m_DAQNetwork;
   koLogger                *m_Log; //local log
   MasterMongodbConnection *m_Mongodb;
   
 
};

#endif
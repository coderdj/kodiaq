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

// Classification for error messages from slaves 
#define DAQMASTER_WARNING           0      // DAQ can continue run, log this 
#define DAQMASTER_ERR_MINOR         1      // DAQ can continue run, notify someone
#define DAQMASTER_ERR_CRITICAL      2      // Stop the DAQ, notify someone
#define DAQMASTER_ERR_WORLDENDING   3      // Stop the DAQ, notify someone, panic

class MasterDAQMonitor
{
public:
  MasterDAQMonitor();
  virtual ~MasterDAQMonitor();
  MasterDAQMonitor(koNetServer *DAQNetwork, koLogger *logger, 
		   MasterMongodbConnection *mongodb);
  
  void ProcessWebCommand(string command);
  
  bool UpdateReady();
  bool CheckError(int ERRNO, string ERRTXT);
  const koStatusPacket_t GetStatus(){
    return m_DAQStatus;
  };
  
  void Connect();          //Bring up DAQ network 
  void Disconnect();       //Put down DAQ network
  void Stop(string user, string comment="");
  void Start(string  mode, string user, string comment="");
  
private:
  
  int                      ArmDAQ(string mode);
  int                      ShutdownDAQ();

  koStatusPacket_t         m_DAQStatus;
  koRunInfo_t              m_RunInfo;
  
  koNetServer             *m_DAQNetwork;
  koLogger                *m_Log; //local log
  MasterMongodbConnection *m_Mongodb;
  
  int                      m_iErrorFlag;
  string                   m_sErrorText;
};

#endif

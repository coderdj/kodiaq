#ifndef DAQMONITOR_HH_
#define DAQMONITOR_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 16.05.2014
// File    : DAQMonitor.hh
// 
// Brief   : Class for managing DAQ for dispatcher
// 
// ****************************************************

#include <koLogger.hh>
#include <koNetServer.hh>
#include <koOptions.hh>
#include <pthread.h>
#include "MasterMongodbConnection.hh"

// Classification for error messages from slaves 
#define DAQMASTER_WARNING           0      // DAQ can continue run, log this 
#define DAQMASTER_ERR_MINOR         1      // DAQ can continue run, notify someone
#define DAQMASTER_ERR_CRITICAL      2      // Stop the DAQ, notify someone
#define DAQMASTER_ERR_WORLDENDING   3      // Stop the DAQ, notify someone, panic

class DAQMonitor
{
public:
  DAQMonitor();
  virtual ~DAQMonitor();
  DAQMonitor(koNetServer *DAQNetwork, koLogger *logger, 
		   MasterMongodbConnection *mongodb);
  
  void ProcessCommand(string command,string user, 
		      string comment="", koOptions *options=NULL);
  
  bool UpdateReady(){
    return m_bReady;
  };
  bool CheckError(int ERRNO, string ERRTXT);
  const koStatusPacket_t GetStatus(){    
    m_bReady=false;
    return m_DAQStatus;
  };

  void PollNetwork();
  static void* UpdateThreadWrapper(void* monitor);
    
private:
  
  int                      Connect();
  int                      Disconnect();
  int                      Arm(koOptions* mode);
  int                      Start(string user, string comment,koOptions *options);
  int                      Stop(string user, string comment);
  int                      Shutdown();

  koStatusPacket_t         m_DAQStatus;
  koRunInfo_t              m_RunInfo;
  
  koNetServer             *m_DAQNetwork;
  koLogger                *m_Log; //local log
  MasterMongodbConnection *m_Mongodb;
  
  int                      m_iErrorFlag;
  string                   m_sErrorText;

  pthread_t                m_NetworkUpdateThread;
  pthread_mutex_t          m_DAQStatusMutex;
  bool                     m_bReady;
};

#endif

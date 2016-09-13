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
#ifdef WITH_DDC10
#include <ddc_10.hh>
#endif

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
  //DAQMonitor(const DAQMonitor &rhs);
  DAQMonitor(vector<int> ports, vector<int> dports, vector<string> tags,
	     koLogger *logger, MasterMongodbConnection *mongodb, string detector, 
	     string ini_file="DAQConfig.ini");

  void ProcessCommand(string command,string user, 
		      string comment="", koOptions *options=NULL);
  int  PreProcess(koOptions* mode);
  int  Arm(koOptions* mode, string run_name="");
  int  Start(string run_name, string user, 
	     string comment,koOptions *options);
  bool PreProcessFinished(){ return true; };
  bool UpdateReady(){
    return m_bReady;
  };
  //  bool CheckError(int ERRNO, string ERRTXT);
  void ThrowFatalError(bool killDAQ, string errTxt);
  void ThrowWarning(bool killDAQ, string errTxt);
  int ValidateStartCommand(string user, string comment, 
			   koOptions *options, string &message, string run_name);

  koStatusPacket_t* GetStatus(bool SetReady=false){    
    if(SetReady) m_bReady=false;
    return &m_DAQStatus;
  }; 

  int LockStatus();
  int UnlockStatus();
  
  void PollNetwork();
  static void* UpdateThreadWrapper(void* monitor);
    
private:
  
  int                      Connect();
  int                      Disconnect();
  int                      Stop(string user, string comment);
  int                      Shutdown();

  koStatusPacket_t         m_DAQStatus;
  koRunInfo_t              m_RunInfo;
  
  vector <koNetServer*>    m_DAQNetworks;
  koLogger                *m_Log; //local log
  MasterMongodbConnection *m_Mongodb;
  
  int                      m_iErrorFlag;
  //  int                      m_port, m_dport;
  string                   m_sErrorText;
  string                   m_detector;
  //string                   m_ini_file;

  pthread_t                m_NetworkUpdateThread;
  pthread_mutex_t          m_DAQStatusMutex;
  bool                     m_bReady;
};

#endif

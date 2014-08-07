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

#include "MasterDAQMonitor.hh"

MasterDAQMonitor::MasterDAQMonitor()
{
   m_DAQNetwork=NULL;
   m_Log       =NULL;
   m_Mongodb   =NULL;
   koHelper::InitializeStatus(m_DAQStatus);
   koHelper::InitializeRunInfo(m_RunInfo);
}

MasterDAQMonitor::MasterDAQMonitor(koNetServer *DAQNetwork, koLogger *logger,
				   MasterMongodbConnection *mongodb)
{
   m_DAQNetwork = DAQNetwork;
   m_Log        = logger;
   m_Mongodb    = mongodb;
   koHelper::InitializeStatus(m_DAQStatus);
   kOHelper::InitializeRunInfo(m_RunInfo);
}


MasterDAQMonitor::~MasterDAQMonitor()
{
}

void MasterDAQMonitor::ProcessWebCommand(string command, string user, string mode, string comment)
{
   m_DAQStatus.DAQState = KODAQ_PROCESSING;
   
   switch(command)  {
    case "Start":
      if(ArmDAQ(mode)==0)
	StartDAQ(user,comment);
      else ShutdownDAQ();
      break;
    case "Stop":
      StopDAQ(user,comment);
      break;
    default:
      if(m_Log!=NULL)	{
	 stringstream message;
	 message<<"MasterDAQMonitor::ProcessWebCommand - received command I don't know: "<<command;
	 m_Log->Message(message.str());
      }
      break;
   }   
}

void MasterDAQMonitor::StartDAQ(string user, string comment)
{
   //Check current state. DAQ must be armed
   if(m_DAQStatus.DAQState!=KODAQ_ARMED){
      stringstream message;
      message<<"Received start command from "<<user<<" but DAQ was not armed";
      m_Mongodb->SendLogMessage(message.str(),KOMESS_WARNING);      
      return;
   }
   
   // Get the next run number 
   koHelper::UpdateRunInfo(m_RunInfo,user);
   
}

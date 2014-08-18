// **************************************************** 
//                                                       
// kodiaq Data Acquisition Software                      
//                                                                             
// Author  : Daniel Coderre, LHEP, Universitaet Bern                           
// Date    : 08.07.2014                                                       
// File    : koMaster.cc                                                       
//                                                                             
// Brief   : Updated main program for koMaster controller
//                                                      
// *****************************************************

#include <iostream>
#include <fstream>
#include <kbhit.hh>
#include "DAQMonitor.hh"
#include "MasterMongodbConnection.hh"

string MakeStatusString(DAQMonitor dMonitor);

int main()
{
  // command line arguments later to provide mongodb path
  string monitordb_address="xedaq01";
  string monitorDB = "online";

  //local log used for errors if network goes down
  koLogger LocalLog("log/koMaster.log");
  
  //network
  koNetServer DAQNetwork(&LocalLog);
  DAQNetwork.Initialize(2002,2003,1);
  
  // Mongodb Connection 
  MasterMongodbConnection Mongodb(&LocalLog);

  // DAQ Status Object
  DAQMonitor dMonitor(&DAQNetwork,&LocalLog,&Mongodb);
  time_t fStatusTime = koLogger::GetCurrentTime();
  time_t fPrevTime = koLogger::GetCurrentTime();

  // Allow local commands to start/stop in case web interface fails
  char cCommand = '0';

  cout<<"(c)onnect (d)isconnect (s)tart sto(p)"<<endl;
  //Main loop
  while(cCommand!='q'){    
    sleep(1);
    usleep(100); //prevent high cpu
    
    if(kbhit()) {
      cin.get(cCommand);      
      // Local commands can connect, disconnect, and stop DAQ (start coming later?)
      string commandString = "";
      koOptions runMode;
      if(cCommand=='c')
	commandString = "Connect";
      if(cCommand=='d')
	commandString = "Disconnect";
      if(cCommand=='p')
	commandString = "Stop";
      if(cCommand=='s'){
	commandString = "Start";
	if(runMode.ReadParameterFile("DAQOptions.ini")!=0){
	  cout<<"Error reading DAQOptions.ini"<<endl;
	  cCommand='0';
	  continue;
	}
      }
      string comment = "";      
      
      dMonitor.ProcessCommand(commandString,"dispatcher_console",comment,&runMode);
      cCommand='0';//reset
      
    }

    //Check for commands on web network. Only "start" and "stop" are possible
    string command="",user="", comment="";
    koOptions options;
    if(Mongodb.CheckForCommand(command,user,comment,options)==0){
      dMonitor.ProcessCommand(command,user,comment,&options);
    }
    
    //Get updates from slaves, send updates to monitors
    if(dMonitor.UpdateReady()){
      time_t fCurrentTime = koLogger::GetCurrentTime();
      //keep data socket alive            
      double dtimeStatus = difftime(fCurrentTime,fStatusTime);
      if(dtimeStatus>1.){
	fStatusTime = fCurrentTime;
	Mongodb.UpdateDAQStatus(dMonitor.GetStatus());
      }
      double dtime = difftime(fCurrentTime,fPrevTime);      
      if(dtime>10.){
	fPrevTime = fCurrentTime;
	Mongodb.AddRates(dMonitor.GetStatus());
	Mongodb.UpdateDAQStatus(dMonitor.GetStatus());
      }
    }
    // Print a status to the local console
    cout<<MakeStatusString(dMonitor)<<'\r';
    cout.flush();
  }//end while cCommand!='q'
  
}

string MakeStatusString(DAQMonitor dMonitor){
  stringstream ostream;
  koStatusPacket_t stat = dMonitor.GetStatus();
  ostream<<"Network: ";
  stat.NetworkUp ? ostream<<"up ("<<stat.Slaves.size()<<") " : ostream<<"down ";
  ostream<<"DAQ: ";
  switch(stat.DAQState){
  case KODAQ_IDLE:
    ostream<<"idle ";
    break;
  case KODAQ_ARMED:
    ostream<<"armed ";
    break;
  case KODAQ_RUNNING:
    ostream<<"running ";
    break;
  case KODAQ_MIXED:
    ostream<<"mixed ";
    break;
  case KODAQ_ERROR:
    ostream<<"error ";
    break;
  default:
    ostream<<"unknown ";
    break;
  }
  ostream<<"Mode: "<<stat.RunMode;
  double rate=0.;
  for(unsigned int i=0;i<stat.Slaves.size();i++)
    rate+=stat.Slaves[i].Rate;
  ostream<<" Rate: "<<rate;
  return ostream.str();
}

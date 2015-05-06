// ****************************************************
//
// kodiaq Data Acquisition Software
//
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 08.07.2014
// Update  : 02.12.2014
// File    : koMaster.cc          
// 
// Brief   : Dispatcher program to handle multiple detectors    
//
// *****************************************************

#include <iostream>
#include <fstream>
#include <kbhit.hh>
#include <map>
#include "DAQMonitor.hh"
#include "MasterMongodbConnection.hh"
#include "MasterUI.hh" // has to be after MasterMongodbConnection.hh

// Global DAQ Monitor object map

int ReadIniFile(string filepath, string &monitorDB, string &monitorADDR,
		map<string, koNetServer*> &dNetworks, 
		map<string, DAQMonitor*> &dMonitors, koLogger *LocalLog,
		MasterMongodbConnection *Mongodb);

koStatusPacket_t UpdateDAQStatus( string detector );

struct timepair_t {
  time_t RatesTime;
  time_t StatusTime;
};

int main()
{
  // Declare some objects
  koLogger LocalLog("log/koMaster.log");
  MasterMongodbConnection Mongodb(&LocalLog);
  map <string, timepair_t> StatusTimes;
  map <string, timepair_t> UpdateTimes;

  // Read an ini file
  map <string, koNetServer*> dNetworks;
  map <string, DAQMonitor*> dMonitors;
  string monitorDB   = "none";
  string monitorADDR = "none";
  if( ReadIniFile("MasterConfig.ini", monitorDB, monitorADDR,
		  dNetworks, dMonitors, &LocalLog, &Mongodb) != 0 ) {
    cerr<<"Error reading initialization file MasterConfig.ini! Closing."<<endl;
    return -1;
  }
  for(auto iterator : UpdateTimes) {
    iterator.second.RatesTime = koLogger::GetCurrentTime();
    iterator.second.StatusTime = koLogger::GetCurrentTime();
  }

  // Start NCurses UI
  MasterUI theGUI;
  theGUI.Initialize( false);
  theGUI.StartLoop( &LocalLog, &dMonitors, &dNetworks, &Mongodb );
  return 0;







  // below is outdated and can be removed!



  // Local command interface
  char cCommand = '0';
  
  cout<<"Welcome to the kodiaq dispatcher interface."<<endl;
  cout<<"(c)onnect (d)isconnect (s)tart sto(p)"<<endl;
  
  // Main Loop
  while ( cCommand != 'q' ){
    sleep(1); //prevent high CPU
    
    if ( kbhit() ) {
      cin.get( cCommand );
      string commandString = "";
      koOptions runMode;
      
      if( cCommand == 'c' )
	commandString = "Connect";
      if( cCommand == 'd' )
	commandString = "Disconnect";
      if( cCommand == 'p' )
	commandString = "Stop";
      if( cCommand == 's' ) {
	commandString = "Start";
	if ( runMode.ReadParameterFile( "DAQOptions.ini" ) != 0 ) {
	  cout<<"Error reading DAQOptions.ini. Fix to proceed."<<endl;
	  cCommand = '0';
	  continue;
	}
      }
      
      string comment = "";
      for(auto iter : dMonitors) {
	iter.second->ProcessCommand( commandString,
				      "dispatcher_console",
				      comment,
				      &runMode);
      }
      cCommand = '0';
    } // end if kbhit
    
    // Check for commands on web network. 
    string command="", user="", comment="", detector="";
    koOptions options;
    if(Mongodb.CheckForCommand( command, user, comment, detector, options ) ==0 ){
      for(auto iterator : dMonitors ){
	if ( detector == "all" || detector == iterator.first ) {
	  //send command to this detector
	  iterator.second->ProcessCommand( command, user, comment, &options );
	}
      }
    } 

    // Status update section
    for(auto iter : dMonitors) {
      if( iter.second->UpdateReady() ) {
	time_t CurrentTime = koLogger::GetCurrentTime();
	double dTimeStatus = difftime( CurrentTime, 
				       UpdateTimes[iter.first].StatusTime );
	if ( dTimeStatus > 1. ) { //don't flood DB with updates
	  UpdateTimes[iter.first].StatusTime = CurrentTime;
	  Mongodb.UpdateDAQStatus( iter.second->GetStatus(), 
				   iter.first );
	  
	}
	double dTimeRates = difftime( CurrentTime,
				      UpdateTimes[iter.first].RatesTime);
	if ( dTimeRates > 10. ) { //send rates
	  UpdateTimes[iter.first].RatesTime = CurrentTime;
	  Mongodb.AddRates( iter.second->GetStatus() );
	  Mongodb.UpdateDAQStatus( iter.second->GetStatus(),
				   iter.first );
	}
      }
    } // End status update

  } // end while through main loop
  return 0;
}

int ReadIniFile(
		string filepath, string &monitorDB, string &monitorADDR,
                map<string, koNetServer*> &dNetworks,
                map<string, DAQMonitor*> &dMonitors, koLogger *LocalLog,
		MasterMongodbConnection *Mongodb 
	       ) 
// Reads in an initialization file for the master. The following options
// MUST be provided: MONITOR_DB, MONITOR_ADDR, DETECTOR (min 1)
{
  ifstream inifile;
  inifile.open( filepath.c_str() );
  if( !inifile ) return -1;

  monitorDB   = "";
  monitorADDR = "";  
  vector <string> detnames;
  vector <int>    ports;
  vector <int>    dataports;

  string line;
  while ( !inifile.eof() ){
    getline( inifile, line );
    if ( line[0] == '#' ) continue; //ignore comments
    
    //parse                                                                   
    istringstream iss(line);
    vector<string> words;
    copy(istream_iterator<string>(iss),
	 istream_iterator<string>(),
	 back_inserter<vector<string> >(words));
    if(words.size()<2) continue;
    
    if( words[0] == "MONITOR_DB" )
      monitorDB = words[1];
    else if ( words[0] == "MONITOR_ADDR" )
      monitorADDR = words[1];
    else if ( words[0] == "DETECTOR" ){
      if ( words.size() < 4) continue;
      detnames.push_back( words[1] );
      ports.push_back( koHelper::StringToInt( words[2] ) );
      dataports.push_back( koHelper::StringToInt( words[3] ) );
    }
  }
  inifile.close();
  // Check to make sure all values are filled
  if ( monitorDB == "" || monitorADDR == "" || detnames.size() == 0 )
    return -1;
  
  for ( unsigned int x=0; x<detnames.size(); x++ ){

    // Define the network connection
    koNetServer *Network = new koNetServer(LocalLog);
    Network->Initialize(ports[x], dataports[x], 1);
    dNetworks[detnames[x]] = Network;
      
    // Define the monitor
    DAQMonitor *Monitor = new DAQMonitor(Network, LocalLog, Mongodb, detnames[x]);
    dMonitors[detnames[x]] = Monitor;

  }
  return 0;
}

/*koStatusPacket_t UpdateDAQStatus( string detector ){

  map< string, DAQMonitor* >::iterator it = dMonitors.find( detector ); 
  if( it != dMonitors.end() ){
    //element found;
    
    // this has to go in somehow
    //Mongodb.AddRates( iter.second->GetStatus() );
    //Mongodb.UpdateDAQStatus( iter.second->GetStatus(),
    //iter.first );

    return *(it->second->GetStatus());
  }

}
*/

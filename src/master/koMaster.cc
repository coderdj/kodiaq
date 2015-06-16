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
#include <getopt.h>
#include <map>
#include <koHelper.hh>
#include "DAQMonitor.hh"
#include "MasterMongodbConnection.hh"
#include "MasterUI.hh" // has to be after MasterMongodbConnection.hh

struct db_options_t {
  db_def logdb;
  db_def monitordb;
  db_def runsdb;
};
struct timepair_t {
  time_t RatesTime;
  time_t StatusTime;
};

int ReadIniFile(string filepath, db_options_t db_options,
		map<string, koNetServer*> &dNetworks, 
		map<string, DAQMonitor*> &dMonitors, koLogger *LocalLog, 
		string &default_ini, MasterMongodbConnection *Mongodb);


int main(int argc, char *argv[])
{
  // Get command line options
  string filepath = "MasterConfig.ini";
  int c;
  while(1){
    static struct option long_options[] =
      {
	{"ini_file", required_argument, 0, 'i'},
	{0,0,0,0}
      };
    
    int option_index = 0;
    c = getopt_long(argc, argv, "i:", long_options, &option_index);
    if( c == -1) break;

    switch(c){
    case 'i':
      filepath = optarg;
      break;
    default:
      break;
    }
  }

  std::cout<<"Reading .ini file "<<filepath<<"...";

  // Declare some objects    
  db_options_t db_options; 
  koLogger LocalLog("log/koMaster.log");
  map <string, timepair_t> StatusTimes;
  map <string, timepair_t> UpdateTimes;
  time_t PrintTime = koLogger::GetCurrentTime();
  map <string, koNetServer*> dNetworks;
  map <string, DAQMonitor*> dMonitors;
  string default_ini = "DAQConfig.ini";
  MasterMongodbConnection Mongodb(&LocalLog);

  int ret = ReadIniFile(filepath, db_options, dNetworks, dMonitors, &LocalLog,
			default_ini, &Mongodb);
  if(ret != 0 ){
    cout<<endl<<"Failed to read ini file. Please check that it exists!"<<endl;
    return -1;
  }
  cout<<"Done!"<<endl;

  // Start the MongoDB
  Mongodb.SetDBs( db_options.logdb, db_options.monitordb,
		  db_options.runsdb);
  
  for(auto iterator : UpdateTimes) {
    iterator.second.RatesTime = koLogger::GetCurrentTime();
    iterator.second.StatusTime = koLogger::GetCurrentTime();
  }

  // Initialize detector list
  vector<string> detectors;
  detectors.push_back("All");
  int iCurrentDet = 0;
  for(auto iterator : dNetworks )
    detectors.push_back(iterator.first);

  // Local command interface
  char cCommand = '0';
  
  cout<<"Welcome to the kodiaq dispatcher interface."<<endl;
  cout<<"(c)onnect (q)uit"<<endl;//(d)isconnect (s)tart sto(p)"<<endl;
  
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
      if( cCommand == 'r' ){
	// reload inis
	cout<<"Reloading ini files from config file. Will not reload any other settings!"<<endl;
	// and do it
      }
      if( cCommand == 't' ){
	// toggle current detector
	iCurrentDet++;
	if(iCurrentDet >= (int)detectors.size())
	  iCurrentDet = 0;
	cCommand = '0';
	continue;
      }
      if( cCommand == 'p' )
	commandString = "Stop";
      if( cCommand == 's' ) {
	commandString = "Start";
	string ini = default_ini;
	if( detectors[iCurrentDet]!="All")
	  ini = dMonitors[detectors[iCurrentDet]]->GetIni();
	if ( runMode.ReadParameterFile( ini ) != 0 ) {
	  cout<<"Error reading ini file. Fix to proceed."<<endl;
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

    time_t CurrentTime = koLogger::GetCurrentTime();
    double dTime = difftime( CurrentTime, PrintTime );
    if( dTime > 10. ){
      cout<<endl<<"Update: "<<koLogger::GetTimeString()<<endl;
      cout<<"*********************************************************************"<<endl;
      for( auto iter : dMonitors ){
	cout<<"Detector: "<<iter.second->GetName()<<endl;
	for( unsigned int slave=0; slave<iter.second->GetStatus()->Slaves.size(); 
	     slave++ ){
	  cout<<"     "<<iter.second->GetStatus()->Slaves[slave].name<<": "<<
	    iter.second->GetStatus()->Slaves[slave].Rate<<" MB/s "<<
	    iter.second->GetStatus()->Slaves[slave].Freq<<" BLT/s "<<
	    iter.second->GetStatus()->Slaves[slave].nBoards<<" boards "<<
	    koHelper::GetStatusText(iter.second->GetStatus()->Slaves[slave].status)<<endl;
	}
      }
      cout<<"*********************************************************************"<<endl;
      cout<<"(c)onnect (d)isconnect (s)tart sto(p) (r)eload inis (t)oggle detector"<<endl;
      cout<<"(q)uit             Current detector: "<<detectors[iCurrentDet]<<endl;
      cout<<"*********************************************************************"<<endl;
      PrintTime =  koLogger::GetCurrentTime();
    }

  } // end while through main loop
  return 0;
}

int ReadIniFile(string filepath, db_options_t &db_options,
                map<string, koNetServer*> &dNetworks,
                map<string, DAQMonitor*> &dMonitors, koLogger *LocalLog,
		string &default_ini, MasterMongodbConnection *Mongodb)
// Reads in an initialization file for the master. The following options
// MUST be provided: MONITOR_DB, MONITOR_ADDR, DETECTOR (min 1)
{
  ifstream inifile;
  inifile.open( filepath.c_str() );
  if( !inifile ) return -1;

  vector <string> detnames;
  vector <int>    ports;
  vector <int>    dataports;
  vector <string> inis;
  //reset DB options
  db_options.monitordb.address = db_options.monitordb.db = 
    db_options.monitordb.collection = "";
  db_options.logdb.address = db_options.logdb.db =
    db_options.logdb.collection = "";
  db_options.runsdb.address = db_options.runsdb.db =
    db_options.runsdb.collection = "";


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
    if( words.size() >= 2 && words[0] == "DEFAULT_INI"){
      default_ini = words[1];
      continue;
    }

    if(words.size() < 4) continue;
    
    if( words[0] == "MONITOR_DB"){
      db_options.monitordb.address = words[1];
      db_options.monitordb.db = words[2];
      db_options.monitordb.collection = words[3];
    }    
    else if( words[0] == "LOG_DB"){
      db_options.logdb.address = words[1];
      db_options.logdb.db = words[2];
      db_options.logdb.collection = words[3];
    }
    else if( words[0] == "RUNS_DB"){
      db_options.runsdb.address = words[1];
      db_options.runsdb.db = words[2];
      db_options.runsdb.collection = words[3];
    }
    else if ( words[0] == "DETECTOR" && words.size() >= 5 ){
      detnames.push_back( words[1] );
      ports.push_back( koHelper::StringToInt( words[2] ) );
      dataports.push_back( koHelper::StringToInt( words[3] ) );
      inis.push_back(words[4]);
    }
  }
  inifile.close();
  // Check to make sure all values are filled
  if ( detnames.size() == 0 ){
    cout<<"You didn't define any detectors in your config file. Quitting since nothing to do"<<endl;
    return -1;
  }
  
  for ( unsigned int x=0; x<detnames.size(); x++ ){

    // Define the network connection
    koNetServer *Network = new koNetServer(LocalLog);
    Network->Initialize(ports[x], dataports[x], 1);
    dNetworks[detnames[x]] = Network;
      
    // Define the monitor
    DAQMonitor *Monitor = new DAQMonitor(Network, LocalLog, Mongodb, 
					 detnames[x], inis[x]);
    dMonitors[detnames[x]] = Monitor;

  }
  return 0;
}


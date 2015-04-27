// *************************************************
// 
// DAQ Control for XENON-1T
//
// File      : MasterUI.cc
// Author    : Daniel Coderre, LHEP, Uni Bern
// Date      : 15.04.2015
//
// Brief     : Special control module for koMaster
//
// **************************************************

#include "MasterUI.hh"

MasterUI::MasterUI()
{
}

MasterUI::~MasterUI()
{
}
 
struct timepair_t {
  time_t RatesTime;
  time_t StatusTime;
};

int MasterUI::StartLoop( koLogger *LocalLog,
			 map<string, DAQMonitor*> *dMonitors,
			 map<string, koNetServer*> *dNetworks,
			 MasterMongodbConnection *MongoDB
		       )
{
  
  bool bBreak = false;

  // Need to know if each detector is connected
  //map<string, bool> mbConnected;
  map <string, timepair_t> mUpdateTimes;
  

  if( dMonitors->size() > 1 ){
    mDetectors.push_back("all");
    mIniFiles["all"] = "DAQConfig.ini";
    mLatestStatus["all"] = new koStatusPacket_t;
    koHelper::InitializeStatus(*mLatestStatus["all"]);
  }

  for( auto const &it : (*dMonitors) ){

    mbConnected[ it.first ] = false;
    mDetectors.push_back( it.first );
    timepair_t uTimes;
    uTimes.RatesTime = koLogger::GetCurrentTime();
    uTimes.StatusTime = koLogger::GetCurrentTime();
    mUpdateTimes[ it.first ] = uTimes;
    mCurrentDet = it.first;
    mIniFiles[it.first] = "DAQConfig.ini";
    mLatestStatus[ it.first ] = (it.second->GetStatus());

  }

  if( dMonitors->size() > 1 )
    mCurrentDet = "all";
  Refresh();
  

  while( !bBreak ){
    wchar_t cCommand = 'a';
    string command_string = "";
    koOptions runMode;
    
    if( kbhit() )
      cCommand = getch();
    
    switch( cCommand ){
      
      // Default (skip)
    case 97:
      //      usleep(10);
      break;
      // GUI commands                                                         
    case 56:
      // QUIT                                                                 
      bBreak = true;
      break;
    case KEY_RESIZE:
      // Resize window (always active)                                        
      WindowResize();
      break;
    case KEY_UP:
      // Scroll messages                                                      
      MoveMessagesUp();
      break;
    case KEY_DOWN:
      // Scroll messages                                                      
      MoveMessagesDown();
      break;
    case 53:
      // Move to next detector 
      for( unsigned int x=0; x<mDetectors.size(); x++ ){
	if( mDetectors[x] == mCurrentDet ){
	  if( x != mDetectors.size() - 1 ){
	    mCurrentDet = mDetectors[x+1];
	    Refresh();
	    break;
	  }
	  else {
	    mCurrentDet = mDetectors[0];
	    Refresh();
	    break;
	  }
	}
      }
      break;
      // Networked mode commands                                              
    case 49:
      // If networked mode and not connected, connect                         
      if( ( mCurrentDet != "all" && mbConnected[ mCurrentDet ] == false ) ||
	  ( MapCheckAllFalse( mbConnected ) ) ) {
	// connect current detector                                           
	command_string = "Connect";	  
	AddMessage( "Putting up DAQ interface for detector " + mCurrentDet );

      }
      // Else if not running start                                            
      else if ( ( mCurrentDet != "all" && mbConnected[ mCurrentDet ] == true ) 
		|| MapCheckAllTrue( mbConnected ) ){
	// if not running start                  
	
	if ( runMode.ReadParameterFile( mIniFiles[mCurrentDet] ) != 0 ) {
          AddMessage("Error reading ini file. Fix to proceed.");
          cCommand = '0';	  
          break;
        }
	command_string = "Start";
	AddMessage("Starting DAQ for detector " + mCurrentDet);

      }
      break;
    case 50:
      // Set to stop
      command_string = "Stop";
      break;
    case 52:
      // Change .ini file
      
      InputBox( "Enter new .ini path: ", mIniFiles[mCurrentDet] );
      Refresh();
      break;
    case 55:
      command_string = "Disconnect";
      //mbConnected[ mCurrentDet ] = false;
      break;
    default:
      stringstream Command;
      Command<<"Hit key "<<cCommand;
      AddMessage(Command.str());
      break;
    } //end switch

    string comment = ""; //can add later

    if( command_string != "" ){
      
      if( mCurrentDet == "all" ){
	for(auto iter : (*dMonitors) ) {
	  iter.second->ProcessCommand( command_string,
				       "dispatcher_console",
				       comment,
				       &runMode);
	}
      }
      else
	(*dMonitors)[ mCurrentDet ]->ProcessCommand( command_string,
						  "dispatcher_console",
						  comment,
						  &runMode);
    }

    // Update stuff
    if( MongoDB->IsConnected() )
      mMongoOnline = true;
    else
      mMongoOnline = false;

    // Check Mongodb for commands
    string command="", user="", detector="";
    koOptions options;
    if(MongoDB->CheckForCommand( command, user, 
				comment, detector, options ) ==0 ){
      for(auto iterator : (*dMonitors) ){
        if ( detector == "all" || detector == iterator.first ) {
          //send command to this detector              
          iterator.second->ProcessCommand( command, user, comment, &options );
	  AddMessage("Got command from web: " + command + " for " + iterator.first);
        }
      }
    }

    // Check status
    for( auto iter : (*dMonitors) ){   

      // There is a simple check if the monitor is connected
      // A monitor counts as connected if there is at least one slave

      // This is to check for updates
      if( iter.second->UpdateReady() ) {
        time_t CurrentTime = koLogger::GetCurrentTime();
        double dTimeStatus = difftime( CurrentTime,
                                       mUpdateTimes[iter.first].StatusTime );
        if ( dTimeStatus > 1. ) { //don't flood DB with updates                      
          mUpdateTimes[iter.first].StatusTime = CurrentTime;
          MongoDB->UpdateDAQStatus( iter.second->GetStatus(),
                                   iter.first );
	  mbConnected[ iter.first ] = true;	  
	  //AddMessage(iter.first);
	  //mLatestStatus[ iter.first ] = (iter.second->GetStatus());
	  Refresh();
        }
        double dTimeRates = difftime( CurrentTime,
                                      mUpdateTimes[iter.first].RatesTime);
        if ( dTimeRates > 10. ) { //send rates                                       
          mUpdateTimes[iter.first].RatesTime = CurrentTime;
          MongoDB->AddRates( iter.second->GetStatus() );
          MongoDB->UpdateDAQStatus( iter.second->GetStatus(),
                                   iter.first );
	  Refresh();
        }
      }
      
    } // End status update 
       
    
  }

  delete mLatestStatus["all"];
  return 0;
}


bool MasterUI::MapCheckAllTrue( map<string, bool> themap ){
  bool retval = true;
  for(auto iter : themap) {
    if( iter.first == "all" ) continue;
    if( iter.second == false )
      retval = false;
  }
  return retval;
}

bool MasterUI::MapCheckAllFalse( map<string, bool> themap ){
  bool retval = true;
  for(auto iter : themap) {
    if( iter.first == "all" ) continue;
    if( iter.second == true )
      retval = false;
  }
  return retval;
}

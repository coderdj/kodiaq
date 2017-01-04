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
#include <kbhit.hh>
#include <getopt.h>
#include <koHelper.hh>
#include "MasterControl.hh"

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

  // Set up DAQ control objects and initialize variables
  std::cout<<"Reading .ini file "<<filepath<<"...";
  MasterControl *controller = new MasterControl();
  int ret = controller->Initialize( filepath );
  if(ret != 0 ){
    cout<<endl<<"Failed to read ini file. Please check that it exists!"<<endl;
    return -1;
  }
  vector <string> detectors = controller->GetDetectorList();
  detectors.insert( detectors.begin(), "all");
  int iCurrentDet = 0;
  time_t PrintTime = koLogger::GetCurrentTime();
  time_t UpdateTime = koLogger::GetCurrentTime();

  cout<<"Done!"<<endl;

  // Local command interface
  char cCommand = '0';    
  bool bForceUpdate = true;
  // Main Loop
  while ( cCommand != 'q' ){
    usleep(100); //prevent high CPU
    
    if ( kbhit() ) {
      cin.get( cCommand );
      string commandString = "";
            
      if( cCommand == 'c' ){
	cout<<"Received command 'CONNECT'"<<endl;
	controller->Connect();      
      }
      else if( cCommand == 'i'){
	cout<<"DISCONNECT for TPC"<<endl;
	controller->Disconnect("tpc");
      }
      else if( cCommand == 'o' ){
	cout<<"CONNECT for TPC"<<endl;
	controller->Connect("tpc");
      }
      else if( cCommand == 'm'){
	cout<<"CONNECT for MUON VETO"<<endl;
	controller->Connect("muon_veto");
      }
      else if( cCommand == 'n'){
        cout<<"DISCONNECT for MUON VETO"<<endl;
        controller->Disconnect("muon_veto");
      }
      else if( cCommand == 'd' ){
	cout<<"Received command 'DISCONNECT'"<<endl;
	controller->Disconnect();
      }
      else if( cCommand == 'r' ){
	// reload inis
	cout<<"Reloading config file "<<filepath<<endl;
	controller->Close();
	if(controller->Initialize(filepath)!=0){
	  cout<<"Bad input file. No master contoller. Quitting."<<endl;
	  return -1;
	}
	bForceUpdate = true;
      }
      else if( cCommand == 't' ){
	// toggle current detector
	iCurrentDet++;
	if(iCurrentDet >= (int)detectors.size())
	  iCurrentDet = 0;
	cCommand = '0';
	bForceUpdate = true;
	continue;
      }
      else if( cCommand == 'p' ){
	cout<<"Sending stop command"<<endl;
	controller->Stop(detectors[iCurrentDet]);
      }
      else if( cCommand == 's' ) {
	cout<<"Staring from command line currently disabled."<<endl;/*
	cout<<"Sending start command"<<endl;
	if(controller.Start(detectors[iCurrentDet])==-1){
	  controller.Stop(detectors[iCurrentDet]);
	}*/
      }
      else if(cCommand == 'q')
	break;
      cCommand = '0';
    } // end if kbhit
    
    
    // Print update
    time_t CurrentTime = koLogger::GetCurrentTime();
    double dTime = difftime( CurrentTime, PrintTime );
    double dTime2 = difftime( CurrentTime, UpdateTime);
    if(dTime2 > 2.){

      // Check the run queue       
      controller->CheckRunQueue();
      
      // Check for commands on web network.  
      controller->CheckRemoteCommand();

      // Update data objects        
      controller->StatusUpdate();

      UpdateTime = koLogger::GetCurrentTime();
    }
    if( dTime > 9. || bForceUpdate){
      if(bForceUpdate)
	bForceUpdate = false;
      cout<<"Controlling detector: "<<detectors[iCurrentDet]<<endl;
      cout<<controller->GetStatusString()<<endl;
      PrintTime =  koLogger::GetCurrentTime();
    }

  } // end while through main loop

  cout<<"Exiting."<<endl;
  return 0;
}



// ****************************************************
// 
// kodiaq - DAQ Control for Xenon-1T
// 
// File     : koSlave.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 28.10.2013
// 
// Brief    : Network-controlled DAQ driver
// 
// *****************************************************

#include <koLogger.hh>
#include <iostream>
#include <iomanip>
#include <koNetClient.hh>
#include "DigiInterface.hh"
#include <config.h>

//If using the light version we need user input functions
#ifdef KLITE
#include <kbhit.hh>
#endif

using namespace std;


#ifdef KLITE
// *******************************************************************************
// 
//           STANDALONE CODE FOR KODIAQ_LITE, SINGLE-INSTANCE DAQ
//           
// *******************************************************************************

int StandaloneMain()
{   
   koLogger fLog("log/slave.log");
   fLog.Message("Started standalone reader");
   DigiInterface *fElectronics = new DigiInterface(&fLog);
   koOptions fDAQOptions;
   string fOptionsPath = "DAQConfig.ini";
      
program_start:
   char input='a';
   cout<<"Welcome to kodiaq lite! Press 's' to start the run, 'q' to quit."<<endl;
   cout<<"To arm over and over again (5 seconds in between) press 'l'"<<endl;
   while(!kbhit())
     usleep(100);
   cin.get(input);
   if(input=='q') return 0;
   else if(input=='l')  {
      if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)
	return -1;
      int n=0;
      while(input!='q')	{
	 if(fElectronics->Initialize(&fDAQOptions)==0)
	   cout<<"Initialized! That was number "<<n<<"! (q-quit)"<<endl;
	 else{
	    cout<<"Initialization FAILED!!!!"<<endl;
//	    n=-1;
	   return -1;
	 }	 
	 n++;
	 int counter=0;
	 while(!kbhit() && counter<1000)  {
	    usleep(1000);
	    counter++;
	 }
	 if(counter<1000)
	   cin.get(input);	        
      }      
      return -1;
   }   
   else if(input!='s')  goto program_start;
   
   //load options
   cout<<"Reading options . . .";
   if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)   {	
      cout<<"Error reading options file!"<<endl;
      return 1;
   }   
   cout<<" done!"<<endl;
   //start digi interface
   cout<<"Initializing electronics."<<endl;
   if(fElectronics->Initialize(&fDAQOptions)!=0)  {	
      cout<<"Error initializing electronics"<<endl;
      return 1;
   }   
   cout<<"Finished initializing electronics."<<endl;
   //start run
   if(fElectronics->StartRun()!=0)    {	
      cout<<"Error starting run."<<endl;
      return 1;
   }
   cout<<koLogger::GetTimeString()<<" Start of run."<<endl;
   
   input='a';
   time_t prevTime = koLogger::GetCurrentTime();
   while(1)  {	
      if(kbhit()) cin.get(input);
      if(input=='q') break;
      time_t currentTime = koLogger::GetCurrentTime();
      
      //updates every second
      time_t tdiff;
      if((tdiff = difftime(currentTime,prevTime))>=1.0)  {	 
	 prevTime=currentTime;
	 unsigned int iFreq=0;
	 unsigned int iRate = fElectronics->GetRate(iFreq);
	 double rate = (double)iRate;
	 double freq = (double)iFreq;
	 rate = rate/tdiff;
	 rate/=1048576;
	 freq=freq/tdiff;
	 
	 cout<<setprecision(2)<<"Rate: "<<rate<<"MB/s   Freq: "<<freq<<"Hz             "<<'\r';//   Averaged over "<<tdiff<<"s"<<'\r';//endl;
	 cout.flush();
      }      
   }      
   cout<<koLogger::GetTimeString()<<" End of run."<<endl;
   
   return 0;
}

#endif     

// *******************************************************************************
// 
//           NETWORKED DAQ SLAVE CODE
//           
// *******************************************************************************
int main()
{
#ifdef KLITE
   return StandaloneMain();
#endif
         
   //Set up objects
   koLogger      *koLog = new koLogger("log/slave.log");
   koNetClient    fNetworkInterface(koLog);
   fNetworkInterface.Initialize("xedaq02",2002,2003,2,"xedaq02");
   DigiInterface  *fElectronics = new DigiInterface(koLog);
   koOptions   fDAQOptions;
   koRunInfo_t    fRunInfo;
   
   string         fOptionsPath = "DAQConfig.ini";
   time_t         fPrevTime = koLogger::GetCurrentTime();
   bool           bArmed=false,bRunning=false,bConnected=false,bERROR=false;
   //
   koLog->Message("Started koSlave module.");
   
   
   //try to connect. this is the idle state where the module will revert to if reset.
connection_loop:
   while(!bConnected)  {
      if(fNetworkInterface.Connect()==0) bConnected=true;      
      sleep(1);
   }
   cout<<"CONNECTED"<<endl;
   
   //While connected listen for commands and broadcast status
   while(bConnected){
      usleep(100);
      //watch for remote commands
      string command="0",sender;
      int id;      
      if(fNetworkInterface.ListenForCommand(command,id,sender)==0)	{
	 if(command=="ARM")  {
	    bArmed=false;
	    bERROR=false;
	    fElectronics->Close();
	    if(fNetworkInterface.ReceiveOptions(fOptionsPath)==0)  {
	       if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)	 {
		  koLog->Error("koSlave - error loading options");
		  fNetworkInterface.SlaveSendMessage("Error loading options!");
		  continue;
	       }
	       int ret;
	       if((ret=fElectronics->Initialize(&fDAQOptions))==0){
		  fNetworkInterface.SlaveSendMessage("Boards armed successfully.");
		  bArmed=true;
	       }	       
	       else{
		  if(ret==-2)
		    bERROR=true;
		  fNetworkInterface.SlaveSendMessage("Error initializing electronics!");
		  koLog->Error("koSlave - error initializing electronics.");		  
		  continue;
	       }	       
	    }
	    else   {
	       koLog->Error("koSlave - error receiving options!");
	       fNetworkInterface.SlaveSendMessage("Error receiving options!");
	       continue;
	    }	    
	 }	 
	 if(command=="DBUPDATE") { //Change the write database. Done in case of dynamic write modes
	    string dbname="none";	    
	    int count=0;
	    while(fNetworkInterface.ListenForCommand(dbname,id,sender)!=0)  {
	       usleep(500);
	       count++;
	       if(count==20)
		 break;
	    }
	    if(count==20)  {
	       koLog->Error("koSlave - error receiving new mongodb collection.");
	       continue;
	    }	    
	    //change the write collection
	    cout<<"Updating db to "<<dbname<<endl;
	    fDAQOptions.UpdateMongodbCollection(dbname);
	    fElectronics->UpdateRecorderCollection(&fDAQOptions);	    	 
	 }	 
	 if(command=="SLEEP")  {
	    if(bRunning) continue;
	    bArmed=false;
	    fElectronics->Close();	    
	 }	 
	 if(command=="START")  {
	    if(!bArmed || bRunning) continue;
	    fElectronics->StartRun();
	    bRunning=true;
	 }
	 if(command=="STOP")  {
	    if(!bRunning) continue;
	    fElectronics->StopRun();
	    bRunning=false;
	 }
	 
	 
      }//end if listen for command      
      
      //Send status
      double tdiff;
      time_t fCurrentTime=koLogger::GetCurrentTime();
      if((tdiff=difftime(fCurrentTime,fPrevTime))>=1.0){
	 fPrevTime=fCurrentTime;
	 int status=KODAQ_IDLE;
	 if(bArmed && !bRunning) status=KODAQ_ARMED;
	 if(bRunning) status=KODAQ_RUNNING;
	 if(bERROR) status=KODAQ_ERROR;
	 double rate=0.,freq=0.,nBoards=fElectronics->GetDigis();
	 unsigned int iFreq=0;	 
	 unsigned int iRate=fElectronics->GetRate(iFreq);
	 rate=(double)iRate;
	 freq=(double)iFreq;
	 rate=rate/tdiff;
	 rate/=1048576;
	 freq=freq/tdiff;
	 cout<<"rate: "<<rate<<" freq: "<<freq<<" iRate: "<<iRate<<" tdiff: "<<tdiff<<endl;
	 if(fNetworkInterface.SendStatusUpdate(status,rate,freq,nBoards)!=0){
	    bConnected=false;
	   koLog->Error("koSlave::main - [ FATAL ERROR ] Could not send status update. Going to idle state!");
	 }	 
      }
      
      
   }   
   //Close everything
   if(bRunning)
     fElectronics->StopRun();
   if(bArmed)
     fElectronics->Close();
   bArmed=false;
   bRunning=false;
   fNetworkInterface.Disconnect();
   bConnected=false;
   goto connection_loop;
   
   delete koLog;
   delete fElectronics;
   return 0;
   
   
}

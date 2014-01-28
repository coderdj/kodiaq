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

#include <XeDAQLogger.hh>
#include <iostream>
#include <XeNetClient.hh>
#include "DigiInterface.hh"

using namespace std;

XeDAQLogger *gLog = new XeDAQLogger("log/koSlave.log");

int main()
{
   gLog->Message("Started koSlave module.");

   //Set up objects
   XeNetClient    fNetworkInterface(gLog);
   fNetworkInterface.Initialize("xedaq02",2002,2003,2,"xedaq02");
   DigiInterface  *fElectronics = new DigiInterface();
   XeDAQOptions   fDAQOptions;
   
   string         fOptionsPath = "DAQConfig.ini";
   time_t         fPrevTime = XeDAQLogger::GetCurrentTime();
   bool           bArmed=false,bRunning=false,bConnected=false;
   //
   
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
	    fElectronics->Close();
	    if(fNetworkInterface.ReceiveOptions(fOptionsPath)==0)  {
	       if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)	 {
		  gLog->Error("koSlave - error loading options");
		  fNetworkInterface.SlaveSendMessage("Error loading options!");
		  continue;
	       }	         		    	       
	       if(fElectronics->Initialize(&fDAQOptions)==0){
		  fNetworkInterface.SlaveSendMessage("Boards armed successfully.");
		  bArmed=true;
	       }	       
	       else{		    
		  fNetworkInterface.SlaveSendMessage("Error initializing electronics!");
		  gLog->Error("koSlave - error initializing electronics.");		  
		  continue;
	       }	       
	    }
	    else   {
	       gLog->Error("koSlave - error receiving options!");
	       fNetworkInterface.SlaveSendMessage("Error receiving options!");
	       continue;
	    }	    
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
      time_t fCurrentTime=XeDAQLogger::GetCurrentTime();
      if((tdiff=difftime(fCurrentTime,fPrevTime))>=1.0){
	 fPrevTime=fCurrentTime;
	 int status=XEDAQ_IDLE;
	 if(bArmed && !bRunning) status=XEDAQ_ARMED;
	 if(bRunning) status=XEDAQ_RUNNING;
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
	   gLog->Error("koSlave::main - [ FATAL ERROR ] Could not send status update. Going to idle state!");
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
   return 0;
   
   
}

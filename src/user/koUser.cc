// ****************************************************
// 
// kodiaq DAQ Software
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.10.2013
// File    : koUser.cc
// 
// Brief   : User interface for kodiaq
// 
// ****************************************************

#include <iostream>
#include "XeNetClient.hh"
#include "XeCursesInterface.hh"

using namespace std;

int main()
{
   //Declare objects
   
   //logger
   XeDAQLogger        fLog("log/koUser.log");
   
   //network
   XeNetClient        fMasterNetwork(&fLog);
   
   //DAQStatus
   XeStatusPacket_t  fDAQStatus;
   XeDAQHelper::InitializeStatus(fDAQStatus);
   
   //Run info (must fetch from network)
   XeRunInfo_t fRunInfo;
   fRunInfo.RunNumber=fRunInfo.StartedBy=fRunInfo.StartDate="";
   
   //UI
   XeCursesInterface  fUI(&fLog);   
   
   //Timer used to periodically send a test message over open ports. If you don't 
   //do this the ports automatically close after some period of inactivity
   time_t fKeepAlive=XeDAQLogger::GetCurrentTime();               
                                                                
   
   //***********************************************************
   //***********************************************************
   // LOGIN
   //***********************************************************
   //***********************************************************

   //Get login info from the UI
   bool repeat=false; 
login_screen: 
   string name,hostname; int port,dataport,uiret=-1;
   string UIMessage;
   if(!repeat) UIMessage="                                ";
   else {
      UIMessage="Failed to connect. Check settings.";
      repeat=false;
   }          
   while((uiret=fUI.GetLoginInfo(name,port,dataport,hostname,UIMessage))!=0){
      if(uiret==-2)
	return 0;
   }   
   
   //put connection information into the local log 
   stringstream logmess;
   logmess<<"Starting DAQ with "<<name<<" "<<hostname<<" "<<port<<" "<<dataport;
   fLog.Message(logmess.str());
   fMasterNetwork.Initialize(hostname,port,dataport,32000,name);
      
   //Connect to the master process. If it fails go back to the login screen
   //a progress bar or something is forseen here
   int timeout=0;
   while(fMasterNetwork.Connect()!=0)  {
      sleep(1);
      timeout++;
      if(timeout==10){ 
	 repeat=true;
	 goto login_screen;
      }      
   }
   vector <string> fHistory;
   while(fMasterNetwork.GetHistory(fHistory,1000)!=0)  {        
      sleep(1);
   }  
   while(fMasterNetwork.GetRunInfoUI(fRunInfo)!=0)  {
      sleep(1);
   }   
   
   fUI.Initialize(&fDAQStatus,&fRunInfo,fHistory,name);   

   
   //**********************************************************************
   //**********************************************************************
   // MAIN LOOP
   //**********************************************************************
   //**********************************************************************
   
   int UI_SCREEN_SHOWING=1; //1-startmenu, 2-mainmenu, 3-runmenu
   
   //Have to declare all variables before switch statement. C++ doesn't seem
   //to live variable initialization within the case: statements.
   unsigned char command='0'; 
   bool bBlockInput=false;    
   string message,tempString;
   vector<string> runModes;
  
   fUI.DrawStartMenu();
   while(command!='q' && command!='Q')   {
      usleep(100); //keep processor use down
      command='0';
      
      //Draw the proper menu
      //
      //MAIN MENU: Connected to master but DAQ not running
      if((fDAQStatus.NetworkUp && UI_SCREEN_SHOWING<=1) || 
	 (fDAQStatus.NetworkUp && fDAQStatus.DAQState!=XEDAQ_RUNNING 
	  && UI_SCREEN_SHOWING==3))  	{	   
	 fUI.DrawMainMenu();
	 fUI.Update();
	 UI_SCREEN_SHOWING=2;
      }      
      //RUN MENU: DAQ is running
      if(fDAQStatus.DAQState==XEDAQ_RUNNING && UI_SCREEN_SHOWING!=3)	{	   
	 fUI.PrintDAQRunScreen();
	 fUI.Update();
	 UI_SCREEN_SHOWING=3;
      }
      //START MENU: Not connected to master
      if(!fDAQStatus.NetworkUp && UI_SCREEN_SHOWING!=1)	{
	 fUI.DrawStartMenu();
	 fUI.Update();
	 UI_SCREEN_SHOWING=1;
      }                              
      //
      
      //Get command from user (if he puts one)
      if(kbhit())	{
	 command=getch();
	 
	 //Local UI commands
	 if((int)command==3)  {//scroll up intercept
	    fUI.NotificationsScroll(false);
	    command='0';
	 }
	 else if((int)command==2)  {//scroll down intercept
	    fUI.NotificationsScroll(true);
	    command='0';
	 }	 	 
	 
      }  
      
      //Network commands
      switch(command)	{	 
       case 'b': //BROADCAST
	 //broadcasts are possible at every stage as long as the network is connected
	 //we will send a message to all UIs and to the DAQ log file
	 message = fUI.BroadcastMessage(name,uiret);
	 UI_SCREEN_SHOWING=-1;
	 if(fMasterNetwork.SendBroadcastMessage(message)!=0)
	   fUI.PrintNotify("Failed to broadcast your message. How's your connection look?");
	 command='0';
	 break;
       case 'c': //CONNECT
	 //send connect command
	 command='0';
	 if(fDAQStatus.NetworkUp) break;
	 fMasterNetwork.SendCommand("CONNECT");
	 break;
       case 'r': //RECONNECT
	 //tell daq to disconnect everything then put network back up (if settings changed)
	 //do this if you reconfigure slaves or something else that requires a complete network
	 //reboot
	 if(!fDAQStatus.NetworkUp || fDAQStatus.DAQState==XEDAQ_RUNNING || 
	    fDAQStatus.DAQState==XEDAQ_ARMED || fDAQStatus.DAQState==XEDAQ_MIXED) {
	    fUI.PrintNotify("You can only mess with the network when the DAQ is in 'IDLE' mode.");
	    break;
	 }	 
	 fUI.PrintNotify("Really reconnect DAQ network? (y/n)");
	 command=getch();
	 if(command!='y')
	   break;
	 fMasterNetwork.SendCommand("RECONNECT");
	 fDAQStatus.Slaves.clear();
	 fDAQStatus.NetworkUp=false;
	 command='0';
	 break;	 
       case 'd': //DISCONNECT
	 //tell the DAQ network to disconnect from all slaves and close the master/slave interface
	 //do this before bringing slaves offline for maintenance
	 if(!fDAQStatus.NetworkUp || fDAQStatus.DAQState==XEDAQ_RUNNING || 
	     fDAQStatus.DAQState==XEDAQ_ARMED || fDAQStatus.DAQState==XEDAQ_MIXED) {
	    fUI.PrintNotify("You can only mess with the DAQ network if the DAQ is in 'IDLE' mode.");
	    break;
	 }	 
	 fUI.PrintNotify("Really disconnect DAQ network? (y/n)");
	 command=getch();
	 if(command!='y')
	   break;
	 fMasterNetwork.SendCommand("DISCONNECT");
	 fDAQStatus.Slaves.clear();
	 fDAQStatus.NetworkUp=false;
	 command='0';	 
	 break;
       case 'm': //CHANGE RUN MODE
	 //requests a list of run modes from the master. Then lets the user pick which mode he 
	 //wants from a menu. The chosen mode is then sent to the DAQ to arm it
	 if(!fDAQStatus.NetworkUp || fDAQStatus.DAQState==XEDAQ_RUNNING || 
	    fDAQStatus.DAQState==XEDAQ_MIXED) break;
	 fMasterNetwork.SendCommand("MODES");
	 runModes.clear();
	 if(fMasterNetwork.GetStringList(runModes)!=0) {
	    fLog.Error("Error getting run mode list from master");
	    fUI.PrintNotify("Error getting run mode list from master.");
	    break;
	 }
	 tempString = fUI.EnterRunModeMenu(runModes);
	 UI_SCREEN_SHOWING=-1;	 
	 
	 fMasterNetwork.SendCommand("ARM");
	 fMasterNetwork.SendCommand(tempString);
	 command='0';
	 break;
       case 'x'://sleep mode
	 if(!fDAQStatus.DAQState==XEDAQ_ARMED || fDAQStatus.DAQState==XEDAQ_RUNNING
	   || fDAQStatus.DAQState==XEDAQ_MIXED) break;
	 fMasterNetwork.SendCommand("SLEEP");
	 command='0';
	 break;
       case 's':
	 if(!fDAQStatus.DAQState==XEDAQ_ARMED || fDAQStatus.DAQState==XEDAQ_RUNNING
	   || fDAQStatus.DAQState==XEDAQ_MIXED) break;
	 fMasterNetwork.SendCommand("START");
	 command='0';
	 break;
       case 'p':
	 if(!fDAQStatus.DAQState==XEDAQ_RUNNING) break;
	 fMasterNetwork.SendCommand("STOP");
	 command='0';
	 break;
       case 'z':
	 fUI.PrintNotify("Really shut down the DAQ? This will stop acquisition and tell slaves to go to idle state.");
	 command=getch();
	 if(command=='y') {
	    fMasterNetwork.SendCommand("STOP");
	    fMasterNetwork.SendCommand("SLEEP");
	 }	 
	 command='0';
	 break;
       case 'q':
	 fUI.PrintNotify("Signing out of kodiaq. This does not impact the DAQ itself!");
	 break;
      }
      
      
      //monitor data socket
      int uSuccess = fMasterNetwork.WatchForUpdates(fDAQStatus);
      if(uSuccess==0)	{
//	 fUI.PrintNotify("uSuccess = 0");
	 if(fDAQStatus.Slaves.size()>0) fDAQStatus.NetworkUp=true;
	 else fDAQStatus.NetworkUp=false;
	 
	 while(fDAQStatus.Messages.size()>0){
	    fUI.PrintNotify(fDAQStatus.Messages[0].message,fDAQStatus.Messages[0].priority);
	    fDAQStatus.Messages.erase(fDAQStatus.Messages.begin(),fDAQStatus.Messages.begin()+1);
	 }
	 fUI.SidebarRefresh();
	 fUI.Update();
      }
      else if(uSuccess==-2)	{
//	 fUI.PrintNotify("uSuccess = -2");
	 goto login_screen;
      }
      
      //watch for timeouts -- TIMEOUT HANDLING SHOULD BE IN MASTER NOT HERE
      time_t currentTime=XeDAQLogger::GetCurrentTime();
      for(unsigned int x=0;x<fDAQStatus.Slaves.size();x++)      	{	       
	 if(difftime(currentTime,fDAQStatus.Slaves[x].lastUpdate)<60.)
	   continue;
	 fDAQStatus.Slaves.erase(fDAQStatus.Slaves.begin()+x,fDAQStatus.Slaves.begin()+x+1);
	 if(fDAQStatus.Slaves.size()==0) fDAQStatus.NetworkUp=false;
	 fUI.SidebarRefresh();
	 fUI.Update();		 
	 //GIVE A WARNING, PROBABLY KILL WHOLE DAQ UNLESS DISCONNECT COMMAND WAS GIVEN 
      }
      if(difftime(currentTime,fKeepAlive)>100.)	{
	 fMasterNetwork.SendCommand("KEEPALIVE");
	 fKeepAlive = XeDAQLogger::GetCurrentTime();
      }
      
      
   }
   fMasterNetwork.Disconnect();   
   fUI.Close();
   endwin();
   return 0;
}

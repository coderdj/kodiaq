// ****************************************************
// 
// kodiak DAQ Software
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
   //temp file output
   ofstream outfile;
   outfile.open("data.dat");
   
   //Declare objects
   //
   //logger
   XeDAQLogger        fLog("log/koUser.log");
   //
   //network
   XeNetClient        fMasterNetwork(&fLog);
   //
   //DAQStatus
   XeStatusPacket_t  fDAQStatus;
   XeDAQHelper::InitializeStatus(fDAQStatus);
   //
   //Run info (must fetch from network)
   XeRunInfo_t fRunInfo;
   fRunInfo.RunNumber=fRunInfo.StartedBy=fRunInfo.StartDate="";
   //
   //UI
   XeCursesInterface  fUI(&fLog);   
   
   //Now connect UI and download info from server
login_screen:
   string name,hostname; int port,dataport,uiret=-1;
   while((uiret=fUI.GetLoginInfo(name,port,dataport,hostname))!=0){
      if(uiret==-2)
	return 0;
   }   
   stringstream logmess;
   logmess<<"Starting DAQ with "<<name<<" "<<hostname<<" "<<port<<" "<<dataport;
   fLog.Message(logmess.str());
   fMasterNetwork.Initialize(hostname,port,dataport,32000,name);   
   //a progress bar or something is forseen here
   int timeout=0;
   while(fMasterNetwork.Connect()!=0)  {
      sleep(1);
      timeout++;
      if(timeout==10) goto login_screen;
   }
   vector <string> fHistory;
   while(fMasterNetwork.GetHistory(fHistory,1000)!=0)  {        
      sleep(1);
   }  
   while(fMasterNetwork.GetRunInfoUI(fRunInfo)!=0)  {
      sleep(1);
   }   
   
   fUI.Initialize(&fDAQStatus,&fRunInfo,fHistory);
//   sleep(5);   
   

   
   int UI_SCREEN_SHOWING=1; //1-startmenu, 2-mainmenu, 3-runmenu
   
   unsigned char command='0';
   bool bBlockInput=false;
   string message,tempString;
   vector<string> runModes;
   //main loop
   fUI.DrawStartMenu();
   while(command!='q' && command!='Q')   {
      usleep(100);
//      fUI.DrawStartMenu();
      
      //Draw the proper menu
      if((fDAQStatus.NetworkUp && UI_SCREEN_SHOWING<=1) || 
	 (fDAQStatus.NetworkUp && !fDAQStatus.Running && UI_SCREEN_SHOWING==3))  	{	   
	 fUI.DrawMainMenu();
	 fUI.Update();
	 UI_SCREEN_SHOWING=2;
      }      
      if(fDAQStatus.Running && UI_SCREEN_SHOWING!=3)	{	   
	 fUI.PrintDAQRunScreen();
	 fUI.Update();
	 UI_SCREEN_SHOWING=3;
      }
      if(!fDAQStatus.NetworkUp && UI_SCREEN_SHOWING!=1)	{
	 fUI.DrawStartMenu();
	 fUI.Update();
	 UI_SCREEN_SHOWING=1;
      }                              
      
      if(kbhit())	{
	 command=getch();
	 if((int)command==3)  {//scroll up
	    fUI.NotificationsScroll(false);
	    command='0';
	 }
	 else if((int)command==2)  {
	    fUI.NotificationsScroll(true);
	    command='0';
	 }	 	 
      }      
      switch(command)	{	 
       case 'b': //BROADCAST
	 //broadcasts are possible at every stage
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
	 if(!fDAQStatus.NetworkUp || fDAQStatus.Running || fDAQStatus.Armed) break;
	 fUI.PrintNotify("Really reconnect DAQ network? (y/n)");
	 command=getch();
	 if(command=='y')  
	   fMasterNetwork.SendCommand("RECONNECT");
	 fDAQStatus.Slaves.clear();
	 fDAQStatus.NetworkUp=false;
	 command='0';
	 break;	 
       case 'd': //DISCONNECT
	 if(!fDAQStatus.NetworkUp || fDAQStatus.Running || fDAQStatus.Armed) break;
	 fUI.PrintNotify("Really disconnect DAQ network? (y/n)");
	 command=getch();
	 if(command=='y') fMasterNetwork.SendCommand("DISCONNECT");
	 fDAQStatus.Slaves.clear();
	 fDAQStatus.NetworkUp=false;
	 command='0';
	 break;
       case 'm': //Change run mode
	 if(!fDAQStatus.NetworkUp || fDAQStatus.Running) break;
	 fMasterNetwork.SendCommand("ARM");
	 runModes.clear();
	 if(fMasterNetwork.GetStringList(runModes)!=0) {
	    fLog.Error("Error getting run mode list from master");
	    fUI.PrintNotify("Error getting run mode list from master.");
	    break;
	 }
	 tempString = fUI.EnterRunModeMenu(runModes);
	 UI_SCREEN_SHOWING=-1;	 
	 fMasterNetwork.SendCommand(tempString);
	 command='0';
	 break;
       case 'x'://sleep mode
	 if(!fDAQStatus.Armed || fDAQStatus.Running) break;
	 fMasterNetwork.SendCommand("SLEEP");
	 command='0';
	 break;
       case 's':
	 if(!fDAQStatus.Armed || fDAQStatus.Running) break;
	 fMasterNetwork.SendCommand("START");
	 command='0';
	 break;
       case 'p':
	 if(!fDAQStatus.Running) break;
	 fMasterNetwork.SendCommand("STOP");
	 command='0';
	 break;
       case 'q':
	 fUI.PrintNotify("Signing out of kodiaq. This does not impact the DAQ itself!");
	 break;
      }
      
      
      //monitor data socket
      if(fMasterNetwork.WatchForUpdates(fDAQStatus)==0)	{
	 if(fDAQStatus.Slaves.size()>0) fDAQStatus.NetworkUp=true;
	 else fDAQStatus.NetworkUp=false;
	 
	 while(fDAQStatus.Messages.size()>0){
	    fUI.PrintNotify(fDAQStatus.Messages[0].message);
	    fDAQStatus.Messages.erase(fDAQStatus.Messages.begin(),fDAQStatus.Messages.begin()+1);	
	 }
	 fUI.SidebarRefresh();
	 fUI.Update();
	 
	 //detlete later
	 double ratetot=0;
	 for(unsigned int x=0;x<fDAQStatus.Slaves.size();x++)
	   ratetot+=fDAQStatus.Slaves[x].Rate;
	 outfile<<ratetot<<endl;
	 
      }
      //watch for timeouts
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
      
      
   }
   fMasterNetwork.Disconnect();   
}

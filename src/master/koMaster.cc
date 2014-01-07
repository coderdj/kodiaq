// ****************************************************
// 
// kodiak DAQ Software
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.10.2013
// File    : kMaster.cc
// 
// Brief   : Main program for kMaster controller
// 
// ****************************************************

#include <iostream>
#include <fstream>
#include <algorithm>

#include "XeDAQOptions.hh"
#include "XeNetServer.hh"

int GetRunModeList(vector <string> &runModeList,vector <string> &runModePaths);

int main()
{
   //*******************************************
   //Declare objects
   //
   //logger
   string             fBroadcastPath="log/koBroadcast.log";
   string             fRunInfoPath = "log/koRunInfo.dat";
   XeDAQLogger        fLog("log/koMaster.log");
   XeDAQLogger        fBroadcastLog(fBroadcastPath);
   //      
   //network
   XeNetServer        fDAQNetwork(&fLog,&fBroadcastLog);
   fDAQNetwork.Initialize(2002,2003,1);
   XeNetServer        fUserNetwork(&fLog,&fBroadcastLog);
   fUserNetwork.Initialize(3000,3001,1);
   if(fUserNetwork.PutUpNetwork()!=0){
      cout<<"putting up network failed."<<endl;
      return -1;
   }   
   //
   //DAQ status packet
   XeStatusPacket_t   fDAQStatus;
   XeDAQHelper::InitializeStatus(fDAQStatus);
   //
   //DAQ run info
   XeRunInfo_t        fRunInfo;
   fRunInfo.RunInfoPath=fRunInfoPath;
   XeDAQHelper::InitializeRunInfo(fRunInfo);
   //
   //Assorted other variables
   char               cCommand='0';
   unsigned int       fNSlaves=0;
   time_t             fPrevTime=XeDAQLogger::GetCurrentTime();
   time_t             fKeepAlive=XeDAQLogger::GetCurrentTime();
   bool               update=false;

   //Options object for reading veto options
   XeDAQOptions       fDAQOptions;
   //********************************************
   
//********************************************
//Main loop
//
//Commands
   while(cCommand!='q')  {
      //Check for new UI connections
      usleep(100);
      int cID;
      string cName;
      
      //*********************************************************
      //*********************************************************
      //*****************  Check for logins *********************
      //*********************************************************
      //*********************************************************
      // This section checks the user network connection pipe for
      // UIs trying to login. If a login is detected the login
      // procedure is carried out and a message is sent informing
      // who just logged into the DAQ.
      if(fUserNetwork.AddConnection(cID,cName)==0)	{
	 cout<<"Start login process for user "<<cName<<"("<<cID<<")."<<endl;
	 if(fUserNetwork.SendFilePartial(cID,fBroadcastPath)!=0)
	   cout<<"Could not send log."<<endl;
	 fUserNetwork.SendRunInfoUI(cID,fRunInfo);
	 stringstream messagestream;
	 messagestream<<"User "<<cName<<"("<<cID<<") has logged into the DAQ.";
	 fUserNetwork.BroadcastMessage(messagestream.str(),XEMESS_BROADCAST);
	 fLog.Message(messagestream.str());
	 cout<<"Login successful for user "<<cName<<"("<<cID<<")."<<endl;
      }            
      
      //***********************************************************
      //***********************************************************
      //************* Check for command on user network ***********
      //***********************************************************
      //***********************************************************
      // This secdion listens for commands coming from the UIs
      // if a command comes, the appropriate action is taken and
      // and other UIs are informed who made the command and 
      // what it was.
      // 
      string command="none";
      string message;
      int id=-1, tempint=0; string sender="nobody";
      vector <string> runModeList;
      vector <string> runModePaths;
      stringstream errstring;
      if(fUserNetwork.ListenForCommand(command,id,sender)==0){

	 if(command=="BROADCAST"){	      
	    if(fUserNetwork.ReceiveBroadcast(id,message)!=0){	    
	       errstring<<"Received broadcast header from "<<sender<<"("<<id<<")"<<"but no message."<<endl;
	       fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);	       	       
	    }
	    else
	      fUserNetwork.BroadcastMessage(message,XEMESS_BROADCAST);	       
	 }
	 else if(command=="CONNECT")  {	 
	    errstring<<"Received connect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);	    
	    if(fDAQNetwork.PutUpNetwork()!=0)  
	      fUserNetwork.BroadcastMessage("An attempt to put up the DAQ network has failed.",XEMESS_BROADCAST);
	    else  {
	       int timer=0;
	       fNSlaves=0;
	       while(timer<10)	 {
		  if(fDAQNetwork.AddConnection(cID,cName)==0)  {
		     errstring.clear();
		     errstring.str(std::string());
		     errstring<<"Connected to slave "<<cName<<"("<<cID<<")";
		     fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
		     fNSlaves++;
		  }		  
		  sleep(1);
		  timer++;
	       }
	       errstring.clear();
	       errstring.str(std::string());
	       errstring<<"DAQ network online. Connected with "<<fNSlaves<<" slaves.";
	       fDAQNetwork.TakeDownNetwork();//takes down connection socket only
	       fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	    }	    	    
	 }
	 else if(command=="RECONNECT")  {
	    errstring<<"Received reconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	    fDAQNetwork.Disconnect();
	    XeDAQHelper::InitializeStatus(fDAQStatus);	    
	    command="CONNECT";
	 }	 
	 else if(command=="DISCONNECT")  {
	    errstring<<"Received disconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	    fDAQNetwork.Disconnect();
	    XeDAQHelper::InitializeStatus(fDAQStatus);	    
	    command='0';
	 }
	 else if(command=="ARM")  {
	    errstring<<"Received arm command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	    fDAQNetwork.SendCommand("SLEEP");//tell DAQ to reset
	    runModeList.clear(); runModePaths.clear();
	    GetRunModeList(runModeList,runModePaths);
	    if(fUserNetwork.SendStringList(id,runModeList)!=0)  {
	       fLog.Error("koMaster - Error sending run mode list.");
	    }	    
	    int timeout=0;
	    while(fUserNetwork.ListenForCommand(command,id,sender)!=0) {
	       usleep(1000);
	       timeout++;
	       if(timeout>=25000) break;
	    }
	    if(timeout>=25000 || command=="TIMEOUT")  {
	       fLog.Error("koMaster - Timed out waiting for user to choose mode.");
	       fUserNetwork.BroadcastMessage("Timed out waiting for user to choose mode. You only get 20 seconds.",XEMESS_BROADCAST);
	    }
	    else {    		 
	       tempint=-1;
	       for(unsigned int x=0;x<runModeList.size();x++)  {
		  if(runModeList[x]==command) tempint=(int)x;
	       }
	       if(tempint==-1)  {
		  fLog.Error("koMaster - Got bad mode index from UI");
		  command='0';
		  continue;
	       }
	       //now send ARM command to slave
	       fDAQNetwork.SendCommand("ARM");
	       if(fDAQNetwork.SendOptions(runModePaths[tempint])!=0)  {
		  fLog.Error("koMaster - Error sending options.");
		  fUserNetwork.BroadcastMessage("DAQ arm attempt failed.",XEMESS_BROADCAST);
		  command='0';
		  continue;
	       }
	       
	       if(fDAQOptions.ReadFileMaster(runModePaths[tempint])!=0){		 
		  fUserNetwork.BroadcastMessage("Error setting veto options. Bad file!",XEMESS_BROADCAST);
		  continue;
	       }
	       cout<<"Got veto options and the address is "<<fDAQOptions.GetVetoOptions().Address<<endl;
	    
	       fDAQStatus.RunMode=command;
	       errstring.flush();
	       errstring<<"Armed DAQ in mode "<<command;
	       fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	       command='0';
	    }
	 }	 
	 else if(command=="START")  {
	    fDAQNetwork.SendCommand("START");
	    errstring<<"Received start command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);	    
	    XeDAQHelper::UpdateRunInfo(fRunInfo,sender);
	 }
	 else if(command=="STOP")  {
	    fDAQNetwork.SendCommand("STOP");
	    errstring<<"Received stop command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	 }
	 else if(command=="SLEEP")  {
	    errstring<<"Received standby command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_BROADCAST);
	    fDAQNetwork.SendCommand("SLEEP");
	    command='0';
	 }	 	 	 	 	 	 
      }
      
      
      //Updates   
      //*************************************************************
      //*************************************************************
      //************* Check for updates on DAQ network **************
      //*************************************************************
      //*************************************************************
      if(fNSlaves>0){	   
	 while(fDAQNetwork.WatchDataPipe(fDAQStatus)==0) update=true;
	 if(update)	{
	    time_t fCurrentTime= XeDAQLogger::GetCurrentTime();
	    if(difftime(fCurrentTime,fPrevTime)>1.0){
	       update=false;
	       fPrevTime=fCurrentTime;
	       if(fUserNetwork.TransmitStatus(fDAQStatus)!=0)   {
		  fLog.Error("Error transmitting status to user net.");
		  //fUserNetwork.BroadcastMessage("I have a status update for you but you won't receive it.",XEMESS_BROADCAST);
	       }	 
	       fDAQStatus.Messages.clear();
	    }	 
	 }	
	 time_t fCurrentTime = XeDAQLogger::GetCurrentTime();
	 if(difftime(fCurrentTime,fKeepAlive)>100.0)  { //keep the main socket alive if inactive for 100 sec
	    fDAQNetwork.SendCommand("KEEPALIVE");
	    fKeepAlive = XeDAQLogger::GetCurrentTime();
	 }
	 
      }
	
      
      //this section should listen to the DAQ data pipe (if connected) 
      //to update the current DAQ status, then send that info on to 
      //and UIs that are connected

   }//end while(command!='q')   
   //********************************************
}//end program

int GetRunModeList(vector <string> &runModeList,vector <string> &runModePaths)
{
   runModeList.clear(); runModePaths.clear();
   ifstream infile;
   infile.open("data/RunModes.ini");
   if(!infile) return -1;
   string line;
   while(!infile.eof()){	
      getline(infile,line);
      istringstream iss(line);
      vector<string> words;
      copy(istream_iterator<string>(iss),
	   istream_iterator<string>(),
	   back_inserter<vector<string> >(words));
      if(words.size()!=2) continue;
      runModeList.push_back(words[0]);
      runModePaths.push_back(words[1]);
   }
   infile.close();
   if(runModeList.size()>0 && runModeList.size()==runModePaths.size())
     return 0;
   return -1;   
}

// ****************************************************
// 
// kodiaq DAQ Software for XENON1T
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.10.2013
// File    : koMaster.cc
// 
// Brief   : Main program for koMaster controller
// 
// ****************************************************

#include <iostream>
#include <fstream>
#include <algorithm>

#include "MasterMongodbConnection.hh"
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
   //Mongodb Connection
   MasterMongodbConnection fMongodb(&fLog);   
   //
   //Assorted other variables
   char               cCommand='0';
   unsigned int       fNSlaves=0;
   time_t             fPrevTime=XeDAQLogger::GetCurrentTime();
   time_t             fKeepAlive=XeDAQLogger::GetCurrentTime();
   bool               update=false;

   //Options object for reading veto and mongo options
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
	 fUserNetwork.BroadcastMessage(messagestream.str(),XEMESS_NORMAL);
	 fLog.Message(messagestream.str());
	 cout<<"Login successful for user "<<cName<<"("<<cID<<")."<<endl;
      }            
      
      //***********************************************************
      //***********************************************************
      //************* Check for command on user network ***********
      //***********************************************************
      //***********************************************************
      // This section listens for commands coming from the UIs
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
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_NORMAL);
	    if(fDAQNetwork.PutUpNetwork()!=0)  
	      fUserNetwork.BroadcastMessage("An attempt to put up the DAQ network has failed.",XEMESS_WARNING);
	    else  {
	       int timer=0;
	       fNSlaves=0;
	       while(timer<10)	 {
		  if(fDAQNetwork.AddConnection(cID,cName)==0)  {
		     errstring.clear();
		     errstring.str(std::string());
		     errstring<<"Connected to slave "<<cName<<"("<<cID<<")";
		     fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_NORMAL);
		     fNSlaves++;
		  }		  
		  sleep(1);
		  timer++;
	       }
	       errstring.clear();
	       errstring.str(std::string());
	       errstring<<"DAQ network online. Connected with "<<fNSlaves<<" slaves.";
	       fDAQNetwork.TakeDownNetwork();//takes down connection socket only
	       fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_STATE);
	    }	    	    
	 }
	 else if(command=="RECONNECT")  {
	    errstring<<"Received reconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_NORMAL);
	    fDAQNetwork.Disconnect();
	    XeDAQHelper::InitializeStatus(fDAQStatus);	    
	    command="CONNECT";
	 }	 
	 else if(command=="DISCONNECT")  {
	    errstring<<"Received disconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_NORMAL);
	    fDAQNetwork.Disconnect();
	    XeDAQHelper::InitializeStatus(fDAQStatus);	    
	    sleep(1);
	    command='0';
	 }
	 else if(command=="MODES"){
	    runModeList.clear();
	    runModePaths.clear();
	    GetRunModeList(runModeList,runModePaths);
	    if(fUserNetwork.SendStringList(id,runModeList)!=0)  {
	       errstring<<"koMaster - Failed fulfilling request for run mode list from "<<sender<<"("<<id<<")";
	       fLog.Error(errstring.str());
	    }
	 }	 
	 else if(command=="ARM")  {
	    errstring<<"Received arm command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_NORMAL);
	    fDAQNetwork.SendCommand("SLEEP");//tell DAQ to reset
	    if(fUserNetwork.ListenForCommand(command,id,sender)!=0) {
	       fLog.Error("koMaster main - Timed out waiting for run mode after ARM command");
	       koMaster.BroadcastMessage("Did not receive run mode after arm command");
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
		  fUserNetwork.BroadcastMessage("DAQ arm attempt failed.",XEMESS_WARNING);
		  command='0';
		  continue;
	       }
	       
	       if(fDAQOptions.ReadFileMaster(runModePaths[tempint])!=0){		 
		  fUserNetwork.BroadcastMessage("Error setting veto options. Bad file!",XEMESS_WARNING);
		  continue;
	       }
	       cout<<"Got veto options and the address is "<<fDAQOptions.GetVetoOptions().Address<<endl;
	    
	       fDAQStatus.RunMode=command;
	       errstring.str(std::string());//flush();
	       errstring<<"Armed DAQ in mode "<<command;
	       fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_STATE);
	       command='0';
	    }
	 }	 
	 else if(command=="START")  {
	    
	    if(fDAQOptions.GetRunOptions().WriteMode==2) {
	       //if we have dynamic run names, tell mongo the new collection name
	       XeDAQHelper::UpdateRunInfo(fRunInfo,sender);	    
	       if(fDAQOptions.GetMongoOptions().DynamicRunNames){		    
		  fDAQOptions.UpdateMongodbCollection(XeDAQHelper::MakeDBName(fRunInfo,fDAQOptions.GetMongoOptions().Collection));
		  stringstream ss;
		  ss<<"Writing to collection "<<fDAQOptions.GetMongoOptions().Collection;
		  fUserNetwork.BroadcastMessage(ss.str(),XEMESS_STATE);
		  fDAQNetwork.SendCommand("DBUPDATE");
		  fDAQNetwork.SendCommand(fDAQOptions.GetMongoOptions().Collection);
	       }	       
	       //send run info to event builder
	       if(fMongodb.Initialize(sender,fDAQStatus.RunMode,&fDAQOptions)!=0)
		 fUserNetwork.BroadcastMessage("Failed to send run info to event builder",XEMESS_WARNING);
	    }
	    
	    //send the actual start command
	    fDAQNetwork.SendCommand("START");   
	    errstring<<"DAQ started by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_STATE);
	 }
	 else if(command=="STOP")  {
	    fDAQNetwork.SendCommand("STOP");
	    if(fMongodb.UpdateEndTime()!=0)
	      fUserNetwork.BroadcastMessage("Failed to send stop time to event builder.",XEMESS_WARNING);
	    errstring<<"DAQ stopped by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_STATE);
	 }
	 else if(command=="SLEEP")  {
	    errstring<<"DAQ put to sleep mode by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),XEMESS_STATE);
	    fDAQNetwork.SendCommand("SLEEP");
	    fDAQStatus.RunMode="None";
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
	    if(difftime(fCurrentTime,fPrevTime)>=1.0){
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

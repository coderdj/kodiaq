
// ****************************************************
// 
// kodiaq Data Acquisition Software
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
#include <koNetServer.hh>

#include "MasterMongodbConnection.hh"

int GetRunModeList(vector <string> &runModeList,vector <string> &runModePaths);

int main()
{
   //*******************************************
   //Declare objects
   //
   //logger
   string             fBroadcastPath="log/koBroadcast.log";
   string             fRunInfoPath = "log/koRunInfo.dat";
   koLogger        fLog("log/koMaster.log");
   koLogger        fBroadcastLog(fBroadcastPath);
   //      
   //network
   koNetServer        fDAQNetwork(&fLog,&fBroadcastLog);
   fDAQNetwork.Initialize(2002,2003,1);
   koNetServer        fUserNetwork(&fLog,&fBroadcastLog);
   fUserNetwork.Initialize(3000,3001,1);
   if(fUserNetwork.PutUpNetwork()!=0){
      cout<<"putting up network failed."<<endl;
      return -1;
   }   
   //
   //DAQ status packet
   koStatusPacket_t   fDAQStatus;
   koHelper::InitializeStatus(fDAQStatus);
   //
   //DAQ run info
   fDAQStatus.RunInfo.RunInfoPath = fRunInfoPath;
   koHelper::InitializeRunInfo(fDAQStatus.RunInfo);
   //
   //Mongodb Connection
   MasterMongodbConnection fMongodb(&fLog);   
   //
   //Assorted other variables
   char               cCommand='0';
   unsigned int       fNSlaves=0;
   time_t             fPrevTimeMain=koLogger::GetCurrentTime();
   time_t             fPrevTimeData=koLogger::GetCurrentTime();
   time_t             fPrevTimeMongo = koLogger::GetCurrentTime();
   bool               update=false;
   bool               remoteCommand=false;
   string mode="", remotecommand="",remotesender="";
   
   //Options object for reading veto and mongo options
   koOptions       fDAQOptions;
   //********************************************
   
//********************************************
//Main loop
//
//Commands
   cout<<"Starting main loop."<<endl;
   while(cCommand!='q')  {
      //Check for new UI connections
      usleep(100);
      int cID;
      string cName, cIP;
      
      //*********************************************************
      //*********************************************************
      //*****************  Check for logins *********************
      //*********************************************************
      //*********************************************************
      // This section checks the user network connection pipe for
      // UIs trying to login. If a login is detected the login
      // procedure is carried out and a message is sent informing
      // who just logged into the DAQ.
      if(fUserNetwork.AddConnection(cID,cName,cIP)==0)	{
	 cout<<"Start login process for user "<<cName<<"("<<cID<<") at "<<cIP<<"."<<endl;
	 if(fUserNetwork.SendFilePartial(cID,fBroadcastPath)!=0)
	   cout<<"Could not send log."<<endl;
	 fUserNetwork.SendRunInfoUI(cID,fDAQStatus.RunInfo);
	 stringstream messagestream;
	 messagestream<<"User "<<cName<<"("<<cID<<") has logged into the DAQ from "<<cIP<<".";
	 fUserNetwork.BroadcastMessage(messagestream.str(),KOMESS_NORMAL);
	 fMongodb.SendLogMessage(messagestream.str(),KOMESS_NORMAL);
	 fLog.Message(messagestream.str());
	 cout<<"Login successful for user "<<cName<<"("<<cID<<") at "<<cIP<<"."<<endl;
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
      vector <string> tempStringList;  //string vectors
      vector <string> tempStringList2; //used for menu choices
      stringstream errstring;
      if(remoteCommand || fUserNetwork.ListenForCommand(command,id,sender)==0){
	 if(remoteCommand)  {
	    id=0;
	    if(remotecommand=="Arm")  {
	       command="START";
	       remotecommand=="";
	       remoteCommand=false;
	       sender=remotesender;
	    }
	    else if(remotecommand=="Start")  {
	       remotecommand="Arm";
	       command="ARM";
	       sender=remotesender;
	       if(fDAQStatus.DAQState==KODAQ_RUNNING)	 {
		  command="nope";
		  remoteCommand=false;
		  remotecommand="";
	       }	       
	    }
	    else if(remotecommand=="Sleep")  {
	       command="SLEEP";
	       remoteCommand=false;
	       sender=remotesender;
	    }	    
	    else if(remotecommand=="Stop"){
	       command="STOP";
	       remotecommand="Sleep";
	       sender=remotesender;
	    }
	    else
	      remoteCommand=false;
	 }
	 
	 
	 //
	 // Command     : BROADCAST
	 // Description : A user has sent a message to be broadcast
	 // Action      : Broadcast the message to all UIs and put it in the log
	 // 
	 if(command=="BROADCAST"){	      
	    if(fUserNetwork.ReceiveBroadcast(id,message)!=0){	    
	       errstring<<"Received broadcast header from "<<sender<<"("<<id<<")"<<"but no message."<<endl;
	       fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_BROADCAST);
	       fMongodb.SendLogMessage(errstring.str(),KOMESS_ERROR);	       
	    }
	    else		 {		 
	       fUserNetwork.BroadcastMessage(message,KOMESS_BROADCAST);
	       fMongodb.SendLogMessage(message,KOMESS_BROADCAST);
	    }	    
	       
	 }
	 //
	 // Command     : CONNECT
	 // Description : A user wants to put up the DAQ network
	 // Action      : Put up the DAQ network interface and wait for connections
	 // 
	 else if(command=="CONNECT")  {	 
	    errstring<<"Received connect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_NORMAL);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_NORMAL);	    
	    if(fDAQNetwork.PutUpNetwork()!=0) {	       
	      fUserNetwork.BroadcastMessage("An attempt to put up the DAQ network has failed.",KOMESS_WARNING);
	       fMongodb.SendLogMessage("An attempt to put up the DAQ network has failed",KOMESS_WARNING);
	    }	    
	    else  {
	       double timer=0.;
	       fNSlaves=0;
	       while(timer<5.)	 { //wait 5 seconds for slaves to connect
		  if(fDAQNetwork.AddConnection(cID,cName,cIP)==0)  {
		     errstring.str(std::string());
		     errstring<<"Connected to slave "<<cName<<"("<<cID<<") at IP "<<cIP<<".";
		     fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_NORMAL);
		     fMongodb.SendLogMessage(errstring.str(),KOMESS_NORMAL);		     
		     fNSlaves++;
		  }		  
		  usleep(1000);
		  timer+=0.001;
	       }
	       errstring.str(std::string());
	       errstring<<"DAQ network online. Connected with "<<fNSlaves<<" slaves.";
	       fDAQNetwork.TakeDownNetwork();//takes down connection socket only 
	                                     //keeps DAQ from listening for new connections
	       fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_STATE);
	       fMongodb.SendLogMessage(errstring.str(),KOMESS_STATE);
	       
	    }	    	    
	 }
	 //
	 // Command     : RECONNECT
	 // Description : Disconnect and reconnect the DAQ network
	 // Action      : Disconnects the DAQ network then sets the CONNECT flag
	 // 
	 else if(command=="RECONNECT")  {
	    errstring<<"Received reconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_NORMAL);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_NORMAL);	    
	    fDAQNetwork.Disconnect();
	    koHelper::InitializeStatus(fDAQStatus);	    
	    command="CONNECT";
	 }
	 //
	 // Command     : DISCONNECT
	 // Description : Take down the DAQ network
	 // Action      : Takes down the DAQ network UI and resets the status
	 // 
	 else if(command=="DISCONNECT")  {
	    errstring<<"Received disconnect command from "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_NORMAL);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_NORMAL);	    
	    fDAQNetwork.Disconnect();
	    koHelper::InitializeStatus(fDAQStatus);	    
	    sleep(1);
	    command='0';
	 }
	 //
	 // Command     : MODES
	 // Description : A user wants to list of available run modes
	 // Action      : Retrieve the run modes list from file and send it to the user
	 // 
	 else if(command=="MODES"){
	    tempStringList.clear();
	    tempStringList2.clear();
	    GetRunModeList(tempStringList,tempStringList2);
	    if(fUserNetwork.SendStringList(id,tempStringList)!=0)  {
	       errstring<<"koMaster - Failed fulfilling request for run mode list from "<<sender<<"("<<id<<")";
	       fLog.Error(errstring.str());
	    }
	    command='0';
	 }
	 //
	 // Command     : ARM
	 // Description : A user has chosen a mode and wants to arm the DAQ
	 // Action      : Check the chosen mode for validity, if it exists transfer the 
	 //               appropriate options file to the slaves and tell them to arm
	 // 
	 else if(command=="ARM")  {
	    string modetoarm="";
	    tempStringList.clear();
	    tempStringList2.clear();
	    GetRunModeList(tempStringList,tempStringList2);
	    errstring<<"Received arm command from "<<sender<<"("<<id<<")";
	    fDAQNetwork.SendCommand("SLEEP");//tell DAQ to reset
	    if(!remoteCommand){		 
	       if(fUserNetwork.ListenForCommand(command,id,sender)!=0) {
		  fLog.Error("koMaster main - Timed out waiting for run mode after ARM command");
		  errstring<<" but didn't receive mode.";
		  fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_WARNING);
		  fMongodb.SendLogMessage(errstring.str(),KOMESS_WARNING);	    
		  continue;
	       }
	       modetoarm=command;
	    }
	    else modetoarm=mode;
	    errstring<<" for mode "<<modetoarm;
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_NORMAL);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_NORMAL);	       
	    tempint=-1;
	    for(unsigned int x=0;x<tempStringList.size();x++)  {
	       if(tempStringList[x]==modetoarm) tempint=(int)x;
	    }
	    if(tempint==-1)  {
	       fLog.Error("koMaster - Got bad mode index from UI");
	       fUserNetwork.BroadcastMessage("Warning - that run mode doesn't seem to exist.",KOMESS_WARNING);
	       command='0';
	       continue;
	    }
	    
	       
	    //now send ARM command to slave
	    fDAQNetwork.SendCommand("ARM");
	    if(fDAQNetwork.SendOptions(tempStringList2[tempint])!=0)  {
	       fLog.Error("koMaster - Error sending options.");
	       fUserNetwork.BroadcastMessage("DAQ arm attempt failed.",KOMESS_WARNING);
	       fMongodb.SendLogMessage("DAQ arm attempt failed.",KOMESS_WARNING);		  
	       command='0';
	       continue;
	    }
	    
	    if(fDAQOptions.ReadParameterFile(tempStringList2[tempint])!=0){		 
	       fUserNetwork.BroadcastMessage("Error setting veto options. Bad file!",KOMESS_WARNING);
	       fMongodb.SendLogMessage("Error setting veto options. Bad file!",KOMESS_WARNING);		  
	       continue;
	    }
#ifdef WITH_DDC10
	    ddc_10 vetoModule;
	    if(fDAQOptions.GetVetoOptions().Initialized) {		    
	       if(vetoModule.Initialize(fDAQOptions.GetVetoOptions())==0)  		    
		 vetoModule.LEDTestFlash(fDAQOptions.GetVetoOptions().IPAddress);
	       else{		       
		  fUserNetwork.BroadcastMessage("Failed to contact ddc10",KOMESS_WARNING);
		  fMongodb.SendLogMessage("Failed to contact ddc10",KOMESS_WARNING);
	       }		  
	    }	       
#endif
	       
	    fDAQStatus.RunMode=modetoarm;
	    errstring.str(std::string());//flush();
	    errstring<<"Armed DAQ in mode "<<modetoarm;
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_STATE);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_STATE);	       
	    command='0';
	 }
      
	 //
	 // Command     : START
	 // Description : A user wants to start the DAQ
	 // Action      : If using mongodb output, define a new collection to write to. 
	 //               Then just tell the DAQ network to send out the start command.
	 // 
	 else if(command=="START")  {
	    
	    koHelper::UpdateRunInfo(fDAQStatus.RunInfo,sender);
	    fUserNetwork.SendRunInfoUI(-1,fDAQStatus.RunInfo);
	    if(fDAQOptions.write_mode==2) {
	       //if we have dynamic run names, tell mongo the new collection name
	       if(fDAQOptions.dynamic_run_names){		    
		  fDAQOptions.mongo_collection=(koHelper::MakeDBName
						(fDAQStatus.RunInfo,
						 fDAQOptions.mongo_collection));
		  stringstream ss;
		  ss<<"Writing to collection "<<fDAQOptions.mongo_collection;
		  fUserNetwork.BroadcastMessage(ss.str(),KOMESS_STATE);
		  fMongodb.SendLogMessage(ss.str(),KOMESS_STATE);		  
		  fDAQNetwork.SendCommand("DBUPDATE");
		  fDAQNetwork.SendCommand(fDAQOptions.mongo_collection);
	       }	       
	       //send run info to event builder
	       if(fMongodb.Initialize(sender,fDAQStatus.RunMode,fDAQStatus.RunInfo.RunNumber,
				      &fDAQOptions)!=0){		    
		  fUserNetwork.BroadcastMessage("Failed to send run info to event builder",KOMESS_WARNING);
		  fMongodb.SendLogMessage("Failed to send run info to event builder",KOMESS_WARNING);		  
	       }	       
	    }
	    else fMongodb.Initialize(sender,fDAQStatus.RunMode,fDAQStatus.RunInfo.RunNumber
				     ,&fDAQOptions,true);
	    //previous line to update online DB
	    
	    //send the actual start command
	    fDAQNetwork.SendCommand("START");   
	    errstring<<"DAQ started by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_STATE);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_STATE);	    
	 }
	 //
	 // Command     : USERS
	 // Description : A user wants a list of who is connected to the master
	 // Action      : Ask the user network interface for a list of users and send it to 
	 //               the requesting UI
	 // 
	 else if(command=="USERS")  { //send a list of users
	    tempStringList.clear();
	    if(fUserNetwork.GetUserList(tempStringList)!=0)
	      tempStringList.push_back("ERROR");
	    fUserNetwork.SendStringList(id,tempStringList);	    
	    command='0';
	 }
	 //
	 // Command     : BOOT
	 // Description : A user wants to boot someone from the DAQ
	 // Action      : Send the booted user's UI a command to tell it that it was booted
	 //               Tell the user net to close the booted user's sockets
	 // 
	 else if(command=="BOOT")  {
	    double timer=0.;
	    while(fUserNetwork.ListenForCommand(command,id,sender)!=0)  {
	       usleep(1000);
	       timer+=0.001;
	       if(timer>=1.) break;
	    }
	    if(timer<1.)  {
	       int bootID = koHelper::StringToInt(command);
	       fUserNetwork.BroadcastMessage("You are being booted from the DAQ UI. Sorry.",
					     KOMESS_ERROR,bootID);
	       fUserNetwork.CloseConnection(bootID);
	    }	    	    
	    command='0';
	 }
	 //
	 // Command     : STOP
	 // Description : A user wants to stop acquisition
	 // Action      : Send the DAQ the stop command
	 // 
	 else if(command=="STOP")  {
	    fDAQNetwork.SendCommand("STOP");
	    if(fDAQOptions.write_mode==2) {		 
	       if(fMongodb.UpdateEndTime()!=0){		    
		  fUserNetwork.BroadcastMessage("Failed to send stop time to event builder.",KOMESS_WARNING);
		  fMongodb.SendLogMessage("Failed to send stop time to event builder.",KOMESS_WARNING);
	       }	       
	    }	  
	    else fMongodb.UpdateEndTime(true);
	    errstring<<"DAQ stopped by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_STATE);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_STATE);	    
	 }
	 //
	 // Command     : SLEEP
	 // Description : A user want to put the DAQ into it's idle state
	 // Action      : Send the DAQ the SLEEP command to reset options and put boards
	 //               out of 'ready' state
	 // 
	 else if(command=="SLEEP")  {
	    errstring<<"DAQ put to sleep mode by "<<sender<<"("<<id<<")";
	    fUserNetwork.BroadcastMessage(errstring.str(),KOMESS_STATE);
	    fMongodb.SendLogMessage(errstring.str(),KOMESS_STATE);	    
	    fDAQNetwork.SendCommand("SLEEP");
	    fDAQStatus.RunMode="None";
	    command='0';
	 }	 	 	 	 	 	 
      }//end command if statement
      
      
      //Updates   
      //*************************************************************
      //*************************************************************
      //************* Check for updates on DAQ network **************
      //*************************************************************
      //*************************************************************
      update=false;      
      if(fNSlaves>0){	   
	 while(fDAQNetwork.WatchDataPipe(fDAQStatus)==0) update=true;
      }
      time_t fCurrentTime = koLogger::GetCurrentTime();
      //keep data socket alive
      double dtime = difftime(fCurrentTime,fPrevTimeMain);
      double dtimemongo = difftime(fCurrentTime,fPrevTimeMongo);
      if((dtime>=1.0 && update) || dtime>=10.) {	   	 
	 update=false;
	 fPrevTimeMain=fCurrentTime;	 
	 if(fUserNetwork.TransmitStatus(fDAQStatus)!=0)   {
	    fLog.Error("Error transmitting status to user net.");
	 }	 
	 fDAQStatus.Messages.clear();
	 	 
	 fMongodb.UpdateDAQStatus(fDAQStatus);	 
	 string string2="",string3="";
	 if(fMongodb.CheckForCommand(remotecommand,string2,string3)==0){
	    mode=string2;
	    remotesender=string3;
	    remoteCommand=true;
	    cout<<"Got remote command "<<remotecommand<<" "<<mode<<" "<<remotesender<<endl;	    
	 }
	 
      }	 
      if(dtimemongo>=10.){
	 fMongodb.AddRates(fDAQStatus);
	 fPrevTimeMongo=fCurrentTime;
      }
      
      //keep main socket alive
      dtime = difftime(fCurrentTime,fPrevTimeData);
      if(dtime>=100.){	   
	 fDAQNetwork.SendCommand("KEEPALIVE");
	 fPrevTimeData = fCurrentTime;
      }
         	
      
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


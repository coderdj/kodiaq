
// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNetServer.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// 
// Brief    : Server-specific class for xenet inferface
// 
// ****************************************************

#include <iostream>
#include "XeNetServer.hh"
#include <errno.h>

XeNetServer::XeNetServer()
{
   fPORT=2000;
   fDATAPORT=2001;
   fNUMCLIENTS=0;
}

XeNetServer::~XeNetServer()
{
}

XeNetServer::XeNetServer(XeDAQLogger *logger, XeDAQLogger *broadcastlogger)
{
   fLog=logger;
   fBroadcastLog=broadcastlogger;
   fPORT=2000;
   fDATAPORT=2001;
   fNUMCLIENTS=0;
}

void XeNetServer::Initialize(int PORT, int DATAPORT, int NUMCLIENTS)
{
   fPORT=PORT;
   fDATAPORT=DATAPORT;
   fNUMCLIENTS=NUMCLIENTS;
}

int XeNetServer::Connect()
{   
   for(int x=0;x<fNUMCLIENTS;x++)  {	
      int id; 
      string name;
      if(AddConnection(id,name)!=0)      {	     
	 fLog->Error("XeNetServer::Connect - Could not add connection.");
	 Disconnect();
	 return -1;
      }	
   }   
   return 0;
}


int XeNetServer::PutUpNetwork()
{
   //Check plausibility of ports
   if(fPORT<=0 || fDATAPORT<=0)  {
      fLog->Error("XeNetServer::Connect - Bad options, check configuration.");
      return -1;
   }
   
   //Declare sockets
   fConnectionSocket = socket(AF_INET,SOCK_STREAM,0);
   if(fConnectionSocket<0)  {
      fLog->Error("XeNetServer::Connect - bad file descriptor for main socket.");
      return -1;
   }   
   fConnectionDataSocket = socket(AF_INET,SOCK_STREAM,0);
   if(fConnectionDataSocket<0)  {
      fLog->Error("XeNetServer::Connect - bad file descriptor for data socket.");
      close(fConnectionSocket);
      return -1;
   }
   //Set option to reuse address in case the OS hasn't cleared it
   int a=1;
   #ifdef SO_REUSEPORT
   if(setsockopt(fConnectionSocket,SOL_SOCKET,SO_REUSEPORT,&a,sizeof(int))<0 ||
      setsockopt(fConnectionDataSocket,SOL_SOCKET,SO_REUSEPORT,&a,sizeof(int))<0)  {
      fLog->Error("XeNetServer::Connect - failed to set REUSEPORT.");
      close(fConnectionSocket);
      close(fConnectionDataSocket);
      return -1;
   }
   #endif
   if(setsockopt(fConnectionSocket,SOL_SOCKET,SO_REUSEADDR,&a,sizeof(int))<0 ||
      setsockopt(fConnectionDataSocket,SOL_SOCKET,SO_REUSEADDR,&a,sizeof(int))<0)  {
      fLog->Error("XeNetServer::Connect - failed to set REUSEADDR.");
      close(fConnectionSocket);
      close(fConnectionDataSocket);
      return -1;
   }

   
   //Set up address structures
   struct sockaddr_in server_addr, dataserver_addr;
   bzero((char*)&server_addr,sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   server_addr.sin_addr.s_addr = INADDR_ANY;
   server_addr.sin_port = htons(fPORT);
   bzero((char*)&dataserver_addr,sizeof(dataserver_addr));
   dataserver_addr.sin_family = AF_INET;
   dataserver_addr.sin_addr.s_addr = INADDR_ANY;
   dataserver_addr.sin_port = htons(fDATAPORT);
   
   //bind sockets
   if(bind(fConnectionSocket,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)  {
      fLog->Error("XeNetServer::Connect - error binding main socket.");
      close(fConnectionSocket);
      close(fConnectionDataSocket);
      return -1;
   }
   if(bind(fConnectionDataSocket,(struct sockaddr*)&dataserver_addr,sizeof(dataserver_addr))<0)    {
      fLog->Error("XeNetServer::Connect - error binding data socket");
      close(fConnectionSocket);
      close(fConnectionDataSocket);
      return -1;
   }
   listen(fConnectionSocket,5);
   cout<<fConnectionDataSocket<<endl;
   return 0;
}

int XeNetServer::TakeDownNetwork()
{
   close(fConnectionSocket);
   close(fConnectionDataSocket);
   return 0;
}

int XeNetServer::AddConnection(int &rid, string &rname)
{   
   //Listen for connections
   struct sockaddr_in client_addr, dataclient_addr;
   unsigned int clientSize = sizeof(client_addr);
   unsigned int dataclientSize = sizeof(dataclient_addr);

   if(MessageOnPipe(fConnectionSocket)!=0) return -1;
   int opensocket = accept(fConnectionSocket,(struct sockaddr*)&client_addr,&clientSize);
   int wait=0;
   listen(fConnectionDataSocket,1);
   while(MessageOnPipe(fConnectionDataSocket)!=0)  {
      sleep(1);
      wait++;
      if(wait>10) {
	 close(opensocket);
	 return -1;
      }      
   }
   
   int opendatasocket = accept(fConnectionDataSocket,(struct sockaddr*)&dataclient_addr,&dataclientSize);
   
   if(opensocket<0 || opendatasocket<0)	{
      stringstream mess;
      fLog->Error("XeNetServer::Connect - failed to accept incoming connections.");
      close(opensocket);
      close(opendatasocket);
      return -1;
   }
      
   int ID,DID;
   if(ReceiveInt(opensocket,ID)!=0 || ReceiveInt(opendatasocket,DID)!=0)	{
      fLog->Error("XeNetServer::Connect - failed to fetch client IDs");
      close(opensocket);
      close(opendatasocket);
      return -1;
   }
   if(ID==32000 || DID==32000)  {
      //Find lowest unused ID
      int tryid=0;
      bool unique=false;
      while(!unique)	{
	 tryid++;
	 unique=true;
	 for(unsigned int x=0;x<fSockets.size();x++){	      
	    if(fSockets[x].id==tryid) 
	      unique=false;
	 }	 
	 for(unsigned int y=0;y<fDataSockets.size();y++){	      
	    if(fDataSockets[y].id==tryid) 
	      unique=false;
	 }	 
      }
      if(SendInt(opensocket,tryid)!=0 || SendInt(opendatasocket,tryid)!=0)	{
	 fLog->Error("XeNetServer::Connect - failed to distribute ID to client.");
	 return -1;
      }
      ID=DID=tryid;      
   }
   
   string name,dname;
   if(ReceiveString(opensocket,name)!=0 || ReceiveString(opendatasocket,dname)!=0)	{
      fLog->Error("XeNetServer::Connect - failed to fetch client names.");
      close(opensocket);
      close(opendatasocket);
      return -1;
   }
   
   XeSocket_t XeS,XeDS;
   XeS.socket=opensocket;
   XeDS.socket=opendatasocket;
   XeS.name  = name;
   XeDS.name = dname;
   XeS.id=ID;
   XeDS.id=DID;
   fSockets.push_back(XeS);
   fDataSockets.push_back(XeDS);

   rid=ID;
   rname=name;
   return 0;      
}

int XeNetServer::Disconnect()
{
   for(unsigned int x=0;x<fSockets.size();x++)
     close(fSockets[x].socket);
   for(unsigned int x=0;x<fDataSockets.size();x++)
     close(fDataSockets[x].socket);
   fSockets.clear();
   fDataSockets.clear();
   TakeDownNetwork();
   return 0;
}

int XeNetServer::CloseConnection(int id, string name)
{
   if(id==-1 && name=="")  {
      fLog->Error("XeNetServer::CloseConnection - told to close a connection without id/name.");
      return -1;
   }
   
   bool sockClosed=false,dataSockClosed=false;
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(fSockets[x].id==id || (id==-1 && fSockets[x].name==name))	{
	 close(fSockets[x].socket);
	 fSockets.erase(fSockets.begin()+x,fSockets.begin()+x+1);
	 sockClosed=true;
      }      
   }
   if(!sockClosed)  {
      fLog->Error("XeNetServer::CloseConnection - failed to find socket to close.");
      return -1;
   }   
   for(unsigned int x=0;x<fDataSockets.size();x++)  {
      if(fDataSockets[x].id==id || (id==-1 && fDataSockets[x].name==name))	{
	 close(fDataSockets[x].socket);
	 fDataSockets.erase(fDataSockets.begin()+x,fDataSockets.begin()+x+1);
	 dataSockClosed=true;
      }      
   }
   if(!dataSockClosed)  {
      fLog->Error("XeNetServer::CloseConnection - failed to find data socket to close");
      return -1;
   }
   stringstream message;
   if(id!=-1) message<<"XeNetServer::CloseConnection - closed connection with id "<<id;
   else message<<"XeNetServer::CloseConnection - closed connection with name "<<name;
   fLog->Message(message.str());
   return 0;      
}

int XeNetServer::WatchDataPipe(XeStatusPacket_t &status)
//Watch the data pipe and update the status while updates and messages come in
//decide the overall status of the DAQ by adding up the status of each slave
{
   int retval=-1;
   for(unsigned int x=0;x<fDataSockets.size();x++)  {
      if(CheckDataSocket(fDataSockets[x].socket,status)==0)
	retval=0;
   }
   if(retval==0)
     XeDAQHelper::ProcessStatus(status);
   return retval;
}

int XeNetServer::BroadcastMessage(string message, int priority)
{
   int retval=0;
   for(unsigned int x=0;x<fDataSockets.size();x++)  {
      if(SendMessage(fDataSockets[x].socket,-1,"master",message,priority)!=0)	{
	 CloseConnection(fDataSockets[x].id);
      }
   }      
   fBroadcastLog->SaveBroadcast(message,priority);
   return retval;
   
}

int XeNetServer::SendCommand(string command)
{
   int retval=0;
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(SendCommandToSocket(fSockets[x].socket,command,-1,"master")!=0){
	 fLog->Error("XeNetServer::SendCommand - error sending command to socket.");
	 retval=-1;
      }      
   }
   return retval;   
}

int XeNetServer::ListenForCommand(string &command,int &id, string &sender)
{
   for(unsigned int x=0;x<fSockets.size();x++)  {
      int a;
      if((a=CommandOnSocket(fSockets[x].socket,command,id,sender))==0)
	return 0;
      else if(a==-2) {//SHOULD LOG THIS, ALSO SHOULD PROBABLY NOT HAPPEN FOR SLAVE?
	 CloseConnection(fSockets[x].id);
	 return -1;
      }      
   }
   return -1;   
}

int XeNetServer::SendFilePartial(int id, string filepath)
{
   string temp;
   int socket=-1;
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(fSockets[x].id==id) socket=fSockets[x].socket;
   }
   if(socket==-1)  {
      fLog->Error("XeNetServer::SendFilePartial - id not found.");
      return -1;
   }   
   
   int timer=0;
   while(MessageOnPipe(socket)!=0) {
      sleep(1);
      timer++;
      if(timer==10){	   
	 fLog->Error("XeNetServer::SendFilePartial - No message on pipe.");
	 return -1;
      }      
   }   
   if(ReceiveString(socket,temp)!=0 || temp!="HISTORY") {
      fLog->Error("XeNetServer::SendFilePartial - No or incorrect header received.");
      return -1;
   }   
   int nLines=-1;
   if(ReceiveInt(socket,nLines)!=0 || nLines<0) {
      fLog->Error("XeNetServer::SendFilePartial - No or incorrect line count received.");
      return -1;
   }   
   vector <string> fileBuff;
   ifstream infile;
   infile.open(filepath.c_str());
   if(!infile) {
      fLog->Error("XeNetServer::SendFilePartial - no file found.");
      return -1;
   }   
   while(!infile.eof())  {
      getline(infile,temp);
      fileBuff.push_back(temp);
      if((int)fileBuff.size()>nLines) fileBuff.erase(fileBuff.begin(),fileBuff.begin()+1);
   }
   temp="HISTORYEND";   
   //if(fileBuff.size()==0) return -1;
   for(unsigned int x=0;x<fileBuff.size();x++){	
      if(SendString(socket,fileBuff[x])!=0) {
	 fLog->Error("XeNetServer::SendFilePartial - Error sending line of file.");
	 return -1;
      }
   }           
   if(SendString(socket,temp)!=0) {
      fLog->Error("XeNetServer::SendFilePartial - Error sending footer.");
      return -1;
   }   
   return 0;
   
}

int XeNetServer::SendRunInfoUI(int id, XeRunInfo_t runInfo)
{
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(fSockets[x].id==id && SendRunInfo(fSockets[x].socket,runInfo)==0)
	return 0;
   }
   return -1;
}

int XeNetServer::ReceiveBroadcast(int id, string &broadcast)
{
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(fSockets[x].id==id)	{
	 if(ReceiveString(fSockets[x].socket,broadcast)!=0) return -1;
	 string temp;
	 if(ReceiveString(fSockets[x].socket,temp)!=0 || temp!="ENDBROADCAST") return -1;
	 return 0;
      }      
   }
   return -1;
}

int XeNetServer::TransmitStatus(XeStatusPacket_t status)
{
   int success=0;   
   for(unsigned int x=0;x<fDataSockets.size();x++)  {
      SendRunMode(fDataSockets[x].socket,status.RunMode);
      for(unsigned int y=0;y<status.Slaves.size();y++)	{
	 int id = status.Slaves[y].ID;
	 string name = status.Slaves[y].name;
	 int stat = status.Slaves[y].status;
	 double rate = status.Slaves[y].Rate;
	 double freq = status.Slaves[y].Freq;
	 int nboards = status.Slaves[y].nBoards;
	 if(SendUpdate(fDataSockets[x].socket,id,name,stat,rate,freq,nboards)!=0)
	   success=-1;
      }      
   }
   for(unsigned int x=0;x<status.Messages.size();x++)  {
      for(unsigned int y=0;y<fDataSockets.size();y++)	{
	 SendMessage(fDataSockets[x].socket,-1,"Master",status.Messages[x].message,
		     status.Messages[x].priority);
      }      
   }
   
   return success;
}

int XeNetServer::SendStringList(int id, vector<string> stringList)
{
   int success=-1;
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(fSockets[x].id!=id) continue;
      for(unsigned int y=0;y<stringList.size();y++){	   
	 if(SendString(fSockets[x].socket,stringList[y])!=0)
	   break;
      }      
      string ender = "@END@";
      if(SendString(fSockets[x].socket,ender)!=0) break;
      success=0;
   }
   return success;
}

int XeNetServer::SendOptions(string filepath)
{
   int retval=0;
   for(unsigned int x=0;x<fSockets.size();x++)  {
      if(SendFile(fSockets[x].socket,fSockets[x].id,filepath)!=0)
	retval=-1;
   }
   return retval;
}

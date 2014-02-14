// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNetClient.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// 
// Brief    : Client-specific class for xenet inferface
// 
// ****************************************************

#include "XeNetClient.hh"

XeNetClient::XeNetClient()
{
   fHOSTNAME="localhost";
   fPORT=2000;
   fDATAPORT=2001;
   fID=0;
   fMyName="default";
}
XeNetClient::~XeNetClient()
{
}
XeNetClient::XeNetClient(XeDAQLogger *logger)
{
   fLog=logger;
   //default values
   fHOSTNAME="localhost";
   fPORT=2000;
   fDATAPORT=2001;
   fID=0;
   fMyName="default";
}

void XeNetClient::Initialize(string HOSTNAME, int PORT, int DATAPORT, int ID, string MyName)
{
   fHOSTNAME=HOSTNAME;
   fPORT    =PORT;
   fDATAPORT=DATAPORT;
   fID      =ID;
   fMyName  =MyName;
}

int XeNetClient::Connect()
{  
   
   //Check provided parameters to make sure they are valid
   if(fPORT<=0 || fDATAPORT<=0)  {
      fLog->Error("XeNetClient::Connect - invalid port or data port. Check options.");
      return -1;
   }
   struct hostent *server;
   server = gethostbyname(fHOSTNAME.c_str());
   if(server==NULL)  {
      fLog->Error("XeNetClient::Connect - failed to get server hostname.");
   }
   
   //Define socket
   int sock = socket(AF_INET,SOCK_STREAM,0);  //IPv4 only for now
   if(sock<0)  {
      fLog->Error("XeNetClient::Connect - Error creating socket");
      return -1;
   }
   
   //define server address struct
   struct sockaddr_in server_addr;
   bzero((char*)&server_addr,sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr,server->h_length);
   server_addr.sin_port = htons(fPORT);
   
   //connect main socket
   int a;
   if((a=connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0)  {
      close(sock);
//      fLog->Error("XeNetClient::Connect - failed to connect main socket.");
      return -1;
   }
   fLog->Message("XeNetClient::Connect - connected main socket.");
   
   //Define data socket
   int datasock = socket(AF_INET,SOCK_STREAM,0);
   if(datasock<0)  {
      fLog->Error("XeNetClient::Connect - bad data socket file descriptor");
      close(sock);
      return -1;
   }
   
   //Set server address struct for data
   struct sockaddr_in dataserveraddr;
   bzero((char*)&dataserveraddr,sizeof(dataserveraddr));
   dataserveraddr.sin_family = AF_INET;
   bcopy((char*)server->h_addr,(char*)&dataserveraddr.sin_addr.s_addr,server->h_length);
   dataserveraddr.sin_port = htons(fDATAPORT);

   //connect data socket
   int timer=0;

   int b;
   while((b=connect(datasock,(struct sockaddr*)&dataserveraddr,sizeof(dataserveraddr)))<0)  {
      timer++;
      sleep(1);
      if(timer==20){	   
	 fLog->Error("XeNetClient::Connect - failed to connect data socket.");
	 close(sock);
	 close(datasock);
	 return -1;
      }      
   }
   stringstream logmess;
   logmess<<"Connected to data socket with "<<b<<" and socket with "<<a;
   fLog->Message(logmess.str());
   
   //Sockets should identify themselves
   if(SendInt(sock,fID)!=0 || SendInt(datasock,fID)!=0)  {
      fLog->Error("XeNetClient::Connect - Error sending client identification.");
      close(sock);
      close(datasock);
      return -1;
   }
   if(fID==32000)  {
      if(ReceiveInt(sock,fID)!=0 || ReceiveInt(datasock,fID)!=0)	{
	 fLog->Error("XeNetClient::Connect - Failed to get ID from server. Closing.");
	 close(sock);
	 close(datasock);
	 return -1;
      }      
   }
   
   if(SendString(sock,fMyName)<0 || SendString(datasock,fMyName)<0)  {
      fLog->Error("XeNetClient::Connect - Error sending client name.");
      close(sock);
      close(datasock);
      return -1;
   }
   
   fSocket=sock;
   fDataSocket=datasock;
   fLog->Message("XeNetClient::Connect - connection established.");
   return 0;
}

int XeNetClient::Disconnect()
{
   close(fSocket);
   close(fDataSocket);
   return 0;
}


int XeNetClient::WatchForUpdates(XeStatusPacket_t &status)
{
   int a = CheckDataSocket(fDataSocket,status);
   if(a==0)   {
      XeDAQHelper::ProcessStatus(status);
      return 0;
   }
   return a;
}

int XeNetClient::SendCommand(string command)
{
   return SendCommandToSocket(fSocket,command,fID,fMyName);
}

int XeNetClient::ListenForCommand(string &command,int &id, string &sender)
{   
   return CommandOnSocket(fSocket,command,id,sender);
}

int XeNetClient::GetHistory(vector <string> &history,int linesToGet)
{
   history.clear();
//   if(MessageOnPipe(fSocket)!=0) return -1;
   string temp="HISTORY";
   if(SendString(fSocket,temp)!=0)  {
      fLog->Error("XeNetClient::GetHistory - Failed to send header.");
      return -1;
   }
   if(SendInt(fSocket,linesToGet)!=0)  {
      fLog->Error("XeNetClient::GetHistory - Failed to send number of lines request.");
      return -1;
   }
   while(ReceiveString(fSocket,temp)==0)  {
      if(temp=="HISTORYEND") break;
      history.push_back(temp);
   }
   //if((int)history.size()>=linesToGet)
     return 0;
}

int XeNetClient::GetRunInfoUI(XeRunInfo_t &runInfo)
{
   return ReceiveRunInfo(fSocket,runInfo);
}

int XeNetClient::SendBroadcastMessage(string message)
{
   string temp="BROADCAST";
   SendCommandToSocket(fSocket,temp,fID,fMyName);
   if(SendString(fSocket,message)!=0)    {	      
      fLog->Error("XeNetClient::SendBroadcast - failed to send message.");
	    return -1;
   }	 
   temp = "ENDBROADCAST";
   if(SendString(fSocket,temp)!=0)  {	      
      fLog->Error("XeNetClient::SendBroadcast - failed to send footer.");
      return -1;
   }	             
   return 0;      
}

int  XeNetClient::SendStatusUpdate(int status, double rate, double freq, int nBoards)
{
   return SendUpdate(fDataSocket,fID,fMyName,status,rate,freq,nBoards);
}

int  XeNetClient::GetStringList(vector<string> &stringList)
{
   int count=0;
   stringList.clear();
   while(MessageOnPipe(fSocket)!=0)  {
      sleep(1);
      count++;
      if(count==10) { 
	 fLog->Error("XeNetClient::GetStringList - nothing on pipe.");
	 return -1;
      }      
   }
   
   string temp="";
   while(ReceiveString(fSocket,temp)==0)  {
      if(temp=="@END@") break;
      stringList.push_back(temp);
      fLog->Message(temp);
   }
   if(stringList.size()!=0)
     return 0;
   return -1;
}

int  XeNetClient::ReceiveOptions(string optionsPath)
{
   return ReceiveFile(fSocket,optionsPath);
}

int XeNetClient::SlaveSendMessage(string message)
{
   return SendMessage(fDataSocket,fID,fMyName,message,XEMESS_BROADCAST);
}

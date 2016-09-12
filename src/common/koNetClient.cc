// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : koNetClient.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// Updated  : 27.03.2014
// 
// Brief    : Client-specific class for koNet inferface
// 
// ****************************************************

#include "koNetClient.hh"

koNetClient::koNetClient() : koNet()
{
   fHOSTNAME="localhost";
   fPORT=2000;
   fDATAPORT=2001;
   fID=0;
   fMyName="default";
}
koNetClient::~koNetClient()
{
}

koNetClient::koNetClient(koLogger *logger) : koNet(logger)
{
   fHOSTNAME="localhost";
   fPORT=2000;
   fDATAPORT=2001;
   fID=0;
   fMyName="default";
}

void koNetClient::Initialize(string HOSTNAME, int PORT, 
			     int DATAPORT, int ID, string MyName)
{
   fHOSTNAME=HOSTNAME;
   fPORT    =PORT;
   fDATAPORT=DATAPORT;
   fID      =ID;
   fMyName  =MyName;
}

int koNetClient::Connect()
{  
   
   //Check provided parameters to make sure they are valid
   if(fPORT<=0 || fDATAPORT<=0)  {
      LogError("koNetClient::Connect - invalid port or data port. Check options.");
      return -1;
   }
   struct hostent *server;
   server = gethostbyname(fHOSTNAME.c_str());
   if(server==NULL)  {
      LogError("koNetClient::Connect - failed to get server hostname.");
      return -1;      
   }   
   
   //Define socket
   int sock = socket(AF_INET,SOCK_STREAM,0);  //IPv4 only for now
   if(sock<0)  {
      LogError("koNetClient::Connect - Error creating socket");
      return -1;
   }
   
   //define server address struct
   struct sockaddr_in server_addr;
   bzero((char*)&server_addr,sizeof(server_addr));
   server_addr.sin_family = AF_INET;
   bcopy((char*)server->h_addr,(char*)&server_addr.sin_addr.s_addr,
	 server->h_length);
   server_addr.sin_port = htons(fPORT);
   
   //connect main socket
   int a;
   if((a=connect(sock,(struct sockaddr*)&server_addr,sizeof(server_addr)))<0)  {
      close(sock);
      return -1;
   }
   LogMessage("koNetClient::Connect - connected main socket.");
   
   //Define data socket
   int datasock = socket(AF_INET,SOCK_STREAM,0);
   if(datasock<0)  {
      LogError("koNetClient::Connect - bad data socket file descriptor");
      close(sock);
      return -1;
   }
   
   //Set server address struct for data
   struct sockaddr_in dataserveraddr;
   bzero((char*)&dataserveraddr,sizeof(dataserveraddr));
   dataserveraddr.sin_family = AF_INET;
   bcopy((char*)server->h_addr,(char*)&dataserveraddr.sin_addr.s_addr,
	 server->h_length);
   dataserveraddr.sin_port = htons(fDATAPORT);

   //connect data socket
   int timer=0;

   int b;
   while((b=connect(datasock,(struct sockaddr*)&dataserveraddr,
		    sizeof(dataserveraddr)))<0)  {
      timer++;
      sleep(1);
      if(timer==20){	   
	 LogError("koNetClient::Connect - failed to connect data socket.");
	 close(sock);
	 close(datasock);
	 return -1;
      }      
   }
   stringstream logmess;
   logmess<<"Connected to data socket with "<<b<<" and socket with "<<a;
   LogMessage(logmess.str());
   
   //Sockets should identify themselves
   if(SendInt(sock,fID)!=0 || SendInt(datasock,fID)!=0)  {
      LogError("koNetClient::Connect - Error sending client identification.");
      close(sock);
      close(datasock);
      return -1;
   }
   if(fID==32000)  {
      if(ReceiveInt(sock,fID)!=0 || ReceiveInt(datasock,fID)!=0)	{
	 LogError("koNetClient::Connect - Failed to get ID from server. Closing.");
	 close(sock);
	 close(datasock);
	 return -1;
      }      
   }
   
   if(SendString(sock,fMyName)<0 || SendString(datasock,fMyName)<0)  {
      LogError("koNetClient::Connect - Error sending client name.");
      close(sock);
      close(datasock);
      return -1;
   }
   
   fSocket=sock;
   fDataSocket=datasock;
   LogMessage("koNetClient::Connect - connection established.");
   return 0;
}

int koNetClient::Disconnect()
{
   close(fSocket);
   close(fDataSocket);
   return 0;
}


int koNetClient::WatchForUpdates(koStatusPacket_t &status)
{
  int a = CheckDataSocket(fDataSocket,status);
   if(a==0)   {
      koHelper::ProcessStatus(status);
      return 0;
   }
   return a;
}

int koNetClient::SendCommand(string command)
{
   return SendCommandToSocket(fSocket,command,fID,fMyName);
}

int koNetClient::ListenForCommand(string &command,int &id, string &sender)
{   
   return CommandOnSocket(fSocket,command,id,sender);
}

int koNetClient::GetHistory(vector <string> &history,int linesToGet)
{
   history.clear();
   string temp="HISTORY";
   if(SendString(fSocket,temp)!=0)  {
      LogError("koNetClient::GetHistory - Failed to send header.");
      return -1;
   }
   if(SendInt(fSocket,linesToGet)!=0)  {
      LogError("koNetClient::GetHistory - Failed to send number of lines request.");
      return -1;
   }
   while(ReceiveString(fSocket,temp)==0)  {
      if(temp=="HISTORYEND") break;
      history.push_back(temp);
   }
   return 0;
}

//int koNetClient::GetRunInfoUI(koRunInfo_t &runInfo)
//{
//   return ReceiveRunInfo(fSocket,runInfo);
//}

int koNetClient::SendBroadcastMessage(string message)
{
   string temp="BROADCAST";
   SendCommandToSocket(fSocket,temp,fID,fMyName);
   if(SendString(fSocket,message)!=0)    {	      
      LogError("koNetClient::SendBroadcast - failed to send message.");
      return -1;
   }	 
   temp = "ENDBROADCAST";
   if(SendString(fSocket,temp)!=0)  {	      
      LogError("koNetClient::SendBroadcast - failed to send footer.");
      return -1;
   }	             
   return 0;      
}

int  koNetClient::SendStatusUpdate(int status, double rate, double freq, 
				   int nBoards, koSysInfo_t sysinfo)
{
  return SendUpdate(fDataSocket,fID,fMyName,status,rate,freq,nBoards, sysinfo);
}

int  koNetClient::GetStringList(vector<string> &stringList)
{
   int count=0;
   stringList.clear();
   while(MessageOnPipe(fSocket)!=0)  {
      sleep(1);
      count++;
      if(count==10) { 
	 LogError("koNetClient::GetStringList - nothing on pipe.");
	 return -1;
      }      
   }
   
   string temp="";
   while(ReceiveString(fSocket,temp)==0)  {
      if(temp=="@END@") break;
      stringList.push_back(temp);
      LogMessage(temp);
   }
   if(stringList.size()!=0)
     return 0;
   return -1;
}

int koNetClient::ReceiveOptions(string optionsPath)
{
   return ReceiveFile(fSocket,optionsPath);
}

int koNetClient::SlaveSendMessage(string message)
{
   return SendMessage(fDataSocket,fID,fMyName,message,KOMESS_BROADCAST);
}

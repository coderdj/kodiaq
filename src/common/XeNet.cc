// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNet.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 14.10.2013
// 
// Brief    : General Network Interface for the DAQ
// 
// ****************************************************

#include <math.h>
#include <netinet/in.h>
#include "XeNet.hh"

XeNet::XeNet()
{
}

XeNet::~XeNet()
{
}

XeNet::XeNet(XeDAQLogger *fLogger)
{
   fLog=fLogger;
}

int XeNet::SendChar(int socket, char buff)
{
   return send(socket,&buff,1,MSG_NOSIGNAL);
}
int XeNet::ReceiveChar(int socket, char &buff)
{
   return recv(socket,&buff,1,0);
}

int XeNet::SendString(int socket, string buff)
//0-success, -1 failure
{
   int retval=0;
   for(unsigned int x=0; x<buff.size();x++)  {
      if(SendChar(socket,buff[x])<=0)
	retval=-1;
   }
   char end='~';
   if(SendChar(socket,end)<=0) retval=-1;
   return retval;      
}
int XeNet::ReceiveString(int socket,string &buff)
{
   string retstring="";
   bool end=false;
   while(!end)  {
      char cbuff='a';      
      int a;
      if((a=ReceiveChar(socket,cbuff))<0) return -1;
      if(a==0) return -2;
      if(cbuff=='~') break;
      retstring.push_back(cbuff);
      if(retstring.size()>1000) return -1; //crazy loop
   }
//   if(retstring.size()==0) return -1;
   buff=retstring;
   return 0;
}

int XeNet::SendFile(int socket, int id, string filepath)
{
   ifstream infile;
   infile.open(filepath.c_str());
   if(!infile) {
      fLog->Error("XeNet::SendFile - failed to open file. Check path.");
      return -1;
   }
   string line;
   while(!infile.eof())  {
      getline(infile,line);
      if(line[0]=='%')	{
	 if((int)(line[1]-'0')==id)  {
	    line.erase(0,2);
	 }
	 else
	   continue;
      }      
      if(SendString(socket,line)<0)	{
	 fLog->Error("XeNet::SendFile - error sending file line.");
	 return -1;
      }            
   }
   infile.close();
   string end = "@";
   if(SendString(socket,end)!=0)  {
      fLog->Error("XeNet::SendFile - error sending file ender. Data probably mangled.");
      return -1;
   }
   
   //Remote connection will return if he got the file
   if(ReceiveAck(socket)!=0)  {
      fLog->Error("XeNet::SendFile - file sent but no acknowledgement received.");
      return -1;
   }
   return 0;   
}

int XeNet::ReceiveFile(int socket, string path)
{
   ofstream outfile;
   outfile.open(path.c_str());
   if(!outfile)  {
      fLog->Error("XeNet::ReceiveFile - failed to open file. Check path.");
      string s="b";
      while(s!="@") //dump file
	if(ReceiveString(socket,s)!=0) return -1;
      return -1;
   }
   string line="line";
   while(line!="@")  {
      if(ReceiveString(socket,line)!=0)	{
	 fLog->Error("XeNet::ReceiveFile - failed to get line. Check connection.");
	 int x=-1;
	 while(x!=0 &&x!=-2)
	   x=ReceiveAck(socket);
	 return -1;
      }      
      outfile<<line<<endl;
   }
   outfile.close();
   if(SendAck(socket)!=0) {
      fLog->Error("XeNet::ReceiveFile - error sending acknowledgement.");
      return -1;
   }   
   return 0;
}

int XeNet::SendAck(int socket)
{
   int retval=0;
   char c='y';
   retval+=SendChar(socket,c);
   c='!';
   retval+=SendChar(socket,c);
   if(retval==2) return 0;
   return -1;
}

int XeNet::ReceiveAck(int socket)
{
   int retval=0;
   char c='a';
   retval+=ReceiveChar(socket,c);
   if(retval==-1) return -2;
   if(c!='y') return -1;
   retval+=ReceiveChar(socket,c);
   if(c!='!') return -1;
   if(retval!=2) return -1;
   return 0;
}

int XeNet::SendInt(int socket, int buff)
{
   for(unsigned int t=0;t<sizeof(int);t++)  {
      char sChar = (char)((buff>>(t*8))&0xFF);
      if(SendChar(socket,sChar)<=0) {
	 fLog->Error("XeNet::SendInt - error sending byte of int.");
	 return -1;
      }      
   }
   return 0;
}

int XeNet::ReceiveInt(int socket, int &buff)
{
   int retInt = 0; 
   for(unsigned int t=0;t<sizeof(int);t++)  {     
      char rChar;
      if(ReceiveChar(socket,rChar)<=0)	{
	 fLog->Error("XeNet::ReceiveInt - error receiving byte of int.");
	 return -1;
      }      
//      stringstream stream;
      retInt+=((u_int8_t)rChar<<(8*t));
   }
   buff=retInt;
   return 0;   
}

int XeNet::SendDouble(int socket, double buff)
{
   if(SendString(socket,XeDAQHelper::DoubleToString(buff))!=0)  {
      fLog->Error("XeNet::SendDouble - Error sending double.");
      return -1;
   }
   return 0;   
}

int XeNet::ReceiveDouble(int socket, double &buff)
{
   string temp;
   if(ReceiveString(socket,temp)!=0)  {
      fLog->Error("XeNet::ReceiveDouble - Error receiving double.");
      return -1;
   }
   buff=XeDAQHelper::StringToDouble(temp);
   return 0;   
}

int XeNet::MessageOnPipe(int pipe)
{
   struct timeval timeout; //we will define how long to listen
   fd_set readfds;
   timeout.tv_sec=0;
   timeout.tv_usec=0;
   FD_ZERO(&readfds);
   FD_SET(pipe,&readfds);
   select(FD_SETSIZE,&readfds,NULL,NULL,&timeout);
   if(FD_ISSET(pipe,&readfds)==1) return 0;
   else return -1;
   
}

int XeNet::CheckDataSocket(int socket, XeStatusPacket_t &status)
{
   int retval=-1;
   while(MessageOnPipe(socket)==0) {
      string type;
//      fLog->Message("found message on pipe");
      if((ReceiveString(socket,type))!=0)   {
	 fLog->Error("XeNet::CheckDataSocket - Saw message on pipe but no header.");	 
	 return -2;
      }   
//      fLog->Message(type);
      if(type=="UPDATE")        {      
	 int id,stat,nboards;
	 double rate,freq;
	 string name;
	 if(ReceiveUpdate(socket,id,name,rate,freq,nboards,stat)!=0)  {	 
	    fLog->Error("XeNet::CheckDataSocket - Saw update on pipe but failed to fetch.");
	    return -2;
	 }	
	 bool found=false;
	 for(unsigned int y=0;y<status.Slaves.size();y++)    {	     
	    if(status.Slaves[y].ID!=id) continue;
	    found=true;
	    status.Slaves[y].name=name;
	    status.Slaves[y].Rate=rate;
	    status.Slaves[y].Freq=freq;
	    status.Slaves[y].status=stat;
	    status.Slaves[y].nBoards=nboards;
	    status.Slaves[y].lastUpdate=XeDAQLogger::GetCurrentTime();
	    retval=0;
	 }	
	 if(!found)    {
	    XeNode_t newnode;
	    newnode.name=name;
	    newnode.Rate=rate;
	    newnode.Freq=freq;
	    newnode.status=stat;
	    newnode.nBoards=nboards;
	    newnode.lastUpdate=XeDAQLogger::GetCurrentTime();
	    newnode.ID=id;
	    status.Slaves.push_back(newnode);
	    retval=0;
	 }		
      }      
      else if(type=="MESSAGE")  {	
	 string message;
	 int code;
	 if(ReceiveMessage(socket,message,code)!=0)  {	     
	    fLog->Error("XeNetServer::CheckDataSocket - Saw message on pipe but failed to fetch.");
	    return -2;
	 }	
	 XeMessage_t mess;
	 mess.message=message;
	 mess.priority=code;
	 status.Messages.push_back(mess);
	 retval=0;
      }
      else if(type=="RUNMODE")	{
	 string mode;
	 if(ReceiveRunMode(socket,mode)!=0)  {
	    fLog->Error("XeNetServer::CheckDataSocket - Saw run mode on pipe but failed to fetch.");
	    return -2;
	 }
	 status.RunMode=mode;
	 retval=0;
      }
      
   }   
   return retval;
}

int XeNet::SendRunMode(int socket, string runMode)
{
   if(SendString(socket,"RUNMODE")!=0)  {
      fLog->Error("XeNet::SendRunMode - Error sending header.");
      return -1;
   }
   if(SendString(socket,runMode)!=0)  {
      fLog->Error("XeNet::SendRunMode - Error sending run mode.");
      return -1;
   }
   if(SendString(socket,"@END@")!=0)  {
      fLog->Error("XeNet::SendRunMode - Error sending footer.");
      return -1;
   }
   return 0;
}

int XeNet::ReceiveRunMode(int socket, string &runMode)
{
   if(ReceiveString(socket,runMode)!=0)  {
      fLog->Error("XeNet::ReceiveRunMode - error receiving run mode string.");
      return -1;
   }
   string temp;
   if(ReceiveString(socket,temp)!=0 || temp!="@END@")  {
      fLog->Error("XeNet::ReceiveRunMode - error receiving footer.");
      return -1;
   }   
   return 0;
}

int XeNet::ReceiveUpdate(int socket,int &id, string &name,
			 double &rate, double &freq, int &nBoards,int &status)
{
   //assumption: header UPDATE has already been received by the function
   //that watches the socket
   string temp;
   if(ReceiveInt(socket,id)!=0)    {      
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving ID.");
      return -1;
   }   
   if(ReceiveString(socket,name)!=0)   {	
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving name.");
      return -1;
   }   
   if(ReceiveInt(socket,status)!=0)    {      
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving status.");
      return -1;
   }   
   if(ReceiveInt(socket,nBoards)!=0)    {      
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving nboards.");
      return -1;
   }   
   if(ReceiveDouble(socket,rate)!=0)    {	
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving rate.");
      return -1;
   }   
   if(ReceiveDouble(socket,freq)!=0)    {      
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving frequency.");
      return -1;
   }   
   if(ReceiveString(socket,temp)!=0 || temp!="DONE")    {	
      fLog->Error("XeNetServer::ReceiveUpdate - Error receiving footer.");
      return -1;
   }   
   return 0;   
}

int XeNet::ReceiveMessage(int socket, string &message, int &code)
{   
   int id;
   string name,temp,foot;
   if(ReceiveInt(socket,code)!=0)  {	
      fLog->Error("XeNetServer::ReceiveMessage - Error receiving type code.");
      return -1;
   }   
   if(ReceiveInt(socket,id)!=0)    {	
      fLog->Error("XeNetServer::ReceiveMessage - Error receiving sender ID.");
      return -1;
   }   
   if(ReceiveString(socket,name)!=0)    {	
      fLog->Error("XeNetServer::ReceiveMessage - Error receiving sender name.");
      return -1;
   }
   if(ReceiveString(socket,temp)!=0)    {	
      fLog->Error("XeNetServer::ReceiveMessage - Error receiving message.");
      return -1;
   }   
   if(ReceiveString(socket,foot)!=0 || foot !="DONE")    {	
      fLog->Error("XeNetServer::ReceiveMessage - Error receiving footer.");
      return -1;
   }   
   
   if(id!=-1){
      stringstream ret;
      ret<<"Message from "<<name<<"("<<id<<"): "<<temp;
      message=ret.str();
   }   
   else message=temp;
   fLog->Message(message);
   return 0;
}

int XeNet::SendUpdate(int socket, int id, string name, int status, 
		      double rate, double freq, int nBoards)
{   
   //format: 
   //         UPDATE
   //         id (int)
   //         name (string)
   //         nBoards (int)
   //         rate (double)
   //         freq (double)
   //         DONE
    
   string UPDATEHEADER="UPDATE";
   if(SendString(socket,UPDATEHEADER)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending update header.");
      return -1;
   }
   if(SendInt(socket,id)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending update ID.");
      return -1;
   }
   if(SendString(socket,name)!=0)  {
             fLog->Error("XeNet::SendUpdate() - Error sending update name.");
      return -1;
   }
   if(SendInt(socket,status)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending status.");
      return -1;
   }
   if(SendInt(socket, nBoards)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending nBoards.");
      return -1;
   }
   if(SendDouble(socket,rate)!=0)   {
      fLog->Error("XeNet::SendUpdate() - Error sending rate.");
      return -1;
   }
   if(SendDouble(socket,freq)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending freq.");
      return -1;
   }
   string UPDATEFOOTER="DONE";
   if(SendString(socket,UPDATEFOOTER)!=0)  {
      fLog->Error("XeNet::SendUpdate() - Error sending footer.");
      return -1;
   }
   return 0;
}

int XeNet::SendMessage(int socket, int id, string name,string message, int type)
{   
   string MESSAGEHEADER="MESSAGE";
   string MESSAGEFOOTER="DONE";
   if(SendString(socket,MESSAGEHEADER)!=0)   {      
      fLog->Error("XeNet::SendMessage - Error sending header.");
      return -1;
   }   
   if(SendInt(socket,type)!=0)  {	
      fLog->Error("XeNet::SendMessage - Error sending type code.");
      return -1;
   }   
   if(SendInt(socket,id)!=0)  {	
      fLog->Error("XeNet::SendMessage - Error sending ID.");
      return -1;
   }   
   if(SendString(socket,name)!=0)  {	
      fLog->Error("XeNet::SendMessage - Error sending name.");
      return -1;
   }   
   if(SendString(socket,message)!=0)  {	
      fLog->Error("XeNet::SendMessage - Error sending message string");
      return -1;
   }   
   if(SendString(socket,MESSAGEFOOTER)!=0)  {	
      fLog->Error("XeNet::SendMessage - Error sending message footer.");
      return -1;
   }   
   return 0;
}

int XeNet::SendCommandToSocket(int socket,string command,int id, string sender)
{
   string header="COMMAND";
   string footer="DONE";
   if(SendString(socket,header)==0)  {
      if(SendString(socket,sender)==0)	{
	 if(SendInt(socket,id)==0)  {
	    if(SendString(socket,command)==0){
	       if(SendString(socket,footer)==0)	 {
		  stringstream mess;
		  mess<<"XeNet::SendCommandToSocket - Sent command "<<command<<" to "<<socket<<" from "<<sender<<"("<<id<<")";
		  fLog->Message(mess.str());
		  return 0;
	       }	       
	    }	    
	 }	 
      }      
   }
   fLog->Error("XeNet::SendCommandToSocket - Error sending command.");
   return -1;   
}

int XeNet::CommandOnSocket(int socket, string &command, int &senderid, string &sender)
{
   if(MessageOnPipe(socket)!=0) return -1; 
   string temp;
   int a=0;
   if((a=ReceiveString(socket,temp)==0) && temp=="COMMAND")  {
      if(ReceiveString(socket,sender)==0)	{
	 if(ReceiveInt(socket, senderid)==0)  {
	    if(ReceiveString(socket,command)==0)  {
	       if(ReceiveString(socket,temp)==0 && temp=="DONE"){		    
		  stringstream mess;		
		  mess<<"XeNet::CommandOnSocket - Received command "<<command<<" from "<<sender<<"("<<senderid<<")";
		  if(command!="KEEPALIVE") //specifically ignore this one
		    fLog->Message(mess.str());
		  return 0;
	       }	       
	    }	    
	 }	 
      }      
   }
   if(a==-2)     {      
      fLog->Error("XeNet::CommandOnSocket - Error receiving command.");      
      return -2;
   }
   return -1;
}

int XeNet::SendRunInfo(int socket, XeRunInfo_t runInfo)
{
   string temp="RUNINFO";
   if(SendString(socket,temp)!=0) return -1;
   if(SendString(socket,runInfo.RunNumber)!=0) return -1;
   if(SendString(socket,runInfo.StartedBy)!=0) return -1;
   if(SendString(socket,runInfo.StartDate)!=0) return -1;
   temp="ENDRUNINFO";
   if(SendString(socket,temp)!=0) return -1;
   return 0;
}

int XeNet::ReceiveRunInfo(int socket, XeRunInfo_t &runInfo)
{
   string temp;
   if(ReceiveString(socket,temp)!=0 || temp!="RUNINFO") return -1;
   if(ReceiveString(socket,runInfo.RunNumber)!=0) return -1;
   if(ReceiveString(socket,runInfo.StartedBy)!=0) return -1;
   if(ReceiveString(socket,runInfo.StartDate)!=0) return -1;
   if(ReceiveString(socket,temp)!=0 || temp!="ENDRUNINFO") return -1;
   return 0;
}

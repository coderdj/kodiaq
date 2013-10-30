#ifndef _XENETSERVER_HH_
#define _XENETSERVER_HH_

// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNetServer.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// 
// Brief    : Server-specific class for xenet inferface
// 
// ****************************************************

#include "XeNet.hh"

struct XeSocket_t
{
   int socket;
   int id;
   string name;
};

class XeNetServer : public XeNet
{
 public:
   XeNetServer();
   virtual ~XeNetServer();
   explicit XeNetServer(XeDAQLogger *logger, XeDAQLogger *broadcastlogger);
   
   //XeNetServer::Initialize - Set connection info for clients
   void Initialize(int PORT, int DATAPORT, int NUMCLIENTS);
   
   //Connection management
   int PutUpNetwork();
   int TakeDownNetwork();
   int Connect();
   int Disconnect();
   int AddConnection(int &rid,string &rname);
   int CloseConnection(int id=-1, string name="");
   
   //For data synchronization
   int WatchDataPipe(XeStatusPacket_t &status);
   int TransmitStatus(XeStatusPacket_t status);
   
   //For command
   int SendCommand(string command);
   int ListenForCommand(string &command,int &id, string &sender);
   
   //User communication
   int BroadcastMessage(string message, int priority);

   //UI-specific functions
   int SendFilePartial(int id,string filepath);
   int SendRunInfoUI(int id,XeRunInfo_t runInfo);
   int ReceiveBroadcast(int id,string &broadcast);
   int SendStringList(int id, vector<string> stringlist);

   //DAQ-management functions
   int SendOptions(string filepath);
 private:
   XeDAQLogger *fBroadcastLog;
   
   //Connection info
   int fPORT,fDATAPORT,fNUMCLIENTS;
   vector <XeSocket_t> fSockets;
   vector <XeSocket_t> fDataSockets;
   int fConnectionSocket, fConnectionDataSocket;
   
   //Communication
   int ReceiveUpdate(int socket,int &id,string &name,double &rate,
		     double &freq,int &nBoards,int &status);
   int ReceiveMessage(int socket,string &message,int &code);
};

#endif

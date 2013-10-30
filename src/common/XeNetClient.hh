#ifndef _XENETCLIENT_HH_
#define _XENETCLIENT_HH_

// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNetClient.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// 
// Brief    : Client-specific class for xenet inferface
// 
// ****************************************************

#include "XeNet.hh"

class XeNetClient : public XeNet
{
 public:
   XeNetClient();
   virtual ~XeNetClient();
   explicit XeNetClient(XeDAQLogger *logger);
   
   //XeNetClient::Initialize - Set connection information for server
   void Initialize(string HOSTNAME, int PORT, int DATAPORT, int ID, string MyName);
   
   //Connection Management
   int  Connect();
   int  Disconnect();

   //Data synchronization
   int  WatchForUpdates(XeStatusPacket_t &status);
      
   //Command
   int  SendCommand(string command);
   int  ListenForCommand(string &command,int &id, string &sender);

   //UI-specific functions
   int  GetHistory(vector <string> &history, int linesToGet=1000);
   int  GetRunInfoUI(XeRunInfo_t &fRunInfo);
   int  SendBroadcastMessage(string message);   
   int  GetStringList(vector <string> &stringList);
   
   //slave specific function
   int  SendStatusUpdate(int status, double rate, double freq, int nBoards);
   int  ReceiveOptions(string optionsPath);
 private:   
   
   //connection info
   string fHOSTNAME,fMyName;
   int    fPORT,fDATAPORT,fID;
   int    fSocket,fDataSocket;   
};

#endif
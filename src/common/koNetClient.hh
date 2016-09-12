#ifndef _KONETCLIENT_HH_
#define _KONETCLIENT_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : koNetClient.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// Updated  : 27.03.2014
// 
// Brief    : Client-specific class for koNet inferface
// 
// ****************************************************

#include "koNet.hh"

class koNetClient : public koNet
{
   
 public:
   
                koNetClient();
   virtual      ~koNetClient();
   explicit      koNetClient(koLogger *logger);
   
   //  
   // Name     : XeNetClient::Initialize
   // Function : Set connection information for the server
   // 
   void          Initialize(string HOSTNAME, int PORT, int DATAPORT, 
			    int ID, string MyName);
   
   //
   // Name     : Connect
   // Function : Try to connect to the server (initialize must have been
   //            called already). Return 0 on success, -1 on failure.
   // 
   int           Connect();
   //
   // Name     : Disconnect
   // Function : Close sockets and put down network
   // 
   int           Disconnect();

   //
   // Name     : WatchForUpdates
   // Function : Monitor the data socket and return 0 if an update is there.
   //            Also fill status with the info on the update in this case.
   //            Returns -1 if nothing found.
   // 
  int           WatchForUpdates(koStatusPacket_t &status);
      
   //
   // Name     : SendCommand
   // Function : Send a command over the main socket. The command can be
   //            any string. Server program should be monitoring the 
   //            socket.
   // 
   int           SendCommand(string command);
   //
   // Name     : ListenForCommand
   // Function : Listens for a command from the server. Returns zero if 
   //            one comes and fills the arguments accordingly.
   // 
   int           ListenForCommand(string &command,int &id, string &sender);

   // Note: kodiaq uses two different server/client relationships. 
   //       master/slave and master/UI. The following specialized
   //       functions are useful just for one of them.
   // 
   // 
   // UI-specific functions
   // 
   // Name     : GetHistory
   // Function : The master transfers the last linesToGet of the log file
   //            as a string vector.
   // 
   int           GetHistory(vector <string> &history, int linesToGet=1000);
   //
   // Name     : GetRunInfoUI
   // Function : The koRunInfo object is transferred from the server and 
   //            received by the UI.
   // 
//   int           GetRunInfoUI(koRunInfo_t &fRunInfo);
   //
   // Name     : SendBroadcastMessage
   // Function : Broadcast a message from one UI to all others. Actually this
   //            function just sends it to the server marked as a broadcast.
   //            The server is then responsible for distributing it.
   // 
   int           SendBroadcastMessage(string message);
   //
   // Name     : GetStringList
   // Function : It must have already been arranged with the master that he
   //            will send a vector of strings. This function receives that
   //            vector.
   // 
   int           GetStringList(vector <string> &stringList);
   //
   //
   // Slave specific functions
   // 
   // Name     : SendStatusUpdate
   // Function : Slaves send their status to the master over the data pipe.
   // 
  int           SendStatusUpdate(int status, double rate, double freq, int nBoards,
				 koSysInfo_t sysinfo);
   //
   // Name     : Receive options
   // Function : Receive the options file over the network. The argument is
   //            the path where the file should be saved.
   // 
   int           ReceiveOptions(string optionsPath);
   //
   // Name     : SlaveSendMessage
   // Function : Slaves can send messages too. The master should put these in
   //            the log file and fan them to any connected UIs.
   // 
   int           SlaveSendMessage(string message);
   //
   
 private:   
   
   //connection info
   string fHOSTNAME,fMyName;
   int    fPORT,fDATAPORT,fID;
   int    fSocket,fDataSocket;   
};

#endif

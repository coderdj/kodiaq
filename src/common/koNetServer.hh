#ifndef _KONETSERVER_HH_
#define _KONETSERVER_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : koNetServer.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 22.10.2013
// Updated  : 27.03.2013
// 
// Brief    : Server-specific class for koNet inferface
// 
// ****************************************************

#include "koNet.hh"
#include <arpa/inet.h>

//
// Name    : koSocket_t
// Purpose : Holds all information pertaining to one socket connection
// 
struct         koSocket_t
{
   int         socket;
   int         id;
   string      name;
   string      ip;
   time_t      loginTime;
};

class koNetServer : public koNet
{
   
 public:
   
                koNetServer();
   virtual     ~koNetServer();
   explicit     koNetServer(koLogger *logger, koLogger *broadcastlogger=NULL);
   
   //
   // Name    : Initialize 
   // Purpose : Set connection info for network interface
   // 
  void         Initialize(int PORT, int DATAPORT);//, int NUMCLIENTS);
   //
   // Connection management
   //
   // Name    : PutUpNetwork
   // Purpose : Puts up the network interface and waits for incoming
   //           connections
   // 
   int          PutUpNetwork();
   //
   // Name    : TakeDownNetwork
   // Purpose : Takes down the new connection interface. Existing connections
   //           are maintained but no new ones can be added.
   // 
   int          TakeDownNetwork();
   //
   
   //int Connect();
   //
   // Name    : Disconnect
   // Purpose : Close all active connections (sockets and data sockets)
   // 
   int          Disconnect();
   //
   // Name    : AddConnection
   // Purpose : Listens for connections on the connection interface and
   //           adds a new one if it's there
   // 
   int          AddConnection(int &rid,string &rname, string &rIP);
   // 
   // Name    : Remove connection
   // Purpose : Close a specific connection defined by either id number or
   //           name. In case of defining by name, set id to -1.
   // 
   int          CloseConnection(int id=-1, string name="");
   //
   // 
   // For data synchronization
   //
   // Name    : WatchDataPipe
   // Purpose : Monitors the data pipes for traffic. If something comes then
   //           updates status and returns 0. Otherwise return -1
   // 
   int          WatchDataPipe(koStatusPacket_t &status);
   //
   // Name    : TransmitStatus
   // Purpose : Sends a status packet over the data pipe
   // 
   int          TransmitStatus(koStatusPacket_t status);
   //
   //
   // For commands
   // 
   // Name    : SendCommand
   // Purpose : Send a command string over the main pipe to all connected
   //           clients
   // 
   int          SendCommand(string command);
   //
   // Name    : ListenForCommand
   // Purpose : Monitor all main pipes to see if a command comes over them.
   //           If so, fill the arguments and return zero.
   // 
   int          ListenForCommand(string &command,int &id, string &sender);
   //
   // User communication
   // 
   // Name    : BroadcastMessage
   // Purpose : Send a message to all connected clients
   // 
   int          BroadcastMessage(string message, int priority, int UI=-1);
   //
   //
   // UI-specific functions
   // 
   // Name    : SendFilePartial
   // Purpose : Sends part of a file. The recepient specifies the maximum
   //           number of lines over the network.
   // 
   int          SendFilePartial(int id,string filepath);
   //
   // Name    : SendRunInfoUI
   // Purpose : Send a run info object over the network
   // 
   int          SendRunInfoUI(int id,koRunInfo_t runInfo);
   //
   // Name    : ReceiveBroadcast
   // Purpose : Receive a broadcast from one of the clients. It should then be 
   //           routed to all the others.
   //
   int          ReceiveBroadcast(int id,string &broadcast);
   //
   // Name    : SendStringList
   // Purpose : Send a list of items over the network as a vector of strings
   // 
   int          SendStringList(int id, vector<string> stringlist);
   //
   // Name    : GetUserList
   // Purpose : Return a list of connected users
   // 
   int          GetUserList(vector <string> &stringList);
   //
   // Name    : SendOptions
   // Purpose : Send an options file located at filepath
   // 
   int          SendOptions(string filepath);
   int          SendOptionsStream(stringstream *stream);
   //
   
 private:
   koLogger *m_koBroadcastLog;
   
   //Connection info
  int fPORT,fDATAPORT;//,fNUMCLIENTS;
   vector <koSocket_t> fSockets;
   vector <koSocket_t> fDataSockets;
   int fConnectionSocket, fConnectionDataSocket;
   
   //Communication
   int ReceiveUpdate(int socket,int &id,string &name,double &rate,
		     double &freq,int &nBoards,int &status);
   int ReceiveMessage(int socket,string &message,int &code);
};

#endif

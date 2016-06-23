#ifndef _KONET_HH_
#define _KONET_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : koNet.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.10.2013
// Update   : 27.03.2014
// 
// Brief    : General Network Interface for the DAQ
// 
// ****************************************************
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <strings.h>
#include <cstring>
#include <vector>
#include <time.h>
#include <signal.h>

#include "koLogger.hh"
#include "koHelper.hh"
#include "koSysmon.hh"

using namespace std;

class koNet
{
   
 public: 
   
                 koNet();                  //logging not available
   virtual      ~koNet();   
   explicit      koNet(koLogger *fLogger); //use this one

   void          SetLog(koLogger *logger){
     m_koLog = logger;
   };
 protected:   

   int        MessageOnPipe(int pipe);
   
   //
   // Name     : SendString
   // Function : Send a string (buff) over the pipe (pipe)
   // 
   int           SendString(int pipe, string buff);
   //
   // Name     : ReceiveString
   // Function : Receive a string and place into buff
   // 
   int           ReceiveString(int pipe, string &buff);
   
   //
   // Name     : SendChar
   // Function : Send a single char. This is the only function in this
   //            class which actually calls the 'send' function
   // 
   int           SendChar(int pipe, char buff);
   //
   // Name     : ReceiveChar
   // Function : Receive a char over the pipe and puts into buff
   //            This is the only function that calls recv
   // 
   int           ReceiveChar(int pipe, char &buff);
   
   //
   // Name     : SendFile
   // Function : Sends a file over the pipe. The file must exist at path.
   //            The int 'id' is the id of the recipient. Lines of the file
   //            intended for other recipients (using %n) are culled by
   //            the sender.
   // 
   int           SendFile(int pipe, int id, string path);
   //             SendStream same as send file but for stringstream
   int           SendStream(int pipe,int id, stringstream *stream);
   //
   // Name     : ReceiveFile
   // Function : Receives a file over the network and writes it to path.
   //            Of course path must point somewhere the program is allowed 
   //            to write. Existing files are overwritten.
   int           ReceiveFile(int pipe, string path);
   
   //
   // Name     : SendInt
   // Function : Sends an int over the pipe. This is done by converting the
   //            int to a sequence of chars (2 bytes). Only 32-bit ints can 
   //            be sent with this function. 
   // 
   int           SendInt(int pipe, int buff);
   //
   // Name     : ReceiveInt
   // Function : Receives a 32-bit integer over the network
   // 
   int           ReceiveInt(int pipe, int &buff);
   
   //
   // Name     : SendDouble
   // Function : Sends a double over the network by first converting to a 
   //            string and using the SendString function
   // 
   int           SendDouble(int pipe, double buff);
   //
   // Name     : ReceiveDouble
   // Function : Receives a double over the network
   // 
   int           ReceiveDouble(int pipe, double &buff);

   //
   // Name     : SendRunInfo
   // Function : Sends a koRunInfo_t object over pipe
   // 
   int           SendRunInfo(int pipe,koRunInfo_t runInfo);
   //
   // Name     : ReceiveRunInfo
   // Function : Receives a koRunInfo_t object over the pipe
   // 
   int           ReceiveRunInfo(int pipe,koRunInfo_t &runInfo);
   
   //
   // Name     : SendCommandToSocket
   // Function : Sends a command (a string) to a socket while identifying
   //            the sender through the id number and name       
   int           SendCommandToSocket(int socket, string command,
				     int id, string sender);
   //
   // Name     : CommandOnSocket
   // Function : Checks the socket for a pending command. Returns -1 if there
   //            isn't one. If there is one it returns 0 and fills command,
   //            senderid, and sender with the information from the network
   int           CommandOnSocket(int socket, string &command, 
				 int &senderid, string &sender);
 
   // 
   // The following functions are used to check the data flow coming over the
   // data socket. The data socket is meant to be used as a constant one-way
   // flow of information from slave -> master and master -> user. 
   // 
   // Name     : CheckDataSocket
   // Function : Checks if there is anything on the data socket. Possible info
   //            are updates, messages, and run modes. The status packed is 
   //            updated and zero is returned if something is there.
   // 
  int           CheckDataSocket(int socket, koStatusPacket_t &status,
				koSysInfo_t &sysinfo);
   //
   // Name     : SendMessage
   // Function : Send a message over the data socket. It should be picked up
   //            at the other end by the CheckDataSocket function
   // 
   int           SendMessage(int socket, int id, string name, 
			     string message, int type);
   //
   // Name     : ReceiveMessage
   // Function : Used within CheckDataSocket to receive messages 
   // 
   int           ReceiveMessage(int socket,string &message, int &code);
   //
   // Name     : SendUpdate
   // Function : Sends an update over the socket with the parameters in the 
   //            arguments
   // //
   int           SendUpdate(int socket, int id, string name, int status,
			    double rate, double freq, int nBoards, 
			    koSysInfo_t sysinfo);
   //
   // Name     : ReceiveUpdate
   // Function : Called by CheckDataSocket to receive updates
   // 
   int           ReceiveUpdate(int socket,int &id, string &name,
			       double &rate, double &freq, 
			       int &nBoards,int &status, koSysInfo_t &sysinfo);
   //
   // Name     : SendRunMode
   // Function : Sends the run mode over the data pipe
   // 
   int           SendRunMode(int pipe,string runMode);
   // 
   // Name     : ReceiveRunMode
   // Function : Called by CheckDataSocket to read the run mode
   // 
   int           ReceiveRunMode(int pipe,string &runMode);

   // Protected wrapper for log file
   void          LogError(string error);
   void          LogMessage(string message);
   
 private:
   koLogger  *m_koLog;
   int        SendAck(int pipe);
   int        ReceiveAck(int pipe);
};

#endif

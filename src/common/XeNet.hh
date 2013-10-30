#ifndef _XENET_HH_
#define _XENET_HH_

// ****************************************************
// 
// DAQ Control for XENON1T
// 
// File     : XeNet.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.10.2013
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

#include "XeDAQLogger.hh"
#include "XeDAQHelper.hh"

using namespace std;

class XeNet
{
 public: 
   XeNet();
   virtual ~XeNet();
   
   explicit XeNet(XeDAQLogger *fLogger);
   
 protected:   
   int        SendString(int pipe, string buff);
   int        ReceiveString(int pipe, string &buff);
   
   int        SendChar(int pipe, char buff);
   int        ReceiveChar(int pipe, char &buff);
   
   int        SendFile(int pipe, int id, string path);
   int        ReceiveFile(int pipe, string path);
   
   int        SendInt(int pipe, int buff);
   int        ReceiveInt(int pipe, int &buff);
   
   int        SendDouble(int pipe, double buff);
   int        ReceiveDouble(int pipe, double &buff);

   int        SendRunInfo(int pipe,XeRunInfo_t runInfo);
   int        ReceiveRunInfo(int pipe,XeRunInfo_t &runInfo);
   
   int        SendRunMode(int pipe,string runMode);
   int        ReceiveRunMode(int pipe,string &runMode);
   
   XeDAQLogger *fLog;
   int        MessageOnPipe(int pipe);
 
   int        CheckDataSocket(int socket,XeStatusPacket_t &status);
   int        ReceiveUpdate(int socket,int &id, string &name,
			    double &rate, double &freq, int &nBoards,int &status);
   int        ReceiveMessage(int socket,string &message, int &code);
   int        SendUpdate(int socket, int id, string name, int status, 
			 double rate, double freq, int nBoards);
   int        SendMessage(int socket, int id, string name, string message, int type);
 
   int        SendCommandToSocket(int socket, string command,int id, string sender);
   int        CommandOnSocket(int socket, string &command, int &senderid, string &sender);
 private:
   int        SendAck(int pipe);
   int        ReceiveAck(int pipe);
};

#endif
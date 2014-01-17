#ifndef _XEDAQHELPER_HH_
#define _XEDAQHELPER_HH_

// *********************************************************
// 
// DAQ Control for Xenon-1t
// 
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 3.07.2013
// File     : XeDAQHelper.hh
// 
// Brief    : Several useful functions used throughout the
//            DAQ program.
//
// **********************************************************

#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <sys/types.h>

#include <unistd.h>
#include "XeDAQLogger.hh"


//DAQ STATUS IDENTIFIERS
#define XEDAQ_IDLE    0
#define XEDAQ_ARMED   1
#define XEDAQ_RUNNING 2
#define XEDAQ_MIXED   3
#define XEDAQ_ERROR   4


//MESSAGE PRIORITY IDENTIFIERS
#define XEMESS_UPDATE    0
#define XEMESS_BROADCAST 4
#define XEMESS_WARNING   2
#define XEMESS_ERROR     3
#define XEMESS_NORMAL    1
#define XEMESS_STATE     5

using namespace std;

struct XeNode_t{
   int        status;        //DAQ status identifier
   double     Rate;          //in MB/s
   double     Freq;          //in Hz
   int        nBoards;      
   int        ID;
   string     name;
   time_t     lastUpdate;
};

struct XeMessage_t{  
   string message;
   int priority;
};

struct XeStatusPacket_t{
   string     RunMode;            //Run mode path to file
   string     RunModeLabel;       //Run mode identifier
   int        NumSlaves;          //Number of slaves from net setup file
   int        DAQState;           //state of the total DAQ
   bool       NetworkUp;          //if the network is connected
      
   vector <XeMessage_t> Messages; //Pending messages from slaves
   vector <XeNode_t> Slaves;      //Rates of slaves      
};

struct XeRunInfo_t{
   string StartedBy;
   string RunNumber;
   string StartDate;
   string RunInfoPath;
};


class XeDAQHelper
{
 public:
   XeDAQHelper();
   virtual ~XeDAQHelper();
   
   static bool      ParseMessage(const int pipe, vector <string> &data);
   static bool      ParseMessageFull(const int pipe,vector<string> &data,char &command);
   static bool      ComposeMessage(string &message,vector<string> data,char type);
   static bool      MessageOnPipe(int pipe);
   static string    GetRunNumber();
   static u_int32_t StringToInt(const string &str);
   static string    IntToString(const int num);
   static u_int32_t StringToHex(const string &str);
   static double    StringToDouble(const string &str);
   static string    DoubleToString(const double num);
   static int       CurrentTimeInt();
   static int       InitializeRunInfo(XeRunInfo_t &fRunInfo);
   static int       UpdateRunInfo(XeRunInfo_t &fRunInfo,string startedby);
   
   static void InitializeStatus(XeStatusPacket_t &Status);
   static void ProcessStatus(XeStatusPacket_t &Status);
   static void InitializeNode(XeNode_t &Node);
 private:
};
#endif

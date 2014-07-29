#ifndef _KOHELPER_HH_
#define _KOHELPER_HH_

// *********************************************************
// 
// kodiaq Data Acquisition Software
// 
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 03.07.2013
// Update   : 27.03.2014
// File     : koHelper.hh
// 
// Brief    : Several useful functions used throughout the
//            DAQ program. Many status objects as well as 
//            static converters, initializers, etc.
//
// **********************************************************

#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <sys/types.h>

#include <unistd.h>
#include "koLogger.hh"


//DAQ STATUS IDENTIFIERS
#define KODAQ_IDLE    0
#define KODAQ_ARMED   1
#define KODAQ_RUNNING 2
#define KODAQ_MIXED   3
#define KODAQ_ERROR   4


//MESSAGE PRIORITY IDENTIFIERS
#define KOMESS_UPDATE    0
#define KOMESS_NORMAL    1
#define KOMESS_WARNING   2
#define KOMESS_ERROR     3
#define KOMESS_BROADCAST 4
#define KOMESS_STATE     5

using namespace std;

//
// Object   : koNode_t
// Brief    : Gives the summary of one slave node's status
//  
struct koNode_t{
   int        status;            // holds a DAQ Status Enum
   double     Rate;              //in MB/s
   double     Freq;              //in Hz
   int        nBoards;           //
   int        ID;                //slave ID as configured by user
   string     name;              //slave name as configured by user
   time_t     lastUpdate;        //time last remote update was received
};

//
// Object   : koMessage_t
// Brief    : Message text plus type
// 
struct koMessage_t{  
   string     message;           //
   int        priority;          // message priority enum
};

//
// Object    : koRunInfo_t
// Brief     : Information about the current run
// 
struct koRunInfo_t{
   string StartedBy;              // user who started (user's login name)
   string RunNumber;              // name or number of run
   string StartDate;              // date and time string
   string RunInfoPath;            // where to find the run info
};
            
//
// Object   : koStatusPacket_t
// Brief    : Holds the status of the entire DAQ
// 
struct koStatusPacket_t{
   string     RunMode;            // Run mode path to file
   string     RunModeLabel;       // Run mode identifier
   int        NumSlaves;          // Number of slaves from net setup file
   int        DAQState;           // state of the total DAQ
   bool       NetworkUp;          // if the network is connected
      
   vector <koMessage_t> Messages; // Pending messages from slaves
   vector <koNode_t> Slaves;      // Rates of slaves      
   koRunInfo_t RunInfo;           // info for current run
};


class koHelper
{
 public:
   koHelper();
   virtual ~koHelper();
   
//   static bool      ParseMessage(const int pipe, vector <string> &data);
//   static bool      ParseMessageFull(const int pipe,vector<string> &data,char &command);
//   static bool      ComposeMessage(string &message,vector<string> data,char type);
//   static bool      MessageOnPipe(int pipe);
     
   //
   // Function    : StringToInt
   // Purpose     : Converst a string to a 32-bit unsigned integer
   // 
   static u_int32_t StringToInt(const string &str);
   //
   // Function    : string IntToString
   // Purpose     : Converts an integer to a string
   // 
   static string    IntToString(const int num);
   //
   // Function    : StringToHex
   // Purpose     : Converts a hex string to a 32-bit unsigned int
   // 
   static u_int32_t StringToHex(const string &str);
   //
   // Function    : StringToDouble
   // Purpose     : Converts a string to a double (i.e. when received over a pipe)
   // 
   static double    StringToDouble(const string &str);
   //
   // Function    : DoubleToString
   // Purpose     : Concerts a double to a string (i.e. when sending over a pipe)
   // 
   static string    DoubleToString(const double num);
   //
   // Function    : InitializeStatus
   // Purpose     : Initializes a koStatusPacket_t object be setting all 
   //               members to default values
   // 
   static void InitializeStatus(koStatusPacket_t &Status);
   //
   // Function    : ProcessStatus(koStatusPacket_t)
   // Purpose     : Checks status of all nodes reported to koStatusPacket_t
   //               object to get the status of the whole DAQ
   // 
   static void ProcessStatus(koStatusPacket_t &Status);
   //
   // Function    : CurrentTimeInt()
   // Purpose     : Gives the current time in integer form. Note: this is NOT the
   //               'normal' UNIX time, which is the number of seconds since epoch.
   //               This is actually literally an integral form of the time. For
   //               example 27. Mar 2014 at 5:25AM becomes 14032705. 
   // 
   static int       CurrentTimeInt();
   //
   // Function    : InitializeRunInfo(koRunInfo_t &fRunInfo)
   // Purpose     : Initializes a run info object by reading the information from
   //               the file at fRunInfo.RunInfoPath
   // 
   static int       InitializeRunInfo(koRunInfo_t &fRunInfo);
   // 
   // Function    : UpdateRunInfo(koRunInfo_t &, string)
   // Purpose     : Updates the run info file at fRunInfo.RunInfoPath with the new
   //               start time, number, who started it, etc. The object is of course
   //               also updated.
   // 
   static int       UpdateRunInfo(koRunInfo_t &fRunInfo,string startedby);
   //
   // Function    : EasyPassHash(string pass)
   // Purpose     : A very simple hash for passwords. Nothing fancy, just makes
   //               it :slightly: more difficult to guess. Note that with the current
   //               hash, multiple solutions are possible.
   // 
   static int       EasyPassHash(string pass);    
   //
   // Function    : InitializeNode(XeNode_t&)
   // Purpose     : Sets all values in this XeNode_t to their defaults
   // 
   static void InitializeNode(koNode_t &Node);
   //
   // Function    : GetRunNumber()
   // Purpose     : Returns a run number (actually a run name string) based on 
   //               the current time and date
   // 
   static string    GetRunNumber();
   //
   // Function    : MakeDBName
   // Purpose     : Returns a database name based on the current run number from
   //               the koRunInfo_t object
   // 
   static string    MakeDBName(koRunInfo_t RunInfo,string CollectionName);
  
   // Function    : GetTimeStamp
   // Purpose     : Returns the time stamp out the the header of a CAEN block
   //
   static u_int32_t GetTimeStamp(u_int32_t *buffer);

 private:
};
#endif

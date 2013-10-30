#ifndef _XEDAQLOGGER_HH_
#define _XEDAQLOGGER_HH_

// *************************************************
// *************************************************
// 
// File    : XeDAQLogger.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 27.6.2013
// 
// Brief   : Logger for Xenon-1t DAQ software
// 
// *************************************************
// *************************************************

#include <fstream>
#include <string>
#include <vector>
#include <time.h>

using namespace std;

class XeDAQLogger
{
 public:
   XeDAQLogger();
   virtual ~XeDAQLogger();
   explicit XeDAQLogger(string logpath);
   
   void SetLogfile(string logpath);
   void Message(string message);
   void SaveBroadcast(string message, int priority);
   void Error(string message);
   static string GetTimeString();   
   static time_t GetCurrentTime();
   
   void SendMessage(string message)  {
      fMessageBuffer.push_back(message);
   };
   int GetMessages(){
      return fMessageBuffer.size();
   };
   string GetMessage();
   
   
 private: 
   ofstream fLogfile;   
   std::vector <string> fMessageBuffer;
};

#endif

#ifndef _KOLOGGER_HH_
#define _KOLOGGER_HH_

// *************************************************
//
// kodiaq Data Acquisition Software
//  
// File    : koLogger.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 27.06.2013
// Updated : 27.03.2014
// 
// Brief   : Log file creator and maintainer
// 
// *************************************************

#include <fstream>
#include <string>
#include <vector>
#include <time.h>
#include <pthread.h>

using namespace std;

class koLogger
{
   
 public:
   
                 koLogger();
   virtual      ~koLogger();
   explicit      koLogger(string logpath);
   
   //
   // Name     : SetLogfile(string logpath)
   // Function : Closes the logfile if open and reopens it at logpath
   // 
   void          SetLogfile(string logpath);
   //
   // Name     : Message(string message)
   // Function : Records message into the log file as a normal message
   // 
   void Message(string message);
   //
   // Name     : SaveBroadcast(string message, int priority)
   // Function : The broadcast log is a bit different because of the markup
   //            capability in the UI. The messages in this log are preceeded
   //            by a priority code (given by the enums in koHelper)
   // 
   void SaveBroadcast(string message, int priority);
   //
   // Name     : Error(string message)
   // Function : Records message into the log file as an error
   // 
   void Error(string message);
   //
   // Name     : GetTimeString()
   // Function : Returns a string with the current date and time with form
   //            "%Y.%m.%d [%H:%M:%S] - "
   static string GetTimeString();   
  static u_int64_t GetTimeMus();
   //
   // Name     : GetCurrentTime()
   // Function : Returns a time_t object with the current time
   // 
   static time_t GetCurrentTime();
   //
   // Name     : SendMessage(string message)
   // Function : Put a message into the message buffer. Used to communicate 
   //            with the outside world if we are in an object without
   //            access to networking classes.
   // 
   void SendMessage(string message)  {
      fMessageBuffer.push_back(message);
   };
   //
   // Name     : GetMessages
   // Function : Get number of messages in message buffer
   // 
   int GetMessages(){
      return fMessageBuffer.size();
   };
   //
   // Name     : GetMessage()
   // Function : Get message at position 0 in message buffer
   // 
   string GetMessage();
   
   
 private: 
  ofstream fLogfile;   
   std::vector <string> fMessageBuffer;
   pthread_mutex_t fLogMutex;
};

#endif

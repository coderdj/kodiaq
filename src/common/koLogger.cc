// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : koLogger.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 27.06.2013
// Update  : 27.03.2014
// 
// Brief   : Log file creator and maintainer
// 
// *****************************************************

#include "koLogger.hh"

koLogger::koLogger()
{
  pthread_mutex_init(&fLogMutex,NULL);
}

koLogger::~koLogger()
{
   //close file if open!  
   if(fLogfile.is_open())
     fLogfile.close();
   pthread_mutex_destroy(&fLogMutex);
}

koLogger::koLogger(string logpath)
{
  pthread_mutex_init(&fLogMutex,NULL);
  SetLogfile(logpath);
}

void koLogger::SetLogfile(string logpath)
{
   //close file if open, then open new file at logpath
   if(fLogfile.is_open()) fLogfile.close();
   fLogfile.open(logpath.c_str(),ios::app);
   //   Message("Program (re)opened log file.");
}

void koLogger::Message(string message)
{
   if(!fLogfile.is_open()) return;
   string logmessage = GetTimeString();
   logmessage+=message;
   pthread_mutex_lock(&fLogMutex);
   fLogfile<<logmessage<<endl;   
   pthread_mutex_unlock(&fLogMutex);
}

void koLogger::Error(string message)
{
   if(!fLogfile.is_open()) return;
   string logmessage = GetTimeString();
   string error = " [!ERROR!] ";
   logmessage+=error; logmessage+=message;
   fLogfile<<logmessage<<endl;
}

string koLogger::GetTimeString()
{
   time_t rawtime;
   struct tm *timeinfo;
   char timestring[25];
   
   time(&rawtime);
   timeinfo = localtime(&rawtime);
   
   strftime(timestring,25,"%Y.%m.%d [%H:%M:%S] - ",timeinfo);
   string retstring(timestring);
   return retstring;
}

time_t koLogger::GetCurrentTime()
{
   time_t rawtime;
   time(&rawtime);
   return rawtime;
}

string koLogger::GetMessage()
{
   string temp = fMessageBuffer[0];
   fMessageBuffer.erase(fMessageBuffer.begin());
   return temp;
}

void koLogger::SaveBroadcast(string message,int priority)
{
   if(!fLogfile.is_open()) return;
   fLogfile<<priority<<" "<<GetTimeString()<<message<<endl;
   return;
}

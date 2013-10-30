// ****************************************************
// ****************************************************
// 
// File    : XeDAQLogger.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 27.6.2013
// 
// Brief   : Logger for Xenon-1t DAQ software
// 
// *****************************************************
// *****************************************************

#include "XeDAQLogger.hh"

XeDAQLogger::XeDAQLogger()
{
}

XeDAQLogger::~XeDAQLogger()
{
   //close file if open!
   if(fLogfile.is_open())
     fLogfile.close();
}

XeDAQLogger::XeDAQLogger(string logpath)
{
   SetLogfile(logpath);
}

void XeDAQLogger::SetLogfile(string logpath)
{
   //close file if open, then open new file at logpath
   if(fLogfile.is_open()) fLogfile.close();
   fLogfile.open(logpath.c_str(),ios::app);
}

void XeDAQLogger::Message(string message)
{
   if(!fLogfile.is_open()) return;
   string logmessage = GetTimeString();
   logmessage+=message;
   fLogfile<<logmessage<<endl;   
}

void XeDAQLogger::Error(string message)
{
   if(!fLogfile.is_open()) return;
   string logmessage = GetTimeString();
   string error = " [!ERROR!] ";
   logmessage+=error; logmessage+=message;
   fLogfile<<logmessage<<endl;
}

string XeDAQLogger::GetTimeString()
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

time_t XeDAQLogger::GetCurrentTime()
{
   time_t rawtime;
   time(&rawtime);
   return rawtime;
}

string XeDAQLogger::GetMessage()
{
   string temp = fMessageBuffer[0];
   fMessageBuffer.erase(fMessageBuffer.begin());
   return temp;
}

void XeDAQLogger::SaveBroadcast(string message,int priority)
{
   if(!fLogfile.is_open()) return;
   fLogfile<<priority<<" "<<GetTimeString()<<message<<endl;
   return;
}

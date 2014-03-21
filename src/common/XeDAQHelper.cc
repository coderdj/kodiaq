// ******************************************************
// 
// DAQ Control for Xenon-1t
// 
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 03.07.2013
// File      : XeDAQHelper.cc
// 
// Brief     : Collection of helpful functions for use
//             throughout the DAQ software
//             
// ******************************************************
#include "XeDAQHelper.hh"
#include <iostream>
XeDAQHelper::XeDAQHelper()
{}

XeDAQHelper::~XeDAQHelper()
{}

bool XeDAQHelper::ParseMessageFull(const int pipe,vector<string> &data, 
					  char &command)    
{
   char temp;
   if(read(pipe,&temp,1)<0) return false;
   command=temp;
   if(!ParseMessage(pipe,data)) return false;
   return true;
}

bool XeDAQHelper::ParseMessage(const int pipe,vector<string> &data)
{
   char chin;
   if(read(pipe,&chin,1)<0 || chin!='!') 
     return false;
   string tempString;
   do  {
      if(read(pipe,&chin,1)<0)	
	return false;
      if(chin=='!' || chin=='@')	{
	 if(tempString.size()==0)  
	   return false;	 
	 data.push_back(tempString);
	 tempString.erase();	 
      }
      else 
	tempString.push_back(chin);      
   }while(chin!='@');
   
   return true;   
}

bool XeDAQHelper::ComposeMessage(string &message,vector <string> data,
					char type)
{
   message=type;
   char delimiter='!';
   char ender='@';
   for(unsigned int x=0;x<data.size();x++)  {
      message+=delimiter;
      message+=data[x];
   }
   message+=ender;
   return true;   
}

bool XeDAQHelper::MessageOnPipe(int pipe)
{
   struct timeval timeout; //we will define how long to listen
   fd_set readfds;
   timeout.tv_sec=0;
   timeout.tv_usec=0;
   FD_ZERO(&readfds);
   FD_SET(pipe,&readfds);
   select(/*pipe+1*/FD_SETSIZE,&readfds,NULL,NULL,&timeout);
   if(FD_ISSET(pipe,&readfds)==1) return true;
   else return false;
//   return (FD_ISSET(pipe,&readfds));   
}


u_int32_t XeDAQHelper::StringToInt(const string &str)
{
   stringstream ss(str);
   u_int32_t result;
   return ss >> result ? result : 0;
}

u_int32_t XeDAQHelper::StringToHex(const string &str)
{
   stringstream ss(str);
   u_int32_t result;
   return ss >> std::hex >> result ? result : 0;
}

double XeDAQHelper::StringToDouble(const string &str)
{
   stringstream ss(str);
   double result;
   return ss >> result ? result : -1.;
}

string XeDAQHelper::IntToString(const int num)
{
   ostringstream convert;
   convert<<num;
   return convert.str();
}

string XeDAQHelper::DoubleToString(const double num)
{
   ostringstream convert;
   convert<<num;
   return convert.str();
}

void XeDAQHelper::InitializeStatus(XeStatusPacket_t &Status)
{
   Status.NetworkUp=false;
   Status.DAQState=XEDAQ_IDLE;
   Status.Slaves.clear();
   Status.RunMode="None";
   Status.RunModeLabel="None";
   Status.NumSlaves=0;
   return;
}

string XeDAQHelper::GetRunNumber()
{
   time_t now = XeDAQLogger::GetCurrentTime();
   struct tm *timeinfo;
   char idstring[25];
   
   timeinfo = localtime(&now);
   strftime(idstring,25,"data_%y%m%d_%H%M",timeinfo);
   string retstring(idstring);
   return retstring;
}

int XeDAQHelper::CurrentTimeInt()
{
   time_t now = XeDAQLogger::GetCurrentTime();
   struct tm *timeinfo;
   timeinfo = localtime(&now);
   char yrmdhr[10];
   strftime(yrmdhr,10,"%y%m%d%H",timeinfo);
   string timestring((const char*)yrmdhr);
   return StringToInt(timestring);
}

void XeDAQHelper::InitializeNode(XeNode_t &node)
{
   node.status=node.ID=node.nBoards=0;
   node.Rate=node.Freq=0.;
   node.name="";
   node.lastUpdate=XeDAQLogger::GetCurrentTime();
}

void XeDAQHelper::ProcessStatus(XeStatusPacket_t &Status)
{
   Status.DAQState=XEDAQ_IDLE;
   unsigned int nArmed=0,nRunning=0, nIdle=0;
   for(unsigned int x=0;x<Status.Slaves.size();x++)  {
      if(Status.Slaves[x].status==XEDAQ_ARMED) nArmed++;
      if(Status.Slaves[x].status==XEDAQ_RUNNING) nRunning++;
      if(Status.Slaves[x].status==XEDAQ_IDLE) nIdle++;
   }
   if(nRunning==Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=XEDAQ_RUNNING;
   if(nArmed==Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=XEDAQ_ARMED;
   if(nIdle == Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=XEDAQ_IDLE;
   else if(Status.Slaves.size()!=0 && Status.DAQState==XEDAQ_IDLE) Status.DAQState=XEDAQ_MIXED;
   return;
}

int XeDAQHelper::InitializeRunInfo(XeRunInfo_t &fRunInfo)
{
   ifstream infile;
   infile.open(fRunInfo.RunInfoPath.c_str());
   if(!infile) {
      fRunInfo.RunNumber=" ";
      fRunInfo.StartedBy=" ";
      fRunInfo.StartDate=" ";
      return -1;
   }   
   string temp;
   getline(infile,fRunInfo.RunNumber);
   getline(infile,fRunInfo.StartedBy);
   getline(infile,fRunInfo.StartDate);
   infile.close();
   return 0;
}

string XeDAQHelper::MakeDBName(XeRunInfo_t RunInfo, string CollectionName)
{
   std::size_t pos;
   pos=CollectionName.find_first_of(".",0);
   string retstring = CollectionName.substr(0,pos);
   retstring+=".";
   retstring+=RunInfo.RunNumber;
   return retstring;
}

int XeDAQHelper::UpdateRunInfo(XeRunInfo_t &fRunInfo, string startedby)
{
   ofstream outfile;
   outfile.open(fRunInfo.RunInfoPath.c_str());
   if(!outfile) return -1;
   fRunInfo.RunNumber=XeDAQHelper::GetRunNumber();
   fRunInfo.StartedBy=startedby;
   fRunInfo.StartDate=XeDAQLogger::GetTimeString();
   fRunInfo.StartDate.resize(fRunInfo.StartDate.size()-2);
   outfile<<fRunInfo.RunNumber<<endl<<fRunInfo.StartedBy<<endl<<fRunInfo.StartDate<<endl;
   outfile.close();
   return 0;
}

int XeDAQHelper::EasyPassHash(string pass)
{
   int retVal=0;
   for(unsigned int x=0;x<pass.size();x++){
      retVal+=(int)pass[x];
   }
   return retVal;
}


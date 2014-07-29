// ******************************************************
// 
// kodaiq Data Acquisition Software
// 
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 03.07.2013
// Updated   : 27.03.2014
// File      : koHelper.cc
// 
// Brief     : Collection of helpful functions for use
//             throughout the DAQ software
//             
// ******************************************************
#include "koHelper.hh"
#include <iostream>
koHelper::koHelper()
{}

koHelper::~koHelper()
{}

u_int32_t koHelper::StringToInt(const string &str)
{
   stringstream ss(str);
   u_int32_t result;
   return ss >> result ? result : 0;
}

u_int32_t koHelper::StringToHex(const string &str)
{
   stringstream ss(str);
   u_int32_t result;
   return ss >> std::hex >> result ? result : 0;
}

double koHelper::StringToDouble(const string &str)
{
   stringstream ss(str);
   double result;
   return ss >> result ? result : -1.;
}

string koHelper::IntToString(const int num)
{
   ostringstream convert;
   convert<<num;
   return convert.str();
}

string koHelper::DoubleToString(const double num)
{
   ostringstream convert;
   convert<<num;
   return convert.str();
}

void koHelper::InitializeStatus(koStatusPacket_t &Status)
{
   Status.NetworkUp=false;
   Status.DAQState=KODAQ_IDLE;
   Status.Slaves.clear();
   Status.RunMode="None";
   Status.RunModeLabel="None";
   Status.NumSlaves=0;
   return;
}

string koHelper::GetRunNumber()
{
   time_t now = koLogger::GetCurrentTime();
   struct tm *timeinfo;
   char idstring[25];
   
   timeinfo = localtime(&now);
   strftime(idstring,25,"data_%y%m%d_%H%M",timeinfo);
   string retstring(idstring);
   return retstring;
}

int koHelper::CurrentTimeInt()
{
   time_t now = koLogger::GetCurrentTime();
   struct tm *timeinfo;
   timeinfo = localtime(&now);
   char yrmdhr[10];
   strftime(yrmdhr,10,"%y%m%d%H",timeinfo);
   string timestring((const char*)yrmdhr);
   return StringToInt(timestring);
}

void koHelper::InitializeNode(koNode_t &node)
{
   node.status=node.ID=node.nBoards=0;
   node.Rate=node.Freq=0.;
   node.name="";
   node.lastUpdate=koLogger::GetCurrentTime();
}

void koHelper::ProcessStatus(koStatusPacket_t &Status)
{
   Status.DAQState=KODAQ_IDLE;
   unsigned int nArmed=0,nRunning=0, nIdle=0;
   for(unsigned int x=0;x<Status.Slaves.size();x++)  {
      if(Status.Slaves[x].status==KODAQ_ARMED) nArmed++;
      if(Status.Slaves[x].status==KODAQ_RUNNING) nRunning++;
      if(Status.Slaves[x].status==KODAQ_IDLE) nIdle++;
      if(Status.Slaves[x].status==KODAQ_ERROR) {
	Status.DAQState=KODAQ_ERROR;
	return;
      }
   }
   if(nRunning==Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=KODAQ_RUNNING;
   if(nArmed==Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=KODAQ_ARMED;
   if(nIdle == Status.Slaves.size() && Status.Slaves.size()!=0) Status.DAQState=KODAQ_IDLE;
   else if(Status.Slaves.size()!=0 && Status.DAQState==KODAQ_IDLE) Status.DAQState=KODAQ_MIXED;
   return;
}

int koHelper::InitializeRunInfo(koRunInfo_t &fRunInfo)
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

string koHelper::MakeDBName(koRunInfo_t RunInfo, string CollectionName)
{
   std::size_t pos;
   pos=CollectionName.find_first_of(".",0);
   string retstring = CollectionName.substr(0,pos);
   retstring+=".";
   retstring+=RunInfo.RunNumber;
   return retstring;
}

int koHelper::UpdateRunInfo(koRunInfo_t &fRunInfo, string startedby)
{
   ofstream outfile;
   outfile.open(fRunInfo.RunInfoPath.c_str());
   if(!outfile) return -1;
   fRunInfo.RunNumber=koHelper::GetRunNumber();
   fRunInfo.StartedBy=startedby;
   fRunInfo.StartDate=koLogger::GetTimeString();
   fRunInfo.StartDate.resize(fRunInfo.StartDate.size()-2);
   outfile<<fRunInfo.RunNumber<<endl<<fRunInfo.StartedBy<<endl<<fRunInfo.StartDate<<endl;
   outfile.close();
   return 0;
}

u_int32_t koHelper::GetTimeStamp(u_int32_t *buffer)
//Pull a time stamp out of a CAEN header                                           
{
  int pnt = 0;
  while((buffer[pnt])==0xFFFFFFFF) //filler between events                          
    pnt++;
  if((buffer[pnt]>>20)==0xA00)   { //look for a header                              
    pnt+=3;
    return (buffer[pnt] & 0x7FFFFFFF);
  }
  return 0;
}


int koHelper::EasyPassHash(string pass)
{
   int retVal=0;
   for(unsigned int x=0;x<pass.size();x++){
      retVal+=(int)pass[x];
   }
   return retVal;
}


// ***************************************************************
// 
// DAQ Control for Xenon-1t 
// 
// File      : XeDAQRecorder.hh
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 9.8.2013
// 
// Brief     : Base Class for data output
// 
// ****************************************************************

#include <iostream>
#include "XeDAQRecorder.hh"

XeDAQRecorder::XeDAQRecorder()
{
   fWriteMode=-1;
   bTimeOverTen=false;
   fID=0;
   ResetError(); 
}

XeDAQRecorder::~XeDAQRecorder()
{   
}

int XeDAQRecorder::GetID(u_int32_t currentTime)
{
   int a=GetCurPrevNext(currentTime);
   if(a==1) fID++;
   if(a==-1) return fID-1;
   return fID;
}

int XeDAQRecorder::GetCurPrevNext(u_int32_t timestamp)
{
//For just a counter telling how many times the thing has reset
   if(timestamp<1E8 && bTimeOverTen==true)  {
      bTimeOverTen=false;
      return 1;
   }
   else if(timestamp>1E9 && bTimeOverTen==false)  {
      bTimeOverTen=true;
      return 0;
   }
   else if(timestamp>2E9 && bTimeOverTen==false)
     return -1;
   return 0;   
}

bool XeDAQRecorder::QueryError(string &err)
{
   if(bErrorSet==false) return false;
   err=fErrorText;
   fErrorText="";
   bErrorSet=false;
   return true;
}

void XeDAQRecorder::LogError(string err)
{
   fErrorText=err;
   bErrorSet=true;
   return;   
}

void XeDAQRecorder::ResetError()
{
   bErrorSet=false;
   fErrorText="";
}




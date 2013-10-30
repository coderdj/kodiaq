#ifndef _XEDAQRECORDER_HH_
#define _XEDAQRECORDER_HH_

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


#include <XeDAQLogger.hh>
#include <XeDAQOptions.hh>
#include "CBV1724.hh"

using namespace std;

extern XeDAQLogger *gLog;

class XeDAQRecorder
{
 public:
   XeDAQRecorder();
   virtual ~XeDAQRecorder();
   
   virtual int  Initialize(XeDAQOptions *options) = 0;
   virtual void ShutdownRecorder() = 0;
   int          GetID(u_int32_t currentTime);   
   bool         QueryError(string &err);
      
 protected:
   int           fWriteMode;                 // Write mode (0) none (1) file (2) mongo   
   void          LogError(string err);
   void          ResetError();
   
 private:
   int           GetCurPrevNext(u_int32_t timestamp);
   int           fID;
   bool          bTimeOverTen;
   bool          bErrorSet;
   string        fErrorText;
};

#endif
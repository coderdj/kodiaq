#ifndef _XEPROCESSOR_HH_
#define _XEPROCESSOR_HH_

// *************************************************************** 
//  
// DAQ Control for Xenon-1t  
//  
// File      : XeProcessor.hh 
// Author    : Daniel Coderre, LHEP, Universitaet Bern 
// Date      : 19.9.2013 
//  
// Brief     : Intermediate data processor/router 
//  
// ****************************************************************

#include "XeMongoRecorder.hh"
//#include "DigiInterface.hh"

using namespace std;
class DigiInterface;

class XeProcessor
{
 public:
                  XeProcessor();
   virtual       ~XeProcessor();
   explicit       XeProcessor(DigiInterface *digi, XeMongoRecorder *recorder);
   
   static void*   WProcessMongoDB(void* data);
   
   void           ProcessMongoDB();

   void           ProcessData(vector <u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec);
   void           ProcessDataChannels(vector<u_int32_t*> *&buffvec,
				      vector<u_int32_t> *&sizevec,
				      vector <u_int32_t> *timestamps,
				      vector <u_int32_t> *channels);
   bool           QueryError(string &err);
      
   
 private:   
   DigiInterface *fDigiInterface;
   XeMongoRecorder *fDAQRecorder;
   void           LogError(string err);
   bool           bErrorSet;
   string         fErrorText;
   u_int32_t      fBLTLen;
   u_int32_t      GetTimeStamp(u_int32_t *buffer);
   int            fMinInsertSize;
   int            fBlockSplitting;
};

#endif
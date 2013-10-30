#ifndef _DIGIINTERFACE_HH_
#define _DIGIINTERFACE_HH_

// *****************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : DigiInterface.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.07.2013
// 
// Brief    : Class for managing electronics
// 
// *****************************************************

#include <XeDAQLogger.hh>
#include <pthread.h>
#include "VMECrate.hh"
#include "XeProcessor.hh"

using namespace std;

struct XeThreadType
{
   pthread_t Thread;
   bool IsOpen;
};
struct XeProcThread
{
   pthread_t Thread;
   bool IsOpen;
   XeProcessor *XeP;
};

class DigiInterface
{
 public:
   DigiInterface();
   virtual ~DigiInterface();
   
   //General Control
   int                  Initialize(XeDAQOptions *options);
   int                  StartRun();
   int                  StopRun();   
   u_int32_t            GetRate(u_int32_t &freq);
   void                 Close();
   bool                 RunError(string &err);
   
   //Access to crates and boards
   CBV1724*             operator()(unsigned int i, unsigned int j) {
      return (CBV1724*)(fCrates[i]->GetDigitizer(j));
   };
   int                  NumCrates()  {    
      return fCrates.size();
   };
   VMECrate*            GetCrate(int x)  {
      return fCrates[x];
   };   
   NIMBoard*            GetModuleByID(int moduleID);
   unsigned int         GetDigis();   

   //For write thread
   static void*         ReadThreadWrapper(void* digi);
   void                 ReadThread();   
   
 private:
   void                 CloseThreads(bool Completely=false);
   bool                 LockRateMutex();
   bool                 UnlockRateMutex();
   
   vector<XeProcThread> fProcessingThreads;  
   XeThreadType         fReadThread;
   XeThreadType         fWriteThread;
   
   vector <VMECrate*>   fCrates;  
   NIMBoard*            fRunStartModule; //-1 for independent starting
   XeMongoRecorder       *fDAQRecorder;
   int                  fWriteMode;   
   
   unsigned int         fReadSize;
   unsigned int         fReadFreq;
   pthread_mutex_t      fRateLock;
};

#endif

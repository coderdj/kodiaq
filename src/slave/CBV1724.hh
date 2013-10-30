#ifndef _CBV1724_HH_
#define _CBV1724_HH_

// ****************************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : CBV1724.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 12.07.2013
// 
// Brief    : Class for Caen V1724 digitizers
// 
// *****************************************************************

#include "NIMBoard.hh"
#include <pthread.h>

//Register definitions
#define V1724_BoardInfoReg                0x8140
#define V1724_BlockOrganizationReg        0x800C
#define V1724_BltEvNumReg                 0xEF1C
#define V1724_AcquisitionControlReg       0x8100
#define V1724_DACReg                      0x1098

class CBV1724 : public NIMBoard {
 public: 
   CBV1724();
   virtual ~CBV1724();
   explicit CBV1724(BoardDefinition_t BoardDef){
      fBID=BoardDef;
   };   

   int Initialize(XeDAQOptions *options);   
   unsigned int ReadMBLT();   
   BoardDefinition_t GetBoardDef()  {
      return fBID;
   };  
   
   vector<u_int32_t*>* ReadoutBuffer(vector <u_int32_t> *&sizes);
   
   void ResetBuff();
   u_int32_t GetBLTSize()  {
      return fBLTSize;
   };

   int LockDataBuffer();
   int UnlockDataBuffer();
   int RequestDataLock();
   
   int DetermineBaselines();
   void SetActivated(bool active);
   
 private:
   int                  LoadBaselines();                       //Load baselines to boards
   int                  GetBaselines(vector <int> &baselines, bool bQuiet=false);  //Get baselines from file 

   unsigned int         fReadoutThresh;
   pthread_mutex_t      fDataLock;
   pthread_cond_t       fReadyCondition;
   u_int32_t            fBufferSize;
   u_int32_t            fBLTSize;
   vector <u_int32_t>  *fSizes;
   vector <u_int32_t*> *fBuffers;
};

#endif

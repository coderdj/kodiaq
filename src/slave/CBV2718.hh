#ifndef _CBV2718_HH_
#define _CBV2718_HH_

// ************************************************************
// 
// DAQ Control for Xenon-1t
// 
// File          : CBV2718.hh
// Author        : Daniel Coderre, LHEP, Universitaet Bern
// Date          : 19.07.2013
// 
// Brief         : Class for Caen V2718 crate controllers
// 
// ************************************************************

#include "NIMBoard.hh"

class CBV2718 : public NIMBoard {
 public:
   CBV2718();
   virtual ~CBV2718();
   explicit CBV2718(BoardDefinition_t BID)  {
      fBID=BID;
   };   
   
   int Initialize(XeDAQOptions *options);
   int SendStartSignal();
   int SendStopSignal();
   
 private:
};
#endif

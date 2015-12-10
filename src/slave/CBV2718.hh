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

#include "VMEBoard.hh"

/*! \brief Control class for CAEN V2718 crate controllers. 
 */ 
class CBV2718 : public VMEBoard {
 public:
   CBV2718();
   virtual ~CBV2718();
   explicit CBV2718(board_definition_t BID, koLogger *kLog);
   
   int Initialize(koOptions *options);
   int SendStartSignal();
   int SendStopSignal();
   
 private:

  bool b_startwithsin;
  bool b_led_on;
  bool b_muonveto_on;
  int  i_pulserHz;
  bool bStarted;
};
#endif

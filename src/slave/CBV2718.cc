// *********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File        : CBV2718.cc
// Author      : Daniel Coderre, LHEP, Universitaet Bern
// Date        : 19.07.2013
// 
// Brief       : Class for Caen V2718 crate controllers
// 
// **********************************************************

#include <iostream>
#include "CBV2718.hh"

CBV2718::CBV2718()
{
  b_startwithsin = false;
  b_led_on = false;
  b_muonveto_on = false;
}

CBV2718::~CBV2718()
{
}

CBV2718::CBV2718(board_definition_t BID, koLogger *kLog)
        :VMEBoard(BID,kLog)
{
  b_startwithsin = false;
  b_led_on = false;
  b_muonveto_on = false;
}

int CBV2718::Initialize(koOptions *options)
{
   //Set output multiplex register to channels 0-4 to configure them to 
   //output whatever is written to the output buffer
  
  b_startwithsin = false;
  b_led_on = false;
  b_muonveto_on = false;
  if(options->led_trigger)
    b_led_on = true;
  if(options->muon_veto)
    b_muonveto_on = true;
   unsigned int data = 0x3FF; 
   if(CAENVME_WriteRegister(fCrateHandle,cvOutMuxRegSet,data)!=cvSuccess)
     cout<<"Can't write to CC!"<<endl;
   if(SendStopSignal()!=0)
     cout<<"Stop signal failed."<<endl;
   return 0;
}

int CBV2718::SendStartSignal()
{

   //configure line 3 to pulse (to start pulser) 
   CAENVME_SetOutputConf(fCrateHandle,cvOutput3,cvDirect,
			 cvActiveHigh,cvMiscSignals); 
   CAENVME_SetPulserConf(fCrateHandle,cvPulserB,0x3E8,0x64,
			 cvUnit25ns,1,
			 cvManualSW,cvManualSW);
   
   unsigned int data = 0x7C0;
   if(b_muonveto_on && ! b_led_on) // Set output 1 to high
     data = 0x6C0;
   else if(b_led_on && ! b_muonveto_on)
     data = 0x740;
   else if(!b_led_on && !b_muonveto_on)
     data = 0x640;
  
   // This is the S-IN
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)
     return -1;
   
   usleep(1000);
   //now start the pulser 
   CAENVME_StartPulser(fCrateHandle,cvPulserB);    
   
   return 0;
}

int CBV2718::SendStopSignal()
{
   u_int16_t data = 0x7C0;
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegClear,data)!=0)  
     return -1;
   return 0;   
}



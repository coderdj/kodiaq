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
  i_pulserHz = 0;
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
  i_pulserHz = 0;
}

int CBV2718::Initialize(koOptions *options)
{
   //Set output multiplex register to channels 0-4 to configure them to 
   //output whatever is written to the output buffer
  
  /*
    // Output is set as follows
    // Output_0: S-IN
    // Output_1: muon_veto_on
    // Output_2: trigger_on
    // Output_3: LED pulser
  */
  b_startwithsin = false;
  b_led_on = false;
  b_muonveto_on = false;
  i_pulserHz = 0;
  
  unsigned int data = 0x0; //new
  if(options->led_trigger){
    b_led_on = true;
    data+=0x30;//new
  }
  if(options->muon_veto){
    b_muonveto_on = true;
    data+=0xC;
  }
  if(options->run_start==1){ //all new if
    b_startwithsin = true;
    data+=0x3;
  }
  if(options->pulser_freq>0){ //all new if
    i_pulserHz = options->pulser_freq;
    data+=0x80;
  }
    
  //unsigned int data = 0x3FF; 
   if(CAENVME_WriteRegister(fCrateHandle,cvOutMuxRegSet,data)!=cvSuccess)
     cout<<"Can't write to CC!"<<endl;
   if(SendStopSignal()!=0)
     cout<<"Stop signal failed."<<endl;
   return 0;
}

int CBV2718::SendStartSignal()
{

   //configure line 3 to pulse (to start pulser) 
  if(i_pulserHz > 0){
    CAENVME_SetOutputConf(fCrateHandle,cvOutput3,cvDirect,
			  cvActiveHigh,cvMiscSignals); 
    CAENVME_SetPulserConf(fCrateHandle,cvPulserB,0x3E8,0x64,
			  cvUnit25ns,1,
			  cvManualSW,cvManualSW);
  }
  
  // Set the output register
  unsigned int data = 0x0;
  if(b_led_on)
    data+=0x100;
  if(b_muonveto_on)
    data+=0x80;
  if(b_startwithsin)
    data+=0x40;
  if(i_pulserHz>0)
    data+=0x200;
  /* unsigned int data = 0x7C0;
   if(b_muonveto_on && ! b_led_on) // Set output 1 to high
     data = 0x6C0;
   else if(b_led_on && ! b_muonveto_on)
     data = 0x740;
   else if(!b_led_on && !b_muonveto_on)
     data = 0x640;
  */
   // This is the S-IN
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)
     return -1;
   
   // configure the pulser
   // right now hardcode 100 kHz because it's a pain in the ass to calculate arbitrary periods
   // set width = 16 mus = 10*1.6 mus
   // set range = 01 = 1.6 mus
   // set period = 10*1.6 = 16 mus = 62.5 kHz
   // So B1 register = range 01, NUM_PULSES = 1111111
   data = 0x011111111;
   if(CAENVME_WriteRegister(fCrateHandle,cvPulserB1,data)!=0)
     return -1;
   
   data = 0xA;
   data+= 0x200;
   if(CAENVME_WriteRegister(fCrateHandle,cvPulserB0,data)!=0)
     return -1;
   usleep(1000);

   //now start the pulser 
   if(i_pulserHz > 0)
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



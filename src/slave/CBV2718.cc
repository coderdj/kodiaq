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
#include <koHelper.hh>
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
  
  unsigned int data = 0x7C00; //new
  if(options->GetInt("led_trigger")==1){
    b_led_on = true;
    data+=0x30;//new
  }
  if(options->GetInt("muon_veto")==1){
    b_muonveto_on = true;
    data+=0xC;
  }
  if(options->GetInt("run_start")==1){ //all new if
    b_startwithsin = true;
    data+=0x3;
  }
  if(options->GetInt("pulser_freq")>0){ //all new if
    i_pulserHz = options->GetInt("pulser_freq");
    data+=0x80;
  }

  m_koLog->Message("Writing cvOutMuxRegSet with :" + 
		   koHelper::IntToString(data) );
  
  //unsigned int data = 0x3FF; 
   if(CAENVME_WriteRegister(fCrateHandle,cvOutMuxRegSet,data)!=cvSuccess)
     m_koLog->Error("Can't write to crate controller!");

   // DAQ test emergency configure
   if(CAENVME_SetOutputConf(fCrateHandle, cvOutput0, cvDirect, cvActiveLow,
			    cvManualSW)!=0)
     m_koLog->Error("Can't write to outputconf directly");
   //cout<<"Can't write to CC!"<<endl;
   if(SendStopSignal()!=0)
     m_koLog->Error("Sending stop signal failed in CBV2718");
   //cout<<"Stop signal failed."<<endl;
   return 0;
}

int CBV2718::SendStartSignal()
{

  //configure line 3 to pulse (to start pulser) 
  if(i_pulserHz > 0){
    CAENVME_SetOutputConf(fCrateHandle,cvOutput3,cvInverted,//cvDirect,
			  cvActiveLow,cvMiscSignals); 

    // i_pulserHz gives number of 1.6 mus units per pulse period.	
    // min is 2 (because the period has a min active high of 1 unit)	
    // max is 652000, corresponds to 1 Hz
    if(i_pulserHz<2) i_pulserHz =2;
    else if(i_pulserHz > 625000) i_pulserHz = 625000;
    CAENVME_SetPulserConf(fCrateHandle,cvPulserB,i_pulserHz,0x1,
			  cvUnit1600ns,0,
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
    data+=0x4;//0x200;
  /* unsigned int data = 0x7C0;
   if(b_muonveto_on && ! b_led_on) // Set output 1 to high
     data = 0x6C0;
   else if(b_led_on && ! b_muonveto_on)
     data = 0x740;
   else if(!b_led_on && !b_muonveto_on)
     data = 0x640;
  */

   // This is the S-IN
  //  cout<<"Writing cvOutRegSet with :"<<dec<<data<<endl;
  //if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)
  //return -1;
   
   // configure the pulser
   // right now hardcode 100 kHz because it's a pain in the ass to calculate arbitrary periods
   // set width = 16 mus = 10*1.6 mus
   // set range = 01 = 1.6 mus
   // set period = 10*1.6 = 16 mus = 62.5 kHz
   // So B1 register = range 01, NUM_PULSES = 1111111
  /*data = 0x1FF;
   if(CAENVME_WriteRegister(fCrateHandle,cvPulserB1,data)!=0)
     return -1;
   
   data = 0xA;
   data+= 0x200;
   if(CAENVME_WriteRegister(fCrateHandle,cvPulserB0,data)!=0)
     return -1;
   usleep(1000);
  */
   // This is the S-IN                
  m_koLog->Message("Writing cvOutRegSet with :" + 
		   koHelper::IntToString( data ) );
  //if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)    
  //return -1; 
  if(CAENVME_SetOutputRegister(fCrateHandle,data)!=0){
    m_koLog->Error("Could't set output register. Crate Controller not found.");
    return -1;
  }
  //now start the pulser 
   if(i_pulserHz > 0)
     CAENVME_StartPulser(fCrateHandle,cvPulserB);    
   
   return 0;
}

int CBV2718::SendStopSignal()
{
  u_int16_t data = 0x7C8;
  //u_int16_t data = 0x7FF;
  //cout<<"Writing cvOutRegClear with: "<<data<<endl;
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegClear,data)!=0)  
     return -1;
   return 0;   
}



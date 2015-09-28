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
  bStarted=false;
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
  bStarted=false;
}

int CBV2718::Initialize(koOptions *options)
{
   //Set output multiplex register to channels 0-4 to configure them to 
   //output whatever is written to the output buffer
  
  /*
    // Output is set as follows
    // Output_0: S-IN
    // Output_1: muon_veto_logic
    // Output_2: led_trigger_logic
    // Output_3: LED pulser
  */
  b_startwithsin = false;
  b_led_on = false;
  b_muonveto_on = false;
  i_pulserHz = 0;
  
  if(options->GetInt("led_trigger")==1)
    b_led_on = true;    
  if(options->GetInt("muon_veto")==1)
    b_muonveto_on = true;
  if(options->GetInt("run_start")==1)
    b_startwithsin = true;
  if(options->GetInt("pulser_freq")>0)
    i_pulserHz = options->GetInt("pulser_freq");
  
  
  //CAENVME_SystemReset(fCrateHandle);
  if(SendStopSignal()!=0)
    m_koLog->Error("Sending stop signal failed in CBV2718");
   //cout<<"Stop signal failed."<<endl;
   return 0;
}

int CBV2718::SendStartSignal()
{

  // Set the output lines

  // Line 0 : S-IN. 
  CAENVME_SetOutputConf(fCrateHandle, cvOutput0, cvDirect, 
			cvActiveHigh, cvManualSW);
  // Line 1 : MV S-IN Logic
  CAENVME_SetOutputConf(fCrateHandle, cvOutput1, cvDirect, 
			cvActiveHigh, cvManualSW);
  // Line 2 : LED Logic
  CAENVME_SetOutputConf(fCrateHandle, cvOutput2, cvDirect, 
			cvActiveHigh, cvManualSW);
  // Line 3 : LED Pulser
  CAENVME_SetOutputConf(fCrateHandle, cvOutput3, cvDirect, 
			cvActiveHigh, cvMiscSignals);

  // Set the output register
  unsigned int data = 0x0;
  if(b_led_on)
    data+=cvOut2Bit;
  if(b_muonveto_on)
    data+=cvOut1Bit;
  if(b_startwithsin)
    data+=cvOut0Bit;

   // This is the S-IN                
  m_koLog->Message("Writing cvOutRegSet with :" + 
		   koHelper::IntToString( data ) );
  //if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)    
  //return -1; 
  if(CAENVME_SetOutputRegister(fCrateHandle,data)!=0){
    m_koLog->Error("Could't set output register. Crate Controller not found.");
    return -1;
  }
  bStarted=true;
  //Configure the LED pulser
  if(i_pulserHz > 0){
    // We allow a range from 1Hz to 1MHz, but this is not continuous!
    // If the number falls between two possible ranges it will be rounded to 
    // the maximum value of the lower one
    CVTimeUnits tu = cvUnit104ms;
    u_int32_t width = 0x1;
    u_int32_t period = 0x0;
    if(i_pulserHz < 10){
      if(i_pulserHz > 5)
	period = 0xFF;
      else
	period = (u_int32_t)((1000/104) / i_pulserHz);
    }
    else if(i_pulserHz < 2450){
      tu = cvUnit410us;
      if(i_pulserHz >1219)
	period = 0xFF;
      else
	period = (u_int32_t)((1000000/410) / i_pulserHz);
    }
    else if(i_pulserHz < 312500){
      tu = cvUnit1600ns;
      period = (u_int32_t)((1000000/1.6) / i_pulserHz);
    }
    else if(i_pulserHz < 20000000){
      tu = cvUnit25ns;
      period = (u_int32_t)((1E9/25)/i_pulserHz);
    }
    else
      m_koLog->Error("Invalid LED frequency set!");
  
    cout<<"Writing period with: "<<period<<" and width with "<<width<<endl;
    
    // Send data to the board
    CAENVME_SetPulserConf(fCrateHandle, cvPulserB, period, width, tu, 0,
			  cvManualSW, cvManualSW);
    CAENVME_StartPulser(fCrateHandle,cvPulserB);    
  }
   return 0;
}

int CBV2718::SendStopSignal()
{
  bStarted = false;
  // Stop the pulser if it's running
  CAENVME_StopPulser(fCrateHandle, cvPulserB);

  //u_int16_t data = 0x7C8;
  //u_int16_t data = 0x7FF;
  //cout<<"Writing cvOutRegClear with: "<<data<<endl;

  // Line 0 : S-IN.
  CAENVME_SetOutputConf(fCrateHandle, cvOutput0, cvDirect, cvActiveHigh, cvManualSW);
  // Line 1 : MV S-IN Logic 
  CAENVME_SetOutputConf(fCrateHandle, cvOutput1, cvDirect, cvActiveHigh, cvManualSW);
  // Line 2 : LED Logic
  CAENVME_SetOutputConf(fCrateHandle, cvOutput2, cvDirect, cvActiveHigh, cvManualSW);
  // Line 3 : LED Pulser
  CAENVME_SetOutputConf(fCrateHandle, cvOutput3, cvDirect, cvActiveHigh, cvMiscSignals);

  // Set the output register                                                                                                                       
  unsigned int data = 0x0;
  if(b_led_on)
    data+=cvOut2Bit;
  if(b_muonveto_on)
    data+=cvOut1Bit;
  if(b_startwithsin)
    data+=cvOut0Bit;

  CAENVME_ClearOutputRegister(fCrateHandle,data);
  
  //   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegClear,data)!=0)  
  //     return -1;
   return 0;   
}



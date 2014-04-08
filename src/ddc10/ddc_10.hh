#ifndef _DDC_10_HH_
#define _DDC_10_HH_

// *************************************************************
// 
// DAQ Control for Xenon-1t
// 
// File    : DDC_10.hh
// Author  : Lukas Buetikofer, LHEP, Universitaet Bern
// Date    : 3.12.2013
// 
// Brief   : Class for comunication with DDC-10 board (HE veto)
// 
// ************************************************************

#include <stdio.h>
#include <string>
#include <cstring>
//#include "parstruct.hh"

using namespace std;


struct ddc10_par_t{
   string IPAddress;	//IPAddress of DDC-10 board
   int Sign;		//sign of DDC-10 input signal (0=positive, 1=negative)
   int IntWindow;	//length of integration window [10 ns]
   int VetoDelay;	//delay of veto signal [10 ns]
   int SigThreshold;	//signal threshold [ADC counts]
   int IntThreshold;	//integration threshold [ADC counts]
   int WidthCut;	//cut for width discrimination [10 ns]
   int RiseTimeCut;	//cut for rise time discrimination [10 ns]
   int ComponentStatus;	//turn integral (ob100), width (0b010) and risetime (0b001) component ON or OFF (0=ON, 1=OFF) 
   double Par[4];	//Parameters of 3rd order polynomial function, 2^-48 < Par1 < 2^15 - 1 (32767)!!!
   int OuterRingFactor;	//OuterRingFactor for radial position veto 16bit 2complement
   int InnerRingFactor;	//InnerRingFactor for radial position veto 16bit 2complement
   int PreScaling;	//PreScaling : every x veto event passes the veto
};


class ddc_10
{
 public:


   ddc_10();
   virtual ~ddc_10();

   // Initialize status of DDC-10 with all parameters of ddc10_par_t'
   // Returns 0=successful, 1=error
   int Initialize(ddc10_par_t IPAddress);

   // All status LEDs will flash for 5 s
   // This function is ment for testing the connection between DAQ and DDC-10
   int LEDTestFlash(string arg0);

 private:

};
#endif

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
}

CBV2718::~CBV2718()
{
}

int CBV2718::Initialize(XeDAQOptions *options)
{
   unsigned int data = 0x3FF;
   if(CAENVME_WriteRegister(fCrateHandle,cvOutMuxRegSet,data)!=cvSuccess)
     cout<<"Can't write to CC!"<<endl;
   if(SendStopSignal()!=0)
     cout<<"Stop signal failed."<<endl;
   return 0;
}

int CBV2718::SendStartSignal()
{
   unsigned int data = 0x7C0;
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegSet,data)!=0)
     return -1;
   return 0;
}

int CBV2718::SendStopSignal()
{
   u_int16_t data = 0x7C0;
   if(CAENVME_WriteRegister(fCrateHandle,cvOutRegClear,data)!=0)  
     return -1;
   return 0;   
}



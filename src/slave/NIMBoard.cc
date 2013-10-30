
// ******************************************************
// 
// DAQ Control for Xenon-1t
// 
// File    : NIMBoard.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 11.07.2013
// 
// Brief   : Base class for Caen NIM boards
// 
// *******************************************************

#include <iostream>
#include "NIMBoard.hh"

NIMBoard::NIMBoard()
{
   fCrateHandle=-1;
   bActivated=false;
}

NIMBoard::~NIMBoard()
{   
}

NIMBoard::NIMBoard(BoardDefinition_t BID)
{
   fBID=BID;   
   fCrateHandle=-1;
}

int NIMBoard::WriteReg32(u_int32_t address, u_int32_t data)
{
   if(CAENVME_WriteCycle(fCrateHandle,fBID.VMEAddress+address,
			  &data,cvA32_U_DATA,cvD32)!=cvSuccess)        
     return -1;
   return 0;   
}

int NIMBoard::ReadReg32(u_int32_t address, u_int32_t &data)
{
   u_int32_t temp;
   if(CAENVME_ReadCycle(fCrateHandle,fBID.VMEAddress+address,
			&temp,cvA32_U_DATA,cvD32)!=cvSuccess)      	
     return -1;      
   data=temp;
   return 0;
}

int NIMBoard::WriteReg16(u_int32_t address,u_int16_t data)
{
   if(CAENVME_WriteCycle(fCrateHandle,fBID.VMEAddress+address,
			 &data,cvA32_U_DATA,cvD16)!=cvSuccess)         		    
     return -1;
   return 0;
}

int NIMBoard::ReadReg16(u_int32_t address,u_int16_t &data)
{
   u_int16_t temp;
   if(CAENVME_ReadCycle(fCrateHandle,fBID.VMEAddress+address,
			&temp,cvA32_U_DATA,cvD16)!=cvSuccess)
     return -1;
   data=temp;
   return 0;
}

int NIMBoard::Initialize(XeDAQOptions *options)
{
   bActivated=false;
   return -1; //? or should this return 0
}


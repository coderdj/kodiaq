
// ******************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : VMEBoard.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 11.07.2013
// 
// Brief   : Base class for Caen VME boards
// 
// *******************************************************

#include <iostream>
#include "VMEBoard.hh"

VMEBoard::VMEBoard()
{
   fCrateHandle=-1;
   bActivated=false;
   bIsSumModule=false;
   m_koLog=NULL;
}

VMEBoard::~VMEBoard()
{   
}

VMEBoard::VMEBoard(board_definition_t BID, koLogger *koLog)
{
   fBID=BID;   
   fCrateHandle=-1;
   m_koLog = koLog;
}

void VMEBoard::LogError(string err)
{
   if(m_koLog!=NULL)
     m_koLog->Error(err);
}
void VMEBoard::LogMessage(string mess)
{
   if(m_koLog!=NULL)
     m_koLog->Message(mess);
}
void VMEBoard::LogSendMessage(string mess)
{
   if(m_koLog!=NULL)
     m_koLog->SendMessage(mess);
}

int VMEBoard::WriteReg32(u_int32_t address, u_int32_t data)
{
   if(CAENVME_WriteCycle(fCrateHandle,fBID.vme_address+address,
			  &data,cvA32_U_DATA,cvD32)!=cvSuccess)        
     return -1;
   return 0;   
}

int VMEBoard::ReadReg32(u_int32_t address, u_int32_t &data)
{
   u_int32_t temp;
   int ret=0;
   if((ret=CAENVME_ReadCycle(fCrateHandle,fBID.vme_address+address,
			     &temp,cvA32_U_DATA,cvD32))!=cvSuccess)
     return -1;      
   data=temp;
   return 0;
}

int VMEBoard::WriteReg16(u_int32_t address,u_int16_t data)
{
   if(CAENVME_WriteCycle(fCrateHandle,fBID.vme_address+address,
			 &data,cvA32_U_DATA,cvD16)!=cvSuccess)         		    
     return -1;
   return 0;
}

int VMEBoard::ReadReg16(u_int32_t address,u_int16_t &data)
{
   u_int16_t temp;
   if(CAENVME_ReadCycle(fCrateHandle,fBID.vme_address+address,
			&temp,cvA32_U_DATA,cvD16)!=cvSuccess)
     return -1;
   data=temp;
   return 0;
}

int VMEBoard::Initialize(koOptions *options)
{
   bActivated=false;
   return -1; //? or should this return 0
}


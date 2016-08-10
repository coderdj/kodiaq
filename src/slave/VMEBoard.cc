
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
   fBID.id = -1;
   fErrorText = "";
   fError = false;
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
  if(m_koLog!=NULL){
    m_koLog->Error(err);
  }
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
  //stringstream str;
  //str<<"Board: "<<fBID.id<<" writes register "<<hex<<address<<" with "<<data<<dec;
  //LogMessage( str.str() );
  //
  //  if(address == 0x8100){
  //stringstream str;                                                             
  //str<<"Board: "<<fBID.id<<" writes register "<<hex<<address<<" with "<<data<<dec;
  //LogMessage( str.str() ); 
  //    }

  int ret = CAENVME_WriteCycle(fCrateHandle,fBID.vme_address+address,
			       &data,cvA32_U_DATA,cvD32);
  if( ret!=cvSuccess ){
    stringstream err;
    err<<"Failed to write with CAEN ENUM "<<ret;
    LogError( err.str() );
    return -1;
  }
   return 0;   
}

int VMEBoard::ReadReg32(u_int32_t address, u_int32_t &data)
{
  //stringstream str;                                                               
  //str<<"Board: "<<fBID.id<<" reads register "<<hex<<address<<" data "<<data<<dec;
  //LogMessage( str.str() );       

  u_int32_t temp=0;
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
   fError=false;
   fErrorText="";
   return -1; //? or should this return 0
}


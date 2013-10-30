#ifndef _NIMBOARD_HH_
#define _NIMBOARD_HH_

// ******************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : NIMBoard.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.07.2013
// 
// Brief    : Base class for Caen NIM boards
// 
// ******************************************************

#include <sys/types.h>
#include <XeDAQOptions.hh>
#include <XeDAQLogger.hh>
#include <CAENVMElib.h>

using namespace std;
extern XeDAQLogger *gLog;

class NIMBoard
{
 public:
   NIMBoard();
   explicit NIMBoard(BoardDefinition_t BID);
   virtual ~NIMBoard();
   
   void SetID(BoardDefinition_t BID){
      fBID=BID;
   };
   void SetCrateHandle(int CrateHandle){
      fCrateHandle=CrateHandle;
   };
   BoardDefinition_t GetID()  {
      return fBID;
   };
   virtual int SendStartSignal()  {
      return -1;
   };
   virtual int SendStopSignal() {
      return -1;
   };   
   
   
   //Access to registers
   int WriteReg32(u_int32_t address, u_int32_t data);
   int ReadReg32(u_int32_t address, u_int32_t &data);
   
   int WriteReg16(u_int32_t address,u_int16_t data);
   int ReadReg16(u_int32_t address,u_int16_t &data);
   
   //Functions for board access
   virtual int Initialize(XeDAQOptions *options);
//   virtual int GetInfo();    //get info to some type of struct?
   
   //Can set if board is active
   bool Activated()  {
      return bActivated;
   }
   void SetActivated(bool active)  {
      bActivated=active;
   };
   
   
 protected:
   BoardDefinition_t fBID;   
   int fCrateHandle;
   bool bActivated;
};

#endif

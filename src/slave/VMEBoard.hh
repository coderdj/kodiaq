#ifndef _VMEBOARD_HH_
#define _VMEBOARD_HH_

// ******************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : VMEBoard.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.07.2013
// 
// Brief    : Base class for Caen VME boards
// 
// ******************************************************

#include <sys/types.h>
#include <XeDAQOptions.hh>
#include <XeDAQLogger.hh>
#include <CAENVMElib.h>

using namespace std;
extern XeDAQLogger *gLog;

/*! \brief General class for CAEN VME boards.
 
    All shared functionality (like access to the VME register) is defined here. Board-specific functionality should be defined in derived classes.
 */ 
class VMEBoard
{
 public:
   VMEBoard();
   explicit VMEBoard(BoardDefinition_t BID);
   virtual ~VMEBoard();
   
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

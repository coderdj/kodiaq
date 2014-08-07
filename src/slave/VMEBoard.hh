#ifndef _VMEBOARD_HH_
#define _VMEBOARD_HH_

// ******************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : VMEBoard.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.07.2013
// 
// Brief    : Base class for Caen VME boards
// 
// ******************************************************

#include <sys/types.h>
#include <koOptions.hh>
#include <koLogger.hh>
#include <CAENVMElib.h>

using namespace std;

/*! \brief General class for CAEN VME boards.
 
    All shared functionality (like access to the VME register) is defined here. Board-specific functionality should be defined in derived classes.
 */ 
class VMEBoard
{
 public:
   VMEBoard();
   explicit VMEBoard(board_definition_t BID, koLogger *koLog);
   virtual ~VMEBoard();

       
   void SetID(board_definition_t BID){
      fBID=BID;
   };
   void SetCrateHandle(int CrateHandle){
      fCrateHandle=CrateHandle;
   };
   board_definition_t GetID()  {
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
   virtual int Initialize(koOptions *options)=0;
//   virtual int GetInfo();    //get info to some type of struct?
   
   //Can set if board is active
   bool Activated()  {
      return bActivated;
   }
   void SetActivated(bool active)  {
      bActivated=active;
   };
   
   void DesignateSumModule()  {
      bIsSumModule=true;
   };
   bool IsSumModule()  {
      return bIsSumModule;
   };
   
   
 protected:
   board_definition_t fBID;   
   int fCrateHandle;
   bool bActivated;
   bool bIsSumModule;                           //Designating a digitizer a sum module means it always gets saved and the event builder does not use it for peak finding
   void LogError(string err);
   void LogMessage(string mess);
   void LogSendMessage(string mess);
   
 private:
   koLogger *m_koLog;
};

#endif

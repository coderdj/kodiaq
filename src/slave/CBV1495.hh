#ifndef _CBV1495_HH_
#define _CBV1495_HH_

// ****************************************************************
// 
// DAQ Control for Xenon-1t Muon Veto
// General Purpose board in charge of the trigger
//
// File     : CBV1495.hh
// Author   : Cyril Grignon, Mainz University
// Date     : 28.01.2015
// 
// Brief    : Class for Caen V1495 board
// 
// *****************************************************************

#include "VMEBoard.hh"
#include <pthread.h>

//Register definitions
#define CBV1495_ModuleReset               0x800A
#define CBV1495_MaskInA                   0x1020
#define CBV1495_MaskInB                   0x1024
#define CBV1495_MaskInD                   0x1028
#define CBV1495_MajorityThreshold         0x1014
#define CBV1495_CoincidenceWidth          0x1010


/*! \brief Control class for CAEN V1495 general purpose board.
 */ 
class CBV1495 : public VMEBoard {
 public: 
   CBV1495();
   virtual ~CBV1495();
   explicit CBV1495(board_definition_t BoardDef, koLogger *kLog);   /*!   The preferred constructor. If you use the default constructor you have an empty board.*/


   int Initialize(koOptions *options);                             /*!<  Initialize all VME options using a XeDAQOptions object. Other run parameters are also set.*/

 private:
  
   u_int32_t            fMaskA;
   u_int32_t            fMaskB;
   u_int32_t            fMaskD;
   u_int32_t            fMajorityThreshold;
   u_int32_t            fCoincidenceWidth;
  int                   LoadVMEOptions( koOptions *options );

};

#endif




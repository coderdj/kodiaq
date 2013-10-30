#ifndef _VMECRATE_HH_
#define _VMECRATE_HH_

// **********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File    : VMECrate.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 12.7.2013
// 
// Brief   : Class representing one VME crate (Caen software
//           interpretation, not necessarily one physical crate).
//           
// ************************************************************

#include "CBV1724.hh"
#include "CBV2718.hh"
#include <vector>
#include <string>

class VMECrate;

class VMECrate
{
 public:
   VMECrate();
   virtual ~VMECrate();
   
   //Crate Actions
   int     AddModule(BoardDefinition_t BoardID);
   int     InitializeModules(XeDAQOptions *options);   
   int     Define(LinkDefinition_t Link);
   int     Close();
   void    StartRunSW();
   void    StopRunSW();
   void    SetActive();
   void    SetInactive();
   
   //Queries
   bool    IsActive()  {	
      return bCrateActive;
   };   
   LinkDefinition_t Info()  {
      return fLink;
   };
   unsigned int GetDigitizers(){
      return fDigitizers.size();
   };   
   NIMBoard* GetDigitizer(int x)  {
      return fDigitizers[x];
   };
   unsigned int ReadCycle(unsigned int &freq);   
   
   int GetModules()  {
      return fBoards.size();
   };
   NIMBoard *GetModule(int x){
      return fBoards[x];
   };   
   
 private:
   LinkDefinition_t   fLink;
   int                fCrateHandle;
   vector <NIMBoard*> fBoards;        //All boards
   vector <NIMBoard*> fDigitizers;    //Digitizers only
   bool               bCrateActive;
};

#endif

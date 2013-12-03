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

/*! \brief Class defining a single VME crate. 
 
    Note that when boards are read out via their own optical links, each board is defined as its own crate. This is CAEN's design choice, not ours. Therefore this class should not be interpreted as necessarily referring to one physical VME crate. When reading several boards out in daisy chain, each is defined as it's own crate with only a single board in it.
 
    When reading out via the crate controller, only one VME crate is defined which holds all boards that will be read out over the VME bus. Therefore even though the XE1T DAQ will probably be cabled using the optical links on the front of the boards, maintaining the crate/board structure is important for keeping the code general and maintaining compatibility with all forseeable setups.
 */ 
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
   VMEBoard* GetDigitizer(int x)  {
      return fDigitizers[x];
   };
   unsigned int ReadCycle(unsigned int &freq);   
   
   int GetModules()  {
      return fBoards.size();
   };
   VMEBoard *GetModule(int x){
      return fBoards[x];
   };   
   
 private:
   LinkDefinition_t   fLink;
   int                fCrateHandle;
   vector <VMEBoard*> fBoards;        //All boards
   vector <VMEBoard*> fDigitizers;    //Digitizers only
   bool               bCrateActive;
};

#endif

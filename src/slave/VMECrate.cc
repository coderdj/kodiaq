
// ***************************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : VMECrate.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 12.07.2013
// 
// Brief    : Class representing one VME crate (Caen software
//            interpretation, not necessarily one physical crate).
//            
// ****************************************************************
#include <iostream>
#include "VMECrate.hh"

VMECrate::VMECrate()
{
   fCrateHandle=-1;
   bCrateActive=false;
   m_koLog=NULL;
}

VMECrate::~VMECrate()
{
   Close();
}

VMECrate::VMECrate(koLogger *koLog)
{
   fCrateHandle = -1;
   bCrateActive = false;
   m_koLog      = koLog;
}

int VMECrate::Define(LinkDefinition_t Link)
{
   fLink=Link;  
   CVBoardTypes BType;
   if(Link.LinkType=="V1718")
     BType = cvV1718;
   else if(Link.LinkType=="V2718")
     BType = cvV2718;
   else  {
      if(m_koLog!=NULL)
	m_koLog->Error("VMECrate::Define - Invalid link type, check file definition.");
      return -1;
   }
   int temp;
   int cerr=-20;
   if((cerr=CAENVME_Init(BType,Link.LinkID,Link.CrateID,&temp))!=cvSuccess)  {
      stringstream message;
      message<<"VMECrate::Define - Failed to initialize link "<<Link.LinkID<<". CAEN error "<<cerr;
      if(m_koLog!=NULL)
	m_koLog->Error(message.str());
      return -1;
   }
//   CAENVME_DeviceReset(temp);
   fCrateHandle=temp;
   
   stringstream ss;
   ss<<"Defined crate "<<Link.CrateID<<" over link "<<Link.LinkID<<
     " with handle "<<fCrateHandle;
   if(m_koLog!=NULL)
     m_koLog->Message(ss.str());  
   bCrateActive=false;
   return 0;
}

int VMECrate::AddModule(BoardDefinition_t BoardID)
{
   //Only defined types will be added to the code
   stringstream ss;
   if(BoardID.BoardType=="V1724")  {
      CBV1724 *module = new CBV1724(BoardID,m_koLog);
      module->SetCrateHandle(fCrateHandle);
      fBoards.push_back(module);
      fDigitizers.push_back(module);
      ss<<"VMECrate::AddModule - Module type V1724 with VME Address "
	<<hex<<(BoardID.VMEAddress &0xFFFFFFFF)<<" added.";
      if(m_koLog!=NULL)
	m_koLog->Message(ss.str());
      return 0;
   }
   else if(BoardID.BoardType=="V2718")  {
      CBV2718 *module = new CBV2718(BoardID,m_koLog);
      module->SetCrateHandle(fCrateHandle);
      fBoards.push_back(module);
      ss<<"VMECrate::AddModule - Module type V2718 with VME Address "
	<<hex<<(BoardID.VMEAddress &0xFFFFFFFF)<<" added.";
      if(m_koLog!=NULL)
	m_koLog->Message(ss.str());
      return 0;
   }   
   else     {
      ss<<"VMECrate::AddModule - Module type "<<BoardID.BoardType<<" unknown.";
      if(m_koLog!=NULL) {	   
	 m_koLog->Error(ss.str());
	 m_koLog->Message(ss.str());
      }      
   }      
   return 0;
}

int VMECrate::InitializeModules(koOptions *options)
{
   stringstream ss;
   for(unsigned int x=0; x<fBoards.size();x++)    {
      if(fBoards[x]->Initialize(options)!=0)	{
	 ss<<"VMECrate::InitializeModules - Could not initialize module "<<fBoards[x]->GetID().BoardID;
	 if(m_koLog!=NULL)
	   m_koLog->Error(ss.str());
	 return -1;
      }      
   }   
   return 0;
}


int VMECrate::Close()
{  
   for(unsigned int x=0;x<fBoards.size();x++)
     delete fBoards[x];
   fBoards.clear();
   fDigitizers.clear();
   
   stringstream ss;
   if(CAENVME_End(fCrateHandle)!=cvSuccess)  {
      ss<<"VMECrate::Close - Failed to close crate "<<fCrateHandle;
      if(m_koLog!=NULL)
	m_koLog->Error(ss.str());
      return -1;
   }
   ss<<"VMECrate::Close - Closed crate "<<fCrateHandle;
   if(m_koLog!=NULL)
     m_koLog->Message(ss.str());   
   return 0;
}

unsigned int VMECrate::ReadCycle(u_int32_t &freq)
{      
   unsigned int rate=0;      
   freq=0;
   for(unsigned int x=0;x<fDigitizers.size();x++)  {
      CBV1724 *digi=dynamic_cast<CBV1724*>(fDigitizers[x]);               	          
      if(digi==NULL) continue;
      u_int32_t size=digi->ReadMBLT();
      if(size!=0)  freq=1;
      rate+=size;
   }
   return rate;
}

void VMECrate::StartRunSW()
{
   bCrateActive=true;
   for(unsigned int x=0;x<fDigitizers.size();x++){
      u_int32_t data;     
      fDigitizers[x]->ReadReg32(V1724_AcquisitionControlReg,data);
      data |= 0x4;
      fDigitizers[x]->WriteReg32(V1724_AcquisitionControlReg,data);
      fDigitizers[x]->SetActivated(true);
   }      
}

void VMECrate::StopRunSW()
{
   bCrateActive=false;
   for(unsigned int x=0;x<fDigitizers.size();x++)  {
      u_int32_t data;
      fDigitizers[x]->ReadReg32(V1724_AcquisitionControlReg,data);      
      data &= 0xFFFFFFFB;
      fDigitizers[x]->WriteReg32(V1724_AcquisitionControlReg,data);
      fDigitizers[x]->SetActivated(false);   
   }   
}

void VMECrate::SetActive()
{
   bCrateActive=true;
   for(unsigned int x=0;x<fDigitizers.size();x++)
     fDigitizers[x]->SetActivated(true);
}

void VMECrate::SetInactive()
{
   bCrateActive=false;
   for(unsigned int x=0;x<fDigitizers.size();x++)
     fDigitizers[x]->SetActivated(false);
}

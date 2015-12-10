
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

#include <iostream>
#include <iomanip>
#include <cstring>
#include <time.h>
#include "CBV1495.hh"
#include "DataProcessor.hh"

CBV1495::CBV1495()
{   
  
   fMaskA = 0x0;
   fMaskB = 0x0;
   fMaskD = 0x0;
   fMajorityThreshold = 0x00000001;
   fCoincidenceWidth = 0x0000000F;

}


CBV1495::CBV1495(board_definition_t BoardDef, koLogger *kLog)
        :VMEBoard(BoardDef,kLog)
{
  
  fMaskA = 0xFFFFFFFF;  // 0-31
  fMaskB = 0xFFFFFFFF;  // 0-31
  fMaskD = 0x1FFFFF;  // 0-21
  fMajorityThreshold = 0x00000001;
  fCoincidenceWidth = 0x0000000F;
  
}

CBV1495::~CBV1495()
{

}

int CBV1495::Initialize(koOptions *options)
{
   int retVal=0;
 
 
   u_int32_t TRGadr;
   u_int32_t TRGdataIn, TRGdataOut;

   TRGdataIn = 0x00000001;

   /*module reset*/
   if(WriteReg32(CBV1495_ModuleReset,TRGdataIn) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_ModuleReset);
     retVal = -1;
   }
   
   /*set mask for A*/
   if(WriteReg32(CBV1495_MaskInA,fMaskA) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_MaskInA);
     retVal = -1;
   }
 
   /*set mask for B*/
   if(WriteReg32(CBV1495_MaskInB,fMaskB) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_MaskInB);
     retVal = -1;
   }

   /*set mask for D*/
   if(WriteReg32(CBV1495_MaskInD,fMaskD) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_MaskInD);
     retVal = -1;
   }

 
   /*set majority threshold */
   if(WriteReg32(CBV1495_MajorityThreshold,fMajorityThreshold) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_MajorityThreshold);
     retVal = -1;
   }

   /*set majority coincidencew width (multiple of 25 ns) */
   if(WriteReg32(CBV1495_CoincidenceWidth,fCoincidenceWidth) !=  cvSuccess ){
     printf(":::: WriteCycle ERROR V1495 ::::\n");
     printf("     Fail to reach address %08X \n",fBID.vme_address+CBV1495_CoincidenceWidth);
     retVal = -1;
   }  

  // Reload VME options
  int tries = 0;
  while( LoadVMEOptions( options ) != 0 && tries < 5) {
    tries ++;
    usleep(100);
    if( tries == 5 )
      retVal = -1;
   }
 
   // Report whether we succeeded or not
   stringstream messstr;
   if( retVal == 0 ){
     messstr<<"Board "<<fBID.id<<" initialized successfully";
     m_koLog->Message( messstr.str() );
   }
   else{
     messstr<<"Board "<<fBID.id<<" failed initialization";
     m_koLog->Message( messstr.str() );
   }

   return retVal;

}
 int CBV1495::LoadVMEOptions( koOptions *options )
 {
   int retVal = 0;
   // Reload VME options    
   for(int x=0;x<options->GetVMEOptions();x++)  {
     if((options->GetVMEOption(x).board==-1 ||
         options->GetVMEOption(x).board==fBID.id)){
       int success = WriteReg32(options->GetVMEOption(x).address,
                                options->GetVMEOption(x).value);
       if( options->GetVMEOption(x).address == 0xEF24 ) // give time to reset
         usleep(1000);
       if( success != 0 ){
         retVal=success;
         stringstream errorstr;
         errorstr<<"Couldn't write VME option "<<hex
                 <<options->GetVMEOption(x).address<<" with value "
                 <<options->GetVMEOption(x).value<<" to board "<<fBID.id;
         LogError( errorstr.str() );
       }
     }

   }
   return retVal;
 }
          



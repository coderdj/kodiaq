#ifndef _XEDAQOPTIONS_HH_
#define _XEDAQOPTIONS_HH_

// *************************************************************
// *************************************************************
// 
// File     :  XeDAQOptions.hh
// Author   :  Daniel Coderre, LHEP, Universitaet Bern
// Date     :  27.6.2013
// 
// Brief    :  Options handler for Xenon-1t DAQ software
// 
// *************************************************************
// *************************************************************

#include <vector>
#include <string>
#include <fstream>
#include <math.h>

#include "XeDAQHelper.hh"

using namespace std;

/*! \brief Stores configuration information for an optical link.
 */ 
struct LinkDefinition_t{
   string LinkType;
   int LinkID;
   int CrateID;
};

/*! \brief Stores configuration information for one VME module.
 */
struct BoardDefinition_t{
   string BoardType;
   int VMEAddress;
   int BoardID;
   int CrateID;
   int LinkID;
};

/*! \brief Generic storage container for a VME option.
 */
struct VMEOption_t{
   u_int32_t Address;
   u_int32_t Value;
   int BoardID;
   int CrateID;
   int LinkID;
};

/*! \brief Various options related to the run.
 */
struct RunOptions_t {
   u_int32_t BLTBytes; //size of block transfer
   int WriteMode; //0-don't write 1-to file 2-to network
   int RunStart; //0-board internal 1-sin controlled
   int RunStartModule; //must be set if RunStart==1
   unsigned int BLTPerBulkInsert;
};

/*! \brief Configuration options for mongodb.
 */
struct MongodbOptions_t {
   bool ZipOutput;
   bool DynamicRunNames;
   int  MinInsertSize;
   bool WriteConcern;
   int  BlockSplitting;
   string DBAddress;
   string Collection;
};

/*! \brief Configuration options for ddc-10 FPGA module.
 */
struct DDC10Options_t{
   int Sign;
   int IntegrationWindow;
   int VetoDelay;
   int SignalThreshold;
   int IntegrationThreshold;
   int WidthCut;
   int RunMode;
   string Address;
};

/*! \brief Reads and processes an options file.
 
    This class automatically reads, parses, and stores the information in an options file. Various functions allow access to this information. This class can be used throughout the program but it most imprortant for the slave PCs.
 */
class XeDAQOptions
{
 public:
   XeDAQOptions();
   virtual ~XeDAQOptions();
   
   int ReadParameterFile(string filename);                         /*!< Read options for slave modules. */
   int ReadFileMaster(string filename);                            /*!< Read options for master module. */
   static int ProcessLine(string line,string option,int &ret);   
   static int ProcessLineHex(string line,string option, int &ret);
   
   int GetLinks()  {
      return fLinks.size();
   };
   LinkDefinition_t GetLink(int x)  {
      return fLinks[x];
   };
   
   int GetBoards()  {
      return fBoards.size();      
   };
   BoardDefinition_t GetBoard(int x)  {
      return fBoards[x];
   };
   
   int GetVMEOptions()  {
      return fVMEOptions.size();
   };
   VMEOption_t GetVMEOption(int x)  {
      return fVMEOptions[x];
   };            
   
   RunOptions_t GetRunOptions()  {
      return fRunOptions;
   };
   MongodbOptions_t GetMongoOptions(){
      return fMongoOptions;
   };   
   
   int GetWriteMode()  {
      return fWriteMode;
   };
   
   int GetProcessingThreads()  {
      return fProcessingThreads;
   };
   int GetBaselineMode(){
      return fBaselineMode;
   };   
   int GetReadoutThreshold()  {
      return fReadoutThreshold;
   };
   DDC10Options_t GetVetoOptions()  {
      return fDDC10Options;
   };
   
   int SumModules()  {
      return vSumModules.size();
   };
   int GetSumModule(int x)  {
      return vSumModules[x];
   };
   
   void UpdateMongodbCollection(string collection)  {
      fMongoOptions.Collection=collection;
   };
   
   
   
 private:
   vector<LinkDefinition_t>  fLinks;   
   vector<BoardDefinition_t> fBoards;
   vector<VMEOption_t>       fVMEOptions;
   vector<int>               vSumModules;
   RunOptions_t              fRunOptions;
   MongodbOptions_t          fMongoOptions;   
   DDC10Options_t            fDDC10Options;
   string                    fRunModeID;
   int                       fWriteMode;
   int                       fProcessingThreads;
   int                       fBaselineMode;
   unsigned int              fReadoutThreshold;
   void                      Reset();
};
#endif

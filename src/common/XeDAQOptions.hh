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

struct LinkDefinition_t{
   string LinkType;
   int LinkID;
   int CrateID;
};

struct BoardDefinition_t{
   string BoardType;
   int VMEAddress;
   int BoardID;
   int CrateID;
   int LinkID;
};

struct VMEOption_t{
   u_int32_t Address;
   u_int32_t Value;
   int BoardID;
   int CrateID;
   int LinkID;
};

struct FileDefinition_t{
   string WritePath;
   int    WriteFormat;   //0-binary 1-ascii
   int    SplitMode;   //0-write to one file 1-split files
   unsigned int    ChunkLength; //in seconds
   int    Overlap;     //in milliseconds
};

struct RunOptions_t {
   u_int32_t BLTBytes; //size of block transfer
   int WriteMode; //0-don't write 1-to file 2-to network
   int RunStart; //0-board internal 1-sin controlled
   int RunStartModule; //must be set if RunStart==1
   unsigned int BLTPerBulkInsert;
};

struct MongodbOptions_t {
   bool ZipOutput;
   int  MinInsertSize;
   bool WriteConcern;
   int  BlockSplitting;
};

class XeDAQOptions
{
 public:
   XeDAQOptions();
   virtual ~XeDAQOptions();
   
   int ReadParameterFile(string filename);
   static int ProcessLine(string line,string option,int &ret);   
   static int ProcessLineHex(string line,string option, int &ret);
   
   FileDefinition_t GetFileDef()  {
      return fFileDef;
   };
   
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
   string GetDBName()  {
      return fMongoDBName;
   };
   
   
 private:
   vector<LinkDefinition_t>  fLinks;   
   vector<BoardDefinition_t> fBoards;
   vector<VMEOption_t>       fVMEOptions;
   FileDefinition_t          fFileDef;
   RunOptions_t              fRunOptions;
   MongodbOptions_t          fMongoOptions;   
   int                       fWriteMode;
   int                       fProcessingThreads;
   int                       fBaselineMode;
   unsigned int              fReadoutThreshold;
   void                      Reset();
   string                    fMongoDBName;
};
#endif

#ifndef _KOOPTIONS_HH_
#define _KOOPTIONS_HH_

// *************************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     :  koOptions.hh
// Author   :  Daniel Coderre, LHEP, Universitaet Bern
// Date     :  27.06.2013
// Update   :  27.03.2014
// 
// Brief    :  Options handler for Xenon-1t DAQ software
// 
// *************************************************************

#include <vector>
#include <string>
#include <fstream>
#include <math.h>
#include "config.h"
#include "koHelper.hh"

#ifdef WITH_DDC10
#include <ddc_10.hh>
#endif

using namespace std;

#define WRITEMODE_NONE    0
#define WRITEMODE_FILE    1
#define WRITEMODE_MONGODB 2

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
   int BaselineMode;
};

/*! \brief Options for intermediate data processing
 */
struct ProcessingOptions_t {
   int NumThreads;
   int Mode;
   int ReadoutThreshold;
};

/*! \brief Options for file output
 */
struct OutfileOptions_t{
   string Path;
   bool DynamicRunNames;
   int EventsPerFile;
};


/*! \brief Configuration options for mongodb.
 */
struct MongodbOptions_t {
   bool DynamicRunNames;
   int  MinInsertSize;
   bool WriteConcern;
   string DBAddress;
   string Collection;
};


/*! \brief Reads and processes an options file.
 
    This class automatically reads, parses, and stores the information in an options file. Various functions allow access to this information. This class can be used throughout the program but it most imprortant for the slave PCs.
 */
class koOptions
{
 public:
   koOptions();
   virtual ~koOptions();
   
   int ReadParameterFile(string filename);                         /*!< Read options for slave modules. */
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
   ProcessingOptions_t GetProcessingOptions()  {
      return fProcessingOptions;
   };
   OutfileOptions_t GetOutfileOptions()  {
      return fOutfileOptions;
   };      
   int SumModules()  {
      return vSumModules.size();
   };
   int GetSumModule(int x)  {
      return vSumModules[x];
   };
   int Compression(){
     return bCompression;
   };
   void UpdateMongodbCollection(string collection)  {
      fMongoOptions.Collection=collection;
   };
   
   //Put any dependency-specific public members here
#ifdef WITH_DDC10
   ddc10_par_t GetVetoOptions()  {	
      return fDDC10Options;
   };
#endif
   
   
 private:
   vector<LinkDefinition_t>  fLinks;   
   vector<BoardDefinition_t> fBoards;
   vector<VMEOption_t>       fVMEOptions;
   vector<int>               vSumModules;
   
   RunOptions_t              fRunOptions;
   MongodbOptions_t          fMongoOptions;   
   ProcessingOptions_t       fProcessingOptions;
   OutfileOptions_t          fOutfileOptions;
     
   string                    fRunModeID;
   void                      Reset();   
   bool                      bCompression;
// put any dependency-specific private members here   
#ifdef WITH_DDC10
   ddc10_par_t               fDDC10Options;
#endif
   
};
#endif

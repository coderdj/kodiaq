#ifndef _KOOPTIONS_HH_
#define _KOOPTIONS_HH_

// *************************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     :  koOptions.hh
// Author   :  Daniel Coderre, LHEP, Universitaet Bern
// Date     :  27.06.2013
// Update   :  27.03.2014, 6.08.2014
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

using namespace std;

#define WRITEMODE_NONE    0
#define WRITEMODE_FILE    1
#define WRITEMODE_MONGODB 2

/*! \brief Stores configuration information for an optical link.
 */ 
struct link_definition_t{
   string type;
   int id;
   int crate;
   char node;
};

/*! \brief Stores configuration information for one VME module.
 */
struct board_definition_t{
  string type;
  int vme_address;
  int id;
  int crate;
  int link;
  char node;
};

/*! \brief Generic storage container for a VME option.
 */
struct vme_option_t{
  u_int32_t address;
  u_int32_t value;
  int board;
  char node;
};

/*! \brief Reads and processes an options file.
 
    This class automatically reads, parses, and stores the information in an options file. Various functions allow access to this information. This class can be used throughout the program but it most imprortant for the slave PCs.
 */
class koOptions
{
public:
  koOptions();
  virtual ~koOptions();
  
  int ReadParameterFile(string filename);    
  string SaveToString(){return "";};
  int WriteToFile(string filename){return 0;};
  
  int GetLinks()  {
    return m_links.size();
  };
  link_definition_t GetLink(int x)  {
    return m_links[x];
  };
  void AddLink(link_definition_t link){
    m_links.push_back(link);
  };

  int GetBoards()  {
    return m_boards.size();      
  };
  board_definition_t GetBoard(int x)  {
    return m_boards[x];
  };
  void AddBoard(board_definition_t board){
    m_boards.push_back(board);
  };
  
  int GetVMEOptions()  {
    return m_registers.size();
  };
  vme_option_t GetVMEOption(int x)  {
    return m_registers[x];
  };
  void AddVMEOption(vme_option_t vme){
    m_registers.push_back(vme);
  };
  
  stringstream* GetDDCStream(){
    return &m_ddc10_options_stream;
  };
  void SetDDCStream(string ddcoptions){
    m_ddc10_options_stream.clear();
    m_ddc10_options_stream.seekg(0,std::ios::beg);
    m_ddc10_options_stream<<ddcoptions;
  };
  void ToStream(stringstream *retstream);
  //Put any dependency-specific public members here
   
  
private:
  vector<link_definition_t>  m_links;   
  vector<board_definition_t> m_boards;
  vector<vme_option_t>       m_registers;
  
  stringstream               m_ddc10_options_stream;

  int ProcessLine(string line,string option,int &ret);
 

public:
  // General info
  string                     name;
  string                     nickname;
  string                     creator;
  string                     creation_date;
  
  // Run options
  int                        write_mode;
  int                        baseline_mode;
  int                        noise_spectra;
  int                        run_start;
  int                        run_start_module;
  int                        pulser_freq;
  int                        blt_size;
  int                        compression;
  bool                       dynamic_run_names;
  string                     run_prefix;

  // Noise
  int                        noise_spectra_enable;
  int                        noise_spectra_length;
  string                     noise_spectra_mongo_addr;
  string                     noise_spectra_mongo_coll;

  // Processing options
  int                        processing_mode;
  int                        processing_num_threads;
  int                        processing_readout_threshold;
  bool                       occurrence_integral;

  // Mongodb options
  string                     mongo_address;
  string                     mongo_database;
  string                     mongo_collection;
  int                        mongo_write_concern;
  int                        mongo_min_insert_size;
  string                     mongo_output_format;

  // File output options
  string                     file_path;
  int                        file_events_per_file;

  // Extra options
  int                        muon_veto;
  int                        led_trigger;
  string                     trigger_mode;
  string                     data_processor_mode;
  int                        buffer_size_kill;
  
  void                       Reset();   
   
};
#endif

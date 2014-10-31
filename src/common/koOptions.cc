// *************************************************************
//
// kodiaq Data Acquisition Software
//  
// File   : koOptions.cc
// Author : Daniel Coderre, LHEP, Universitaet Bern
// Date   : 27.06.2013
// Update : 27.03.2014
// 
// Brief  : Options handler for Xenon-1t DAQ software
// 
// *************************************************************

#include <iostream>

#include "koOptions.hh"

koOptions::koOptions()
{
   Reset();   
}

koOptions::~koOptions()
{
}

void koOptions::Reset()
{
  m_links.clear();
  m_boards.clear();
  m_registers.clear();
  
  //Reset general
  name=creator=creation_date="";
  
  //Reset run options
  write_mode = baseline_mode = run_start = run_start_module = 
    blt_size = compression = -1;
  dynamic_run_names = false;
  
  //Reset mongodb options
  mongo_address = mongo_database = mongo_collection = "";
  mongo_write_concern = mongo_min_insert_size = -1;
  
  //Reset processing options
  processing_mode = processing_num_threads = processing_readout_threshold = -1;
  
  //Reset file options
  file_path = "";
  file_events_per_file = -1;

  //Extra options
  muon_veto = false;
  led_trigger = false;

#ifdef HAS_DDC
  //Reset ddc10 options
  fDDC10Options.Initialized=false;
#endif
}

int koOptions::ProcessLine(string line, string option,int &ret)
{
   istringstream iss(line);
   vector<string> words;
   copy(istream_iterator<string>(iss),
	istream_iterator<string>(),
	back_inserter<vector<string> >(words));
   if(words.size()<2 || words[0]!=option) return -1;
   ret=koHelper::StringToInt(words[1]);
   
   return 0;
}

int koOptions::ReadParameterFile(string filename)
{
   Reset();
   ifstream initFile;
   initFile.open(filename.c_str());
   if(!initFile){
      cout<<"init file not found."<<endl;
      return -1;
   }   
   string line;
   while(!initFile.eof())  {
      getline(initFile,line);
      if(line[0]=='#') continue; //ignore comments
      char node='x';
      if(line[0]=='%') {
	node=line[1];
	line.erase(0,2);
      }
      //parse
      istringstream iss(line);
      vector<string> words;
      copy(istream_iterator<string>(iss),
	   istream_iterator<string>(),
	   back_inserter<vector<string> >(words));
      if(words.size()<2) continue;

      // Ugly if/else but what else to do in C++?

      if(words[0] == "name")
	  name = words[1];
      else if(words[0] == "creator")
	creator = words[1];
      else if(words[0] == "creation_date")
	creation_date = words[1];
      else if(words[0] == "write_mode")
	write_mode = koHelper::StringToInt(words[1]);
      else if(words[0] == "baseline_mode")
	baseline_mode = koHelper::StringToInt(words[1]);
      else if(words[0] == "run_start")
	run_start = koHelper::StringToInt(words[1]);
      else if(words[0] == "run_start_module")
	run_start_module = koHelper::StringToInt(words[1]);
      else if(words[0] == "blt_size")
	blt_size = koHelper::StringToInt(words[1]);
      else if(words[0] == "compression")
	compression = koHelper::StringToInt(words[1]);
      else if(words[0] == "processing_mode")
	processing_mode = koHelper::StringToInt(words[1]);
      else if(words[0] == "processing_num_threads")
	processing_num_threads = koHelper::StringToInt(words[1]);
      else if(words[0] == "processing_readout_threshold")
	processing_readout_threshold = koHelper::StringToInt(words[1]);
      else if(words[0] == "mongo_address")
	mongo_address = words[1];
      else if(words[0] == "mongo_collection")
	mongo_collection = words[1];
      else if(words[0] == "mongo_database")
	mongo_database = words[1];
      else if(words[0] == "mongo_write_concern")
	mongo_write_concern = koHelper::StringToInt(words[1]);
      else if(words[0] == "mongo_min_insert_size")
	mongo_min_insert_size = koHelper::StringToInt(words[1]);
      else if(words[0] == "file_path")
	file_path = words[1];
      else if(words[0] == "file_events_per_file")
	file_events_per_file = koHelper::StringToInt(words[1]);
      else if(words[0] == "led_trigger")
	led_trigger = koHelper::StringToInt(words[1]);
      else if(words[0] == "muon_veto")
	muon_veto = koHelper::StringToInt(words[1]);
      else if(words[0] == "register") {
	vme_option_t reg;
	if(words.size()<3) break;
	reg.address = koHelper::StringToHex(words[1]);
	reg.value   = koHelper::StringToHex(words[2]);
	if(words.size()>=4 && words[3][0]!='#')
	  reg.board = koHelper::StringToInt(words[3]);
	else reg.board=-1;
	reg.node=node;
	m_registers.push_back(reg);
      }
      else if(words[0] == "link"){
	link_definition_t link;
	if(words.size()<4) break;
	link.type = words[1];
	link.id   = koHelper::StringToInt(words[2]);
	link.crate= koHelper::StringToInt(words[3]);
	link.node=node;
	m_links.push_back(link);
      }
      else if(words[0] == "board"){
	board_definition_t board;
	if(words.size()<6) break;
	board.type = words[1];
	board.vme_address = koHelper::StringToHex(words[2]);
	board.id = koHelper::StringToInt(words[3]);
	board.link = koHelper::StringToInt(words[4]);
	board.crate = koHelper::StringToInt(words[5]);	
	board.node=node;
	m_boards.push_back(board);
      }
      else if(words[0].substr(0,5) == "ddc10"){
	for(unsigned int x=0;x<words.size();x++)
	  m_ddc10_options_stream<<words[x]<<" ";
	m_ddc10_options_stream<<endl;
      }
   }   
   initFile.close();
   return 0;   
} 

void koOptions::ToStream(stringstream *retstream)
{
  (*retstream)<<"name "<<name<<endl;
  (*retstream)<<"creator "<<creator<<endl;
  (*retstream)<<"creation_date "<<creation_date<<endl;
  (*retstream)<<"write_mode "<<write_mode<<endl;
  (*retstream)<<"baseline_mode "<<baseline_mode<<endl;
  (*retstream)<<"run_start "<<run_start<<endl;
  (*retstream)<<"run_start_module "<<run_start_module<<endl;
  (*retstream)<<"blt_size "<<blt_size<<endl;
  (*retstream)<<"compression "<<compression<<endl;
  (*retstream)<<"processing_mode "<<processing_mode<<endl;
  (*retstream)<<"processing_num_threads "<<processing_num_threads<<endl;
  (*retstream)<<"processing_readout_threshold "<<processing_readout_threshold<<endl;
  (*retstream)<<"mongo_address "<<mongo_address<<endl;
  (*retstream)<<"mongo_collection "<<mongo_collection<<endl;
  (*retstream)<<"mongo_database "<<mongo_database<<endl;
  (*retstream)<<"mongo_write_concern "<<mongo_write_concern<<endl;
  (*retstream)<<"mongo_min_insert_size "<<mongo_min_insert_size<<endl;
  (*retstream)<<"file_path "<<file_path<<endl;
  (*retstream)<<"file_events_per_file "<<file_events_per_file<<endl;
  (*retstream)<<"led_trigger "<<led_trigger<<endl;
  (*retstream)<<"muon_veto "<<muon_veto<<endl;
  for(unsigned int x=0;x<m_registers.size();x++){
    if(m_registers[x].node!='x')
      (*retstream)<<"%"<<m_registers[x].node;
    (*retstream)<<"register "<<hex<<m_registers[x].address<<" "
		<<m_registers[x].value<<dec<<" "<<m_registers[x].board<<endl;
  }
  for(unsigned int x=0;x<m_links.size();x++){
    if(m_links[x].node!='x')
      (*retstream)<<"%"<<m_links[x].node;  
    (*retstream)<<"link "<<m_links[x].type<<" "<<m_links[x].id<<" "<<
      m_links[x].crate<<endl;
  }
  for(unsigned int x=0;x<m_boards.size();x++){
    if(m_boards[x].node!='x')
      (*retstream)<<"%"<<m_boards[x].node;
    (*retstream)<<"board "<<m_boards[x].type<<" "<<hex<<m_boards[x].vme_address<<
      dec<<" "<<m_boards[x].id<<" "<<m_boards[x].link<<" "<<
      m_boards[x].crate<<" "<<endl;
  }
  (*retstream)<<m_ddc10_options_stream.str()<<endl;
}

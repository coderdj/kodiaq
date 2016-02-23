#include "koOptions.hh"
#include "koHelper.hh"
#include "mongo/db/json.h"
#include <fstream>
#include <iostream>

koOptions::koOptions(){ fLoaded=false; }

koOptions::~koOptions(){}

int koOptions::ReadParameterFile(string filename){

  // open file
  std::ifstream infile;
  infile.open(filename.c_str());
  if(!infile)
    return -1;
  
  // Pull data and put into string
  string json_string = "", str;
  while(getline(infile,str))
    json_string += str;
  infile.close();

  // Make the bson object from the string
  try{
    m_bson = bsoncxx::from_json(json_string);
    cout<<bsoncxx::to_json(m_bson)<<endl;
  }
  catch(...){
    std::cout<<"Error parsing file. Is it valid json?"<<std::endl;
    return -1;
  }
  fLoaded=true;
  cout<<"Read parameter file from "<<filename<<endl;
  return 0;
}

int koOptions::GetArraySize(string key){
  try{
    return m_bson[key].get_array().size();
  }
  catch(...){
    return 0;
  }
}

mongo::BSONElement koOptions::GetField(string key){
  mongo::BSONElement b;
  try{
    b = m_bson[key];
  }
  catch(...){
    std::cout<<"Error fetching option. Key doesn't seem to exist: "<<
      key<<std::endl;
    return bsoncxx::document::element;
  }
  return b;
}

void koOptions::ToStream_MongoUpdate(string run_name, 
				     stringstream *retstream){

  // For dynamic collection names
  mongocxx::builder::stream::document builder{};
  using bsoncxx::builder::core::concatenate;
  builder << concatenate << m_bson;
  builder<<"mongo_collection"<<run_name;
  //  builder.appendElementsUnique(m_bson);
  (*retstream)<<bsoncxx::to_json(builder);
}


void koOptions::SetString(string field_name, string value){
  
  // This is not very beautiful, but you can't modify a BSONObj
  // So we will create a new BSON object that is a copy of the 
  // Current one and update the field
  mongocxx::builder::stream::document builder{};
  using bsoncxx::builder::core::concatenate;
  builder << concatenate << m_bson;
  builder<<field_name<<value;
  m_bson = builder.view();

}
void koOptions::SetInt(string field_name, int value){
  mongocxx::builder::stream::document builder{};
  using bsoncxx::builder::core::concatenate;
  builder << concatenate << m_bson;
  builder<<field_name<<value;
  m_bson = builder.view();
}

link_definition_t koOptions::GetLink(int index){

  // Return dummy link if dne
  link_definition_t dummy;
  dummy.type="NULL";
  dummy.id=-1000;
  dummy.crate=-1000;
  dummy.node=-1;
  if(index < 0 || index > GetArraySize("links") || GetArraySize("links")==0){
      return dummy;
    }

  // Fill object
  link_definition_t retlink;
  try{
    auto link_obj = m_bson["links"].get_array()[index].get_document();
    if(link_obj) return dummy;

    if(! (link_obj["type"] && link_obj["crate"] && link_obj["link"] &&
	  link_obj["reader"] ) )
      return dummy;

    retlink.type = link_obj["type"].get_utf8();
    retlink.crate = link_obj["crate"].get_int32();
    retlink.id = link_obj["link"].get_int32();
    retlink.node = link_obj["reader"].get_int32();
  }
  catch(...){
    return dummy;
  }
  return retlink;
}

board_definition_t koOptions::GetBoard(int index){

  // Return dummy board if dne 
  board_definition_t dummy;
  dummy.type="NULL";
  dummy.vme_address=-1000;
  dummy.id=-1000;
  dummy.crate=-1000;
  dummy.link=-1000;
  dummy.node=-1;  
  if(index < 0 || index > GetArraySize("boards") || GetArraySize("boards")==0){
    return dummy;
  }

  // Fill object 
  board_definition_t retboard;
  try{
    auto board_obj = m_bson["boards"].get_array()[index].get_document();
    if(board_obj) return dummy;

    if(! (board_obj["type"] && board_obj["crate"] && board_obj["link"] &&
          board_obj["reader"] && board_obj["serial"] && board_obj["vme_address"] ))
       return dummy;
              
       retboard.type = board_obj["type"].get_utf8();
       retboard.vme_address = koHelper::StringToHex(board_obj["vme_address"].get_utf8());
       retboard.id = koHelper::StringToInt(board_obj["serial"].get_utf8());
       retboard.crate = board_obj["crate"].get_int32();
       retboard.link = board_obj["link"].get_int32();
       retboard.node = board_obj["reader"].get_int32();
       }
    catch(...){
      return dummy;
    }
    return retboard;
  }
}

vme_option_t koOptions::GetVMEOption(int index){

  // Return dummy link if dne     
  vme_option_t dummy;
  dummy.address=0;
  dummy.value=0;
  dummy.board=-1000;
  dummy.node=-1;
  if(index < 0 || index > GetArraySize("registers") 
     || GetArraySize("registers")==0){
    return dummy;
  }

  // Fill object                                                                    
  vme_option_t retreg;
  try{
    auto vme_obj = m_bson["registers"].get_array()[index].get_document();
    if(vme_obj) return dummy;

    if(! (vme_obj["register"] && vme_obj["value"] && vme_obj["board"] ))
      return dummy;
    
    retreg.address = vme_obj["register"].get_utf8();
    retreg.value = koHelper::StringToHex(vme_obj["value"].get_utf8());
    retreg.board = koHelper::StringToInt(vme_obj["board"].get_int32());
    retreg.node = -1;
  }
  catch(...){
    return dummy;
  }
  return retreg;
}


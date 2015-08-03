#include "koOptions.hh"
#include "koHelper.hh"
#include "mongo/db/json.h"
#include <fstream>
#include <iostream>

koOptions::koOptions(){}

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
    m_bson = mongo::fromjson(json_string);
  }
  catch(...){
    std::cout<<"Error parsing file. Is it valid json?"<<std::endl;
    return -1;
  }
  return 0;
}

int koOptions::GetArraySize(string key){
  try{
    return m_bson[key].Array().size();
  }
  catch(...){
    return 0;
  }
}

mongo::BSONElement koOptions::GetField(string key){
  try{
    return m_bson[key];
  }
  catch(...){
    std::cout<<"Error fetching option. Key doesn't seem to exist."<<std::endl;
    return mongo::BSONElement();
  }
}

void koOptions::ToStream_MongoUpdate(string run_name, 
				     stringstream *retstream){

  // For dynamic collection names
  mongo::BSONObjBuilder builder;
  builder.append("mongo_collection", run_name );
		 //		 koHelper::MakeDBName(run_name, 
		 //		      m_bson["mongo_collection"].String()));
  builder.appendElementsUnique(m_bson);
  (*retstream)<<builder.obj().jsonString();
}


void koOptions::SetString(string field_name, string value){
  
  // This is not very beautiful, but you can't modify a BSONObj
  // So we will create a new BSON object that is a copy of the 
  // Current one and update the field
  mongo::BSONObjBuilder builder;
  builder.append(field_name, value);
  builder.appendElementsUnique(m_bson);
  m_bson = builder.obj();

}
void koOptions::SetInt(string field_name, int value){
  mongo::BSONObjBuilder builder;
  builder.append(field_name, value);
  builder.appendElementsUnique(m_bson);
  m_bson = builder.obj();
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
    mongo::BSONObj link_obj = m_bson["links"].Array()[index].Obj();
    retlink.type = link_obj["type"].String();
    retlink.crate = link_obj["crate"].Int();
    retlink.id = link_obj["link"].Int();
    retlink.node = link_obj["reader"].Int();
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
    mongo::BSONObj board_obj = m_bson["boards"].Array()[index].Obj();
    retboard.type = board_obj["type"].String();
    retboard.vme_address = koHelper::StringToHex(board_obj["vme_address"].String());
    retboard.id = koHelper::StringToInt(board_obj["serial"].String());
    retboard.crate = board_obj["crate"].Int();
    retboard.link = board_obj["link"].Int();
    retboard.node = board_obj["reader"].Int();
  }
  catch(...){
    return dummy;
  }
  return retboard;
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
    mongo::BSONObj vme_obj = m_bson["registers"].Array()[index].Obj();
    retreg.address = koHelper::StringToHex(vme_obj["register"].String());
    retreg.value = koHelper::StringToHex(vme_obj["value"].String());
    retreg.board = koHelper::StringToInt(vme_obj["board"].String());
    retreg.node = -1;
  }
  catch(...){
    return dummy;
  }
  return retreg;
}


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
    m_bson = mongo::fromjson(json_string);
    //cout<<m_bson.jsonString()<<endl;
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
    return m_bson[key].Array().size();
  }
  catch(...){
    return 0;
  }
}

bool koOptions::HasField(string field_name){
  return m_bson.hasField(field_name);
}

mongo::BSONElement koOptions::GetField(string key){
  mongo::BSONElement b;
  try{
    b = m_bson[key];
  }
  catch(...){
    std::cout<<"Error fetching option. Key doesn't seem to exist: "<<
      key<<std::endl;
    return mongo::BSONElement();
  }
  return b;
}

void koOptions::ToStream_MongoUpdate(string run_name, 
				     stringstream *retstream, string host_name){
  mongo::BSONObjBuilder new_mongo_obj;
  mongo::BSONObj mongo_obj;
  try{
    mongo_obj = m_bson["mongo"].Obj();
  } catch ( ... ) {}
  new_mongo_obj.append("collection", run_name);
  if(host_name != "")
    new_mongo_obj.append("address", host_name);
  new_mongo_obj.appendElementsUnique(mongo_obj);

  // For dynamic collection names
  mongo::BSONObjBuilder builder;
  builder.append("mongo", new_mongo_obj.obj());
  //builder.append("mongo_collection", run_name );
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
mongo_option_t koOptions::GetMongoOptions(){

  mongo_option_t ret;
  ret.address = "";
  ret.database = "";
  ret.collection = "DEFAULT";
  ret.index_string = "";
  ret.capped_size = 0;
  ret.unordered_bulk_inserts = false;
  ret.sharding = false;
  ret.shard_string = "";
  ret.write_concern = 0;
  ret.min_insert_size = 1;
  ret.indices = vector<string>();
  ret.hosts = map<string, string>();
  
  mongo::BSONObj mongo_obj;
  try{ 
    mongo_obj = m_bson["mongo"].Obj();
  }
  catch(...){
    return ret;
  }
  
  // Connectivity
  try{
    ret.address = mongo_obj["address"].String();
    ret.database = mongo_obj["database"].String();
  } catch(...){
    ret.database = "untriggered";
  }
  try{
    ret.collection = mongo_obj["collection"].String();
  } catch(...){
    ret.collection = "DEFAULT";
  }
  
  // Indices
  try{
    if(mongo_obj["indices"].Array().size()>0){
      ret.index_string = "{'";
      for(unsigned int x=0; x<mongo_obj["indices"].Array().size(); x++){
	//cout<<mongo_obj["indices"].Array()[x].String()<<endl;
	ret.indices.push_back(mongo_obj["indices"].Array()[x].String());
	ret.index_string += mongo_obj["indices"].Array()[x].String();
	ret.index_string += "': 1";
	if(x != mongo_obj["indices"].Array().size() -1)
	  ret.index_string += ", '";
      }
      ret.index_string += " }";
    }
  }  catch ( ... ){}
  
  // Capped
  try {
    ret.capped_size = mongo_obj["capped_size"].Long();
  } catch( ... ){
    try { 
      ret.capped_size = mongo_obj["capped_size"].Int();
    } catch( ... ) {}
  }

  // Bulk inserts
  try{ 
    if(mongo_obj["unordered_bulk_inserts"].Int() == 1)
      ret.unordered_bulk_inserts = true;
  } catch ( ... ) {}
  
  // Sharting
  try{
    if(mongo_obj["sharding"].Int() == 1)
      ret.sharding = true;
    string shard_key = mongo_obj["shard_key"].String();
    ret.shard_string = "{ " + shard_key + ": ";
    if(mongo_obj["hash_shard"].Int() == 1)
      ret.shard_string += "'hashed'}";
    else
      ret.shard_string += "1}";
  } catch( ... ) {}  

  try{
    ret.write_concern = mongo_obj["write_concern"].Int();
  } catch ( ... ) {}

  try{
    ret.min_insert_size = mongo_obj["min_insert_size"].Int();
  } catch ( ... ) {}

  // Split hosts. If there is a split hosts options set then
  // sharding will be disabled automatically.
  try{
    mongo::BSONObj hosts_obj = mongo_obj["hosts"].Obj();
    for( mongo::BSONObj::iterator i = hosts_obj.begin(); i.more(); ) {       
      mongo::BSONElement e = i.next();
      ret.hosts[e.fieldName()] = hosts_obj[e.fieldName()].String();
      ret.sharding = false; // there is at least one host here
    }
  }
  catch( ... ){}

  return ret;

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


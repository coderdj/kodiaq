#ifndef _KOOPTIONS_HH_
#define _KOOPTIONS_HH_

#include <string>
#include <sstream>
#include "mongo/bson/bson.h"

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
  int node;
};

/*! \brief Stores configuration information for one VME module.
 */
struct board_definition_t{
  string type;
  int vme_address;
  int id;
  int crate;
  int link;
  int node;
};

/*! \brief Generic storage container for a VME option.
 */
struct vme_option_t{
  u_int32_t address;
  u_int32_t value;
  int board;
  int node;
};


class koOptions{

public:
  koOptions();
  virtual ~koOptions();

  int Loaded(){ return fLoaded;}
  int ReadParameterFile(string filename);
  void SetBSON(mongo::BSONObj bson){
    m_bson = bson;
  };
  mongo::BSONObj ExportBSON(){
    return m_bson;
  };

  bool HasField(string field_name);
  int GetLinks(){
    return GetArraySize("links");  
  };
  link_definition_t GetLink(int x);
  int GetBoards(){
    return GetArraySize("boards");
  };
  board_definition_t GetBoard(int x);
  int GetVMEOptions(){ 
    return GetArraySize("registers");
  };
  vme_option_t GetVMEOption(int x);
 
  int GetInt(string field_name){    
    return GetField(field_name).Int();
  };
  string GetString(string field_name){
    return GetField(field_name).String();
  };
  void SetString(string field_name, string value);
  void SetInt(string field_name, int value);

  bool GetBool(string field_name){
    return GetField(field_name).Bool();
  };

  void ToStream(stringstream *retstream){
    (*retstream)<<m_bson.jsonString();
  };
  
  void ToStream_MongoUpdate(string run_name, stringstream *retstream);

private:
  int GetArraySize(string key);
  mongo::BSONElement GetField(string key);
  mongo::BSONObj m_bson;
  bool fLoaded;

};

#endif

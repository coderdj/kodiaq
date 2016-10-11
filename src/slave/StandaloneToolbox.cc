// ****************************************************                               
//
// kodiaq Data Acquisition Software
//
// File    : StandaloneToolbox.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 09.09.2016 
//
// Brief   : Various addon tools for standalone mode
//
// ****************************************************

#include "StandaloneToolbox.hh"

StandaloneToolbox::StandaloneToolbox(){
  fRunsDB = NULL;
  fRunsDBName = "runs";
  fRunsDBCollection = "runs";
  mongo::client::initialize();
}

StandaloneToolbox::StandaloneToolbox(string mongodbURI, string mongodbName,
				     string mongodbCollection){
  mongo::client::initialize();
  if(ConnectRunsDB(mongodbURI, mongodbName, mongodbCollection)!=0){
    fRunsDB = NULL;
    fRunsDBName ="runs";
    fRunsDBCollection = "runs";
  }
}

StandaloneToolbox::~StandaloneToolbox(){
  delete fRunsDB;
}

int StandaloneToolbox::ConnectRunsDB(string mongodbURI, string mongodbName, 
				     string mongodbCollection){
  string errmsg = "";
  mongo::ConnectionString URI = mongo::ConnectionString::parse(mongodbURI, errmsg);
  if(!URI.isValid()){
    cout<<"Bad connection string value for runs DB. Mongo says: "<<errmsg<<endl;
    return -1;
  }

  try{
    fRunsDB = URI.connect(errmsg);
  }
  catch( const mongo::DBException &e) {
    delete fRunsDB;
    fRunsDB = NULL;
    cout<<"Could not connect to runs database with URI: "<<mongodbURI<<endl<<
      "Mongo complains with error: "<<errmsg<<endl;
    return -1;
  }
  
  // We connected OK
  fRunsDBName = mongodbName;
  fRunsDBCollection = mongodbCollection;
  return 0;

}

int StandaloneToolbox::InsertRunDoc(koOptions *options, string comment, string name){

  if(fRunsDB==NULL){
    cout<<"Try to insert runs doc into a non-existent runs DB."<<endl;
    return -1;
  }

  mongo::BSONObjBuilder builder;
  builder.genOID();

  time_t nowTime;
  time(&nowTime);
  builder.append("name",name);
  builder.append("comment", comment);
  builder.append("ini", options->ExportBSON() );
  builder.appendTimeT("start", nowTime);

  mongo::BSONObj bObj = builder.obj();
  try{    
    fRunsDB->insert(fRunsDBName+"."+fRunsDBCollection,bObj);
  }
  catch(const mongo::DBException &e){
    cout<<"Tried to put run doc in database, mongo complains: "<<
      e.what()<<endl;
    return -1;
  }
  return 0;

}
int StandaloneToolbox::UpdateRunDoc(string name){
  if(fRunsDB==NULL){
    cout<<"Want to update the end time of this run but there's no mongo"<<endl;
    return -1;
  }
  
  time_t nowTime;
  time(&nowTime);
  mongo::BSONObjBuilder bo;
  bo << "findandmodify" << fRunsDBCollection.c_str() <<
    "query" << BSON("name" << name) << 
    "update" << BSON("$set" << BSON("end" <<
				    mongo::Date_t(1000*nowTime)));

  mongo::BSONObj comnd = bo.obj();
  mongo::BSONObj res;
  try{
    assert(fRunsDB->runCommand(fRunsDBName.c_str(),comnd,res));
  }
  catch(const mongo::DBException &e){
    cout<<"Couldn't update end time for run. Maybe there was no start time?"<<
      " Mongo says: "<<e.what()<<endl;
    return -1;
  }

  return 0;
}


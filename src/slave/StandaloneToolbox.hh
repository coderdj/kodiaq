#ifndef STANDALONETOOLBOX_HH_
#define STANDALONETOOLBOX_HH_

// ****************************************************                               
//
// kodiaq Data Acquisition Software
//
// File    : StandaloneToolbox.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 09.09.2016 
//
// Brief   : Various addon tools for standalone mode
//
// ****************************************************

#include <koOptions.hh>
#include <koLogger.hh>
#include <koHelper.hh>
#include "mongo/client/dbclient.h"
#include "mongo/util/net/hostandport.h"
#include "mongo/bson/bson.h"

class StandaloneToolbox
{

public:
  
  StandaloneToolbox();
  explicit StandaloneToolbox(string mongodbURI, string mongodbName,
			     string mongodbCollection);
  virtual ~StandaloneToolbox();

  int ConnectRunsDB(string mongodbURI, string mondogbName, 
		    string mongodbCollection);

  int InsertRunDoc(koOptions *options, string comment, string name);
  int UpdateRunDoc(string name);

private:
  
  mongo::DBClientBase *fRunsDB;
  string               fRunsDBName;
  string               fRunsDBCollection;

};


#endif

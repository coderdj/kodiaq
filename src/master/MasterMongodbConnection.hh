#ifndef MASTERMONGODBCONNECTION_HH_
#define MASTERMONGODBCONNECTION_HH_

// ****************************************************
// 
// kodiaq DAQ Software for XENON1T
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
// File    : MasterMongodbConnection.hh
// 
// Brief   : Facilitates connection between master and
//           mongodb database in order to update general
//           run information documents.
// 
// ****************************************************

#include "mongo/client/dbclient.h"
#include "XeDAQOptions.hh"
#include "XeDAQLogger.hh"

class MasterMongodbConnection
{
 public: 
   MasterMongodbConnection();
   explicit MasterMongodbConnection(XeDAQLogger *Log);
   virtual ~MasterMongodbConnection();
   
   int      Initialize(string user, string runMode, XeDAQOptions *options);  
                          // Tries to connect to db, 0 on success, -1 on failure
   int      UpdateEndTime();
                          // Set run end time on DB
      
 private:
   MongodbOptions_t           fMongoOptions;
   mongo::DBClientConnection  fMongoDB;
   mongo::OID                 fLastDocOID;
   XeDAQLogger               *fLog;
};

#endif
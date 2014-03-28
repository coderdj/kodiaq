#ifndef MASTERMONGODBCONNECTION_HH_
#define MASTERMONGODBCONNECTION_HH_

// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : MasterMongodbConnection.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
// 
// Brief   : Facilitates connection between master and
//           mongodb database in order to update general
//           run information documents.
// 
// ****************************************************

#include <koOptions.hh>
#include <koLogger.hh>
#include "mongo/client/dbclient.h"

class MasterMongodbConnection
{

 public: 
   
                MasterMongodbConnection();
   explicit     MasterMongodbConnection(XeDAQLogger *Log);
   virtual     ~MasterMongodbConnection();
   
   // 
   // Name    : Initialize
   // Purpose : Initialize the mongodb connection according to the 
   //           information in the koOptions object
   // 
   int          Initialize(string user, string runMode, koOptions *options);
   //
   // Name    : UpdateEndTime
   // Purpose : Updates run control document with the time the run ended.
   //           Since this field is created by this function this can also be 
   //           seen as confirmation from the DAQ that the user stopped the run.
   int          UpdateEndTime();
   //
      
 private:
   MongodbOptions_t           fMongoOptions;
   mongo::DBClientConnection  fMongoDB;
   mongo::OID                 fLastDocOID;
   koLogger                  *fLog;
};

#endif
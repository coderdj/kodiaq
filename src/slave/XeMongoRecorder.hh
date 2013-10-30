#ifndef _XEMONGORECORDER_HH_
#define _XEMONGORECORDER_HH_

// *********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : XeMongoRecorder.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 9.8.2013
// 
// Brief    : Interface with MongoDB
// 
// *********************************************************

#include "XeDAQRecorder.hh"
#include "../../mongo/mongo-cxx-driver-v2.4/src/mongo/client/dbclient.h"
#include "CBV1724.hh"

class XeMongoRecorder : public XeDAQRecorder  {
 public:
   XeMongoRecorder();
   virtual ~XeMongoRecorder();
   
   int               Initialize(XeDAQOptions *options);
   int               RegisterProcessor();
   int               InsertThreaded(vector <mongo::BSONObj> *insvec,int ID);
   MongodbOptions_t  GetOptions()  {
      return fMongoOptions;
   };
   void              ShutdownRecorder();
   
 private:
   vector <mongo::ScopedDbConnection*> fScopedConnections;
   MongodbOptions_t            fMongoOptions;
   string                      fDBName;
   string                      fMongoAddress;
};
#endif
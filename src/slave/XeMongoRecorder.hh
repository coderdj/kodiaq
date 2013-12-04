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

/*! \brief Class for reading out to a mongodb database.
 
    This is a thread-safe implementation of the mongodb C++ api which is called from processing threads. The mongo connection is defined in the DAQ options file and read into this class with a XeDAQOptions object. 
 */ 
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
};
#endif
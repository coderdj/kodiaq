// *********************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : XeMongoRecorder.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 9.8.2013
// 
// Brief    : Interface with MongoDB
// 
// *********************************************************

#include "XeMongoRecorder.hh"
#include <signal.h>

XeMongoRecorder::XeMongoRecorder() : XeDAQRecorder()
{
   fMongoOptions.ZipOutput=false;      
   fMongoOptions.Collection="data.test";             
}

XeMongoRecorder::~XeMongoRecorder()
{
   for(unsigned int x=0;x<fScopedConnections.size();x++)  {	
      fScopedConnections[x]->done();
//      delete fScopedConnections[x];
   }   
}

int XeMongoRecorder::Initialize(XeDAQOptions *options)
{
   fWriteMode = options->GetRunOptions().WriteMode;
   fMongoOptions = options->GetMongoOptions();
   mongo::DBClientConnection inter(fMongoOptions.DBAddress.c_str());
   if(fMongoOptions.WriteConcern)
     inter.setWriteConcern(mongo::W_NORMAL);
   else
     inter.setWriteConcern(mongo::W_NONE);
   fScopedConnections.clear();
   ResetError();
   return 0;   
}

int XeMongoRecorder::RegisterProcessor()
{
   if(fWriteMode==0) return 0;
   try{      
//      mongo::ScopedDbConnection *conn = new mongo::ScopedDbConnection(fDBName);
      
      mongo::ScopedDbConnection *conn = 
	mongo::ScopedDbConnection::getScopedDbConnection(fMongoOptions.DBAddress,2500.);
      fScopedConnections.push_back(conn);
   }   
   catch(const mongo::DBException &e)    {	
      stringstream err;
      err<<"Error connecting to mongodb "<<e.toString();
      gLog->SendMessage(err.str());
      return -1;
   }
   return fScopedConnections.size()-1;
}

void XeMongoRecorder::ShutdownRecorder()
{
   usleep(100);
   for(unsigned int x=0;x<fScopedConnections.size();x++){     
      fScopedConnections[x]->done();
//      delete fScopedConnections[x];
   }   
   fScopedConnections.clear();
   return;
}

int XeMongoRecorder::InsertThreaded(vector <mongo::BSONObj> *insvec,int ID)
{
   if(fWriteMode==0 || fScopedConnections.size()==0)   {
      delete insvec;
      return 0;
   }  
   
   if(ID>(int)fScopedConnections.size() || ID<0)  {
      delete insvec;
      gLog->SendMessage("Received request for out of scope insert.");
      return -1;
   }
   try{	
      (*fScopedConnections[ID])->insert(fMongoOptions.Collection.c_str(),(*insvec));
   }
   catch(const mongo::DBException &e){
      gLog->SendMessage("Caught Mongodb Exception");
      delete insvec;
      return -1;
   }
      	
/*   string err = (*fScopedConnections[ID])->getLastError(false,false,0,2000); //2 seconds
   if(!err.empty())          {	
      string error="Error from mongodb: ";
      error+=err;
      LogError(error);
      delete insvec;
      return -1;
   }
 */
   delete insvec;
   return 0;				
   
}




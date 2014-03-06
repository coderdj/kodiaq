// ****************************************************
// 
// kodiaq DAQ Software for XENON1T
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
// File    : MasterMongodbConnection.cc
// 
// Brief   : Facilitates connection between master and
//           mongodb database in order to update general
//           run information documents.
// 
// ****************************************************

#include "MasterMongodbConnection.hh"

MasterMongodbConnection::MasterMongodbConnection()
{
   fLog=NULL;
   fLastDocOID.clear();
}

MasterMongodbConnection::MasterMongodbConnection(XeDAQLogger *Log)
{
   fLog=Log;
   fLastDocOID.clear();
}

MasterMongodbConnection::~MasterMongodbConnection()
{
}

int MasterMongodbConnection::Initialize(string user, string runMode, XeDAQOptions *options)
{
   fMongoOptions = options->GetMongoOptions();
   
   //connect to mongodb
   try {
      fMongoDB.connect(fMongoOptions.DBAddress);
   }
   catch(const mongo::DBException &e)  {
      stringstream ss;
      ss<<"Problem connecting to mongo. Caught exception "<<e.what();
      if(fLog!=NULL) fLog->Error(ss.str());
      return -1;
   }
   
   //Create a bson object with the basic run information
   mongo::BSONObjBuilder b;
   b.genOID();
   b.append("runtype",runMode);
   b.append("user",user);
   b.append("compressed",fMongoOptions.ZipOutput);
   
   //put in start time
   time_t currentTime;
   struct tm *starttime;
   time(&currentTime);
   starttime = localtime(&currentTime);
   b.appendTimeT("starttime",mktime(starttime));
   
   
   //eventually putting the .ini file will go here
   
   
   //insert into collection
   mongo::BSONObj bObj = b.obj();
   try   {	
      fMongoDB.insert(fMongoOptions.Collection.c_str(),bObj);
   }
   catch (const mongo::DBException &e) {
      if(fLog!=NULL) fLog->Error("MasterMongodbConnection::Initialize - Error inserting run information doc.");
      return -1;
   }   
   mongo::BSONElement OIDElement;
   bObj.getObjectID(OIDElement);
   fLastDocOID=OIDElement.__oid();
//   if(!bObj.getObjectID(fLastDocOID) && fLog!=NULL)
//     fLog->Error("MasterMongodbConnection::Initialize - Error getting OID of last insert.");
   return 0;   
}


int MasterMongodbConnection::UpdateEndTime()
{
   if(!fLastDocOID.isSet())  {
      if(fLog!=NULL) fLog->Error("MasterMongodbConnection::UpdateEndTime - Want to stop run but don't have the _id field of the run info doc");
      return -1;
   }
   try  {
      time_t nowTime;
      struct tm *currenttime;
      time(&nowTime);
      currenttime = localtime(&nowTime);
      
      std::size_t pos;
      pos=fMongoOptions.Collection.find_first_of(".",0);
      
      string firstString = fMongoOptions.Collection.substr(0,pos);
      string secondString = fMongoOptions.Collection.substr(pos+1,fMongoOptions.Collection.size()-pos);
      cout<<firstString<<" "<<secondString<<endl;
      mongo::BSONObjBuilder b; 
      mongo::BSONObj res;
      b << "findandmodify" << secondString.c_str() <<
	"query" << BSON("_id" << fLastDocOID) << 
	"update" << BSON("$set" << BSON("endtime" << mongo::Date_t(1000*mktime(currenttime))));
      assert(fMongoDB.runCommand(firstString.c_str(),b.obj(),res));
   }
   catch (const mongo::DBException &e) {
      if(fLog!=NULL) fLog->Error("MasterMongodbConnection::UpdateEndTime - Error fetching and updating run info doc with end time stamp.");
      return -1;
   }   
   return 0;
}


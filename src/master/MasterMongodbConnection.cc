// ****************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : MasterMongodbConnection.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 05.03.2014
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

MasterMongodbConnection::MasterMongodbConnection(koLogger *Log)
{
   fLog=Log;
   fLastDocOID.clear();
   try     {	
      fMongoDB.connect("xedaq00");
   }   
   catch(const mongo::DBException &e)    {	
      stringstream ss;
      ss<<"Problem connecting to mongo. Caught exception "<<e.what();
      if(fLog!=NULL) fLog->Error(ss.str());
   }      
}

MasterMongodbConnection::~MasterMongodbConnection()
{
}

int MasterMongodbConnection::Initialize(string user, string runMode, string name,
					koOptions *options, bool onlineOnly)
{
   fMongoOptions = options->GetMongoOptions();
      
   //connect to mongodb
/*   try {
      fMongoDB.connect(fMongoOptions.DBAddress);
   }
   catch(const mongo::DBException &e)  {
      stringstream ss;
      ss<<"Problem connecting to mongo. Caught exception "<<e.what();
      if(fLog!=NULL) fLog->Error(ss.str());
      return -1;
   }*/
   
   //Create a bson object with the basic run information
   mongo::BSONObjBuilder b;
   b.genOID();
   b.append("runmode",runMode);
   b.append("runtype","bern_test_daq");
   b.append("starttime",0);
   b.append("user",user);
   b.append("compressed",fMongoOptions.ZipOutput);
   b.append("data_taking_ended",false);
   b.append("name",name);
   
   //put in start time
   time_t currentTime;
   struct tm *starttime;
   time(&currentTime);
   starttime = localtime(&currentTime);
   b.appendTimeT("starttimestamp",mktime(starttime));
   
   
   //eventually putting the .ini file will go here
   
   
   //insert into collection
   mongo::BSONObj bObj = b.obj();
   try   {	
      if(!onlineOnly)
	fMongoDB.insert(fMongoOptions.Collection.c_str(),bObj);
      fMongoDB.insert("online.runs",bObj);
   }
   catch (const mongo::DBException &e) {
      if(fLog!=NULL) fLog->Error("MasterMongodbConnection::Initialize - Error inserting run information doc.");
      return -1;
   }   
   mongo::BSONElement OIDElement;
   bObj.getObjectID(OIDElement);
   fLastDocOID=OIDElement.__oid();
   return 0;   
}


int MasterMongodbConnection::UpdateEndTime(bool onlineOnly)
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
//      cout<<firstString<<" "<<secondString<<endl;
      mongo::BSONObjBuilder b; 
      mongo::BSONObj res;
      b << "findandmodify" << secondString.c_str() <<
	"query" << BSON("_id" << fLastDocOID) << 
	"update" << BSON("$set" << BSON("endtimestamp" << mongo::Date_t(mktime(currenttime)) << "data_taking_ended" << true));

      string onlinesubstr = "runs";
      mongo::BSONObjBuilder bo;
      bo << "findandmodify" << onlinesubstr.c_str() << 
	"query" << BSON("_id" << fLastDocOID) << 
	"update" << BSON("$set" << BSON("endtimestamp" << mongo::Date_t(mktime(currenttime)) << "data_taking_ended" << true)); 
      
      if(!onlineOnly)
	assert(fMongoDB.runCommand(firstString.c_str(),b.obj(),res));      
      assert(fMongoDB.runCommand("online",bo.obj(),res));
   }
   catch (const mongo::DBException &e) {
      if(fLog!=NULL) fLog->Error("MasterMongodbConnection::UpdateEndTime - Error fetching and updating run info doc with end time stamp.");
      return -1;
   }   
   return 0;
}

void MasterMongodbConnection::SendLogMessage(string message, int priority)
{
/*   try   {	
      fMongoDB.connect("xedaq00");
   }   
   catch(const mongo::DBException &e)   {	
      stringstream ss;
      ss<<"Problem connecting to mongo. Caught exception "<<e.what();
      if(fLog!=NULL) fLog->Error(ss.str());
   }*/
   
   mongo::BSONObjBuilder b;
   b.genOID();
   b.append("message",message);  
   b.append("priority",priority);
   time_t currentTime;
   struct tm *starttime;
   time(&currentTime);
   starttime = localtime(&currentTime);
   b.appendTimeT("time",mktime(starttime));
   fMongoDB.insert("online.log",b.obj());
}

void MasterMongodbConnection::AddRates(koStatusPacket_t DAQStatus)
{
/*   try     {	
      fMongoDB.connect("xedaq00");
   }   
   catch(const mongo::DBException &e)    {	
      stringstream ss;
      ss<<"Problem connecting to mongo. Caught exception "<<e.what();
      if(fLog!=NULL) fLog->Error(ss.str());
   }*/
   
   for(unsigned int x=0;x<DAQStatus.Slaves.size(); x++)  {
      mongo::BSONObjBuilder b;
     // b.genOID();
      time_t currentTime;
      time(&currentTime);
      b.appendTimeT("createdAt",currentTime);
      b.append("node",DAQStatus.Slaves[x].name);
      b.append("bltrate",DAQStatus.Slaves[x].Freq);
      b.append("datarate",DAQStatus.Slaves[x].Rate);
      b.append("runmode",DAQStatus.RunMode);
      b.append("nboards",DAQStatus.Slaves[x].nBoards);
      b.append("timeseconds",(int)currentTime);
      fMongoDB.insert("online.rates",b.obj());
   }
   
   
}

void MasterMongodbConnection::UpdateDAQStatus(koStatusPacket_t DAQStatus)
{
   mongo::BSONObjBuilder b;
   //b.genOID();
   time_t currentTime;
   time(&currentTime);
   b.appendTimeT("createdAt",currentTime);
   b.append("timeseconds",(int)currentTime);
   b.append("mode",DAQStatus.RunMode);
   if(DAQStatus.DAQState==KODAQ_ARMED)
     b.append("state","Armed");
   else if(DAQStatus.DAQState==KODAQ_RUNNING)
     b.append("state","Running");
   else if(DAQStatus.DAQState==KODAQ_IDLE)
     b.append("state","Idle");
   else if(DAQStatus.DAQState==KODAQ_ERROR)
     b.append("state","Error");
   else
     b.append("state","Undefined");
   b.append("network",DAQStatus.NetworkUp);
   b.append("currentRun",DAQStatus.RunInfo.RunNumber);
   b.append("startedBy",DAQStatus.RunInfo.StartedBy);
   b.append("numSlaves",(int)DAQStatus.Slaves.size());
   fMongoDB.insert("online.daqstatus",b.obj());
}

int MasterMongodbConnection::CheckForCommand(string &command, string &second, string &third)
{
   if(fMongoDB.count("online.daqcommands") ==0)
     return -1;
   auto_ptr<mongo::DBClientCursor> cursor = fMongoDB.query("online.daqcommands",mongo::BSONObj());
   mongo::BSONObj b;
   if(cursor->more())
     b = cursor->next();
   else
     return -1;
   command=b.getStringField("command");
   second=b.getStringField("mode");
   third=b.getStringField("name");
   fMongoDB.remove("online.daqcommands",QUERY("command"<<"Start"));
   fMongoDB.remove("online.daqcommands",QUERY("command"<<"Stop")); 
   return 0;
}

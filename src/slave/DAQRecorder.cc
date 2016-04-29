// ***************************************************************
// 
// kodiaq Data Acquisition Software 
// 
// File      : DAQRecorder.cc
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 09.08.2013
// Updated   : 26.03.2014
// 
// Brief     : Classes for data output
// 
// ****************************************************************

#include "DAQRecorder.hh"

DAQRecorder::DAQRecorder()
{
   m_koLogger      = NULL;
   m_bErrorSet     = false;
   m_bInitialized  = false;
   pthread_mutex_init(&m_logMutex,NULL);
}

DAQRecorder::DAQRecorder(koLogger *kLog)
{
   m_koLogger      = kLog;
   m_bErrorSet     = false;
   m_bInitialized  = false;
   pthread_mutex_init(&m_logMutex,NULL);
}

DAQRecorder::~DAQRecorder()
{
  pthread_mutex_destroy(&m_logMutex);
}

bool DAQRecorder::QueryError(string &err)
{
   if(!m_bErrorSet) return false;
   err = m_sErrorText;
   m_sErrorText = "";
   m_bErrorSet  = false;
   return true;
}

void DAQRecorder::LogError(string err)
{
  pthread_mutex_lock(&m_logMutex);
  m_sErrorText = err;
  m_bErrorSet  = true;   
  if( m_koLogger!= NULL )
    m_koLogger->Error( err );
  pthread_mutex_unlock(&m_logMutex);
}

void DAQRecorder::LogMessage(string message)
{
  pthread_mutex_lock(&m_logMutex);
  if( m_koLogger != NULL )
    m_koLogger->Message(message);
  pthread_mutex_unlock(&m_logMutex);
}

void DAQRecorder::ResetError()
{
  pthread_mutex_lock(&m_logMutex);
  m_sErrorText = "";
  m_bErrorSet  = false;
  pthread_mutex_unlock(&m_logMutex);
}

#ifdef HAVE_LIBMONGOCLIENT
#include <sys/types.h>
#include <sys/wait.h>
//
// DAQRecorder_mongodb
// 

DAQRecorder_mongodb::DAQRecorder_mongodb()
                    :DAQRecorder()
{
  pthread_mutex_init(&m_childlock, NULL);
  m_DB_USER=m_DB_PASSWORD="";
}

DAQRecorder_mongodb::~DAQRecorder_mongodb()
{
  pthread_mutex_destroy(&m_childlock);
   CloseConnections();
}

DAQRecorder_mongodb::DAQRecorder_mongodb(koLogger *koLog, string DB_USER,
					 string DB_PASSWORD)
                    :DAQRecorder(koLog)
{
  m_DB_USER=DB_USER;
  m_DB_PASSWORD=DB_PASSWORD;
  pthread_mutex_init(&m_childlock, NULL);
}

void DAQRecorder_mongodb::CloseConnections()
{
   for(unsigned int x=0; x<m_vScopedConnections.size(); x++)  {	
     //m_vScopedConnections[x]->done();
     delete m_vScopedConnections[x];
   }   
   m_vScopedConnections.clear();
   pthread_mutex_destroy(&m_ConnectionMutex);

}

int DAQRecorder_mongodb::Initialize(koOptions *options)
{
   if(options == NULL) return -1;
   m_options = options;
   CloseConnections();
   ResetError();
   m_bInitialized = true;
   pthread_mutex_init(&m_ConnectionMutex,NULL);
      
   return 0;
}

int DAQRecorder_mongodb::RegisterProcessor()
{
  int retval=-1;
  mongo::DBClientBase *conn;
   
  // Create connection string
  mongo_option_t mongo_opts = m_options->GetMongoOptions();
  string connstring = mongo_opts.address;
  if(m_DB_USER!="" && m_DB_PASSWORD!=""){
    connstring=connstring.substr(10, connstring.size()-10);
    connstring = "mongodb://" + m_DB_USER + ":" + m_DB_PASSWORD +"@"+ connstring;
  }
  string errstring;
  mongo::ConnectionString cstring = 
    mongo::ConnectionString::parse(connstring, errstring);
     
  // Check if string is valid
  if(!cstring.isValid()){
    LogError(connstring);
    LogError("Invalid MongoDB connection string provided. Error returned: " + 
	     errstring);
    return -1;
  }
  errstring = "";
  try{
    mongo::client::initialize();
    conn = cstring.connect(errstring);

    //using mongo::client::initialize;
    //using mongo::client::Options;
    // Configure the mongo C++ client driver, enabling SSL and setting
    // the SSL Certificate Authority file to "mycafile".
    //initialize(Options().setSSLMode(Options::kSSLRequired));
    //mongo::client::initialize();
    

    // Set write concern
    if( mongo_opts.write_concern == 0 ){
      conn->setWriteConcern( mongo::WriteConcern::unacknowledged );
      LogMessage( "MongoDB WriteConcern set to NONE" );  
      LogMessage( mongo_opts.address );
    }
    else{      
      conn->setWriteConcern( mongo::WriteConcern::acknowledged );
      LogMessage( "MongoDB WriteConcern set to NORMAL" );
      LogMessage( mongo_opts.address );
    }
  }
  catch(const mongo::DBException &e)  {
    stringstream err;
    err<<"DAQRecorder_mongodb::RegisterProcessor - Error connecting to mongodb "<<e.toString();
    LogError(err.str());
    return -1;
  }
  int lock = pthread_mutex_lock(&m_ConnectionMutex);
  if(lock!=0)  {
    return -1;
  }
  m_vScopedConnections.push_back(conn);
  retval = m_vScopedConnections.size()-1;
  pthread_mutex_unlock(&m_ConnectionMutex);
       
  return retval;
}

void DAQRecorder_mongodb::UpdateCollection(koOptions *options)
{
   m_options = options;
}

void DAQRecorder_mongodb::Shutdown()
{  
   CloseConnections();
   Fillicide(true);
   return;
}

void DAQRecorder_mongodb::Fillicide(bool wait){
  // Check on your children (should be fast)                                                                                                                    
  pthread_mutex_lock(&m_childlock);
  for(unsigned int x=0;x<m_children.size();x+=1){
    int status;
    pid_t result;
    if(!wait)
      result = waitpid(m_children[x], &status, WNOHANG);
    else
      result = waitpid(m_children[x], &status, 0);
    if (result == 0)
      continue;
    else if(result==-1)
      cout<<"Error in child process"<<endl;
    else
      m_children.erase(m_children.begin()+x);
  }
  pthread_mutex_unlock(&m_childlock);
}

int DAQRecorder_mongodb::InsertThreaded(vector <mongo::BSONObj> *insvec,
					 int ID)
{  
  Fillicide();

  // The ownership of insvec is passed to this function!
  mongo_option_t mongo_opts = m_options->GetMongoOptions();

  if(m_vScopedConnections.size()==0 || !m_bInitialized 
     || insvec->size() == 0)  {
      delete insvec;
      return 0;
   }
   
   if(ID>(int)m_vScopedConnections.size() || ID<0 )  {
      delete insvec;
      LogError("DAQRecorder_mongodb - Received request for out of scope insert.");
      return -1;
   }

   stringstream cS;
   cS<<mongo_opts.database<<"."<<
     mongo_opts.collection;

   // Fork the insert
   pid_t pid = fork();
   if (pid == 0)
     {
       // child process
       
       // Make grandchild. This kills zombie processes.
       // The grandchild should be adopted by the system once child dies.
       // Then exit leaves cleanly.
       //pid_t pid2 = fork();
       //if(pid2 == 0){
       //sleep(5);
       //_exit(2);
       //}

       try{
	 
	 mongo::DBClientBase *conn;
	 string errstring;
	 string connstring = mongo_opts.address;
	 mongo::ConnectionString cstring =
	   mongo::ConnectionString::parse(connstring, errstring);
	 mongo::client::initialize();
	 conn = cstring.connect(errstring);
	 
	 
	 // Make results object 
	 mongo::WriteResult RES;
	 mongo::WriteConcern WC;
	 
	 if(mongo_opts.write_concern == 0)
	   WC = mongo::WriteConcern::unacknowledged;
	 else
	   WC = mongo::WriteConcern::acknowledged;
	 
	 // Using mongo bulk op API     
	 if(mongo_opts.unordered_bulk_inserts){
	   mongo:: BulkOperationBuilder bulky = 
	     conn->initializeUnorderedBulkOp(cS.str());
	   //m_vScopedConnections[ID]->initializeUnorderedBulkOp(cS.str());
	   for(unsigned int i=0; i<insvec->size(); i+=1)
	     bulky.insert((*insvec)[i]);
	   bulky.execute(&WC, &RES);
	 }
	 else{
	   conn->insert(cS.str(), (*insvec) );
	   //( m_vScopedConnections[ID])->insert( cS.str(), (*insvec) );
	   /*  mongo::BulkOperationBuilder bulky =
	       m_vScopedConnections[ID]->initializeOrderedBulkOp(cS.str());
	       for(unsigned int i=0; i<insvec->size(); i+=1)
	       bulky.insert((*insvec)[i]);
	       bulky.execute(&WC, &RES);*/
	 }
	 
	 //old line
	 // ( m_vScopedConnections[ID])->insert( cS.str(), (*insvec) );
       }
       catch(const mongo::DBException &e)  {
	 stringstream elog;
	 elog<<"DAQRecorder_mongodb - Caught mongodb exception writing to "
	     <<cS.str()<<" : "<<e.what()<<endl;
	 LogError(elog.str());
	 delete insvec;	 
	 //return -1;
       }
       _exit(0);
     }
   pthread_mutex_lock(&m_childlock);
   m_children.push_back(pid);
   pthread_mutex_unlock(&m_childlock);

   delete insvec;
   return 0;            
}
#endif

#ifdef HAVE_LIBPBF
//
// DAQRecorder_protobuff
// 

DAQRecorder_protobuff::DAQRecorder_protobuff()
                      :DAQRecorder()
{
   m_SWritePath="data.dat";
}

DAQRecorder_protobuff::~DAQRecorder_protobuff()                     
{
}

DAQRecorder_protobuff::DAQRecorder_protobuff(koLogger *koLog)
                      :DAQRecorder(koLog)
{
   m_SWritePath="data.dat";
}

int DAQRecorder_protobuff::Initialize(koOptions *options)
{
  Shutdown();
  
  m_options=options;
   
   // get a file name
  if(m_options->GetInt("dynamic_run_names") == 1)  { //if we have time-based filenames
    std::size_t pos;
    pos = m_options->GetString("file_path").find_first_of("*",0);   
    if(pos>0 && pos<=m_options->GetString("file_path").size())
      m_SWritePath = m_options->GetString("file_path").substr(0,pos);
    else
      m_SWritePath = m_options->GetString("file_path");
    if(m_SWritePath[m_SWritePath.size()]!='/' && 
       m_SWritePath[m_SWritePath.size()]!='_')
      m_SWritePath+="_";
    m_SWritePath+=koHelper::GetRunNumber();
  }
  else
     m_SWritePath = m_options->GetString("file_path");
   
   stringstream sstream;
   if(m_options->GetInt("compression")==1)
     sstream<<"pz:";
   sstream<<"n"<<m_options->GetInt("file_events_per_file");
   
   if(m_outfile.open_file(m_SWritePath,sstream.str())!=0) return -1;
   
   //set header
   m_outfile.header().identifier=koHelper::GetRunNumber();
      
   return 0;
}

int DAQRecorder_protobuff::RegisterProcessor()
{
   return 0;
}

int DAQRecorder_protobuff::InsertThreaded()
{
  return 0;
}

void DAQRecorder_protobuff::Shutdown()
{
   m_outfile.close_file();

   return;
}

#endif


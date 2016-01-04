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
//
// DAQRecorder_mongodb
// 

DAQRecorder_mongodb::DAQRecorder_mongodb()
                    :DAQRecorder()
{
}

DAQRecorder_mongodb::~DAQRecorder_mongodb()
{
   CloseConnections();
}

DAQRecorder_mongodb::DAQRecorder_mongodb(koLogger *koLog)
                    :DAQRecorder(koLog)
{
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
   
  string errstring;
  mongo::ConnectionString cstring = 
    mongo::ConnectionString::parse(m_options->GetString("mongo_address"), 
				   errstring);
     
  // Check if string is valid
  if(!cstring.isValid()){
    LogError("Invalid MongoDB connection string provided. Error returned: " + 
	     errstring);
    return -1;
  }
  errstring = "";
  try{
    conn = cstring.connect(errstring);
    mongo::client::initialize();

    // Set write concern
    if( m_options->GetInt("mongo_write_concern") == 0 ){
      conn->setWriteConcern( mongo::WriteConcern::unacknowledged );
      LogMessage( "MongoDB WriteConcern set to NONE" );  
      LogMessage( m_options->GetString("mongo_address"));
    }
    else{      
      conn->setWriteConcern( mongo::WriteConcern::acknowledged );
      LogMessage( "MongoDB WriteConcern set to NORMAL" );
      LogMessage( m_options->GetString("mongo_address") );
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
   return;
}

int DAQRecorder_mongodb::InsertThreaded(vector <mongo::BSONObj> *insvec,
					 int ID)
{  // The ownership of insvec is passed to this function!
   
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
   cS<<m_options->GetString("mongo_database")<<"."<<
     m_options->GetString("mongo_collection");
   
   try{
     // New insert format. Put insvec into sub-docs of the main BSON
     /*if(m_options->GetString("mongo_output_format") == "trigger") {
       long long minTime = 0x7FFFFFFFFFFFFFFF, maxTime = -1; 
       mongo::BSONArrayBuilder subdoc_array;
       for( unsigned int x = 0; x < insvec->size(); x++ ) {
	 // insvec should be in temporal order somewhat, but might not be perfect. 
	 // so have to track min and max time manually
	 long long thisTime = (*insvec)[x].getField( "time" ).Long();
	 if ( thisTime < minTime ) minTime = thisTime;
	 if ( thisTime > maxTime ) maxTime = thisTime;
	 subdoc_array.append( (*insvec)[x] );
       }
       // Now make a master BSON doc
       mongo::BSONObjBuilder docBuilder;
       docBuilder.append( "time_min", minTime );
       docBuilder.append( "time_max", maxTime );
       docBuilder.appendArray( "bulk", subdoc_array.arr() );
       (m_vScopedConnections[ID])->insert( cS.str(), docBuilder.obj() );
     }
     else*/
     ( m_vScopedConnections[ID])->insert( cS.str(), (*insvec) );

   }
   catch(const mongo::DBException &e)  {
     stringstream elog;
     elog<<"DAQRecorder_mongodb - Caught mongodb exception writing to "
	 <<cS.str()<<" : "<<e.what()<<endl;
     LogError(elog.str());
      delete insvec;
      return -1;
   }
   
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


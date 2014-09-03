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
      m_vScopedConnections[x]->done();
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
   mongo::ScopedDbConnection *conn;
   try  {
     conn = new mongo::ScopedDbConnection(m_options->mongo_address,10000.);
   }
   catch(const mongo::DBException &e)  {
     stringstream err;
      err<<"DAQRecorder_mongodb::RegisterProcessor - Error connecting to mongodb "<<e.toString();
      LogError(err.str());
      cout<<err<<endl;
      return -1;
   }
   int lock = pthread_mutex_lock(&m_ConnectionMutex);
   if(lock!=0)  {
      conn->done();
      return -1;
   }
   m_vScopedConnections.push_back(conn);
   retval = m_vScopedConnections.size()-1;
   pthread_mutex_unlock(&m_ConnectionMutex);

   stringstream cS;
   int ID = m_vScopedConnections.size()-1;
   cS<<m_options->mongo_database<<"_"<<ID<<"."<<m_options->mongo_collection;
   //create capped collection
   conn->conn().createCollection(cS.str(),1000000000,true); //1GB capped collection 
      
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
   
   if(m_vScopedConnections.size()==0 || !m_bInitialized)  {
      delete insvec;
      return 0;
   }
   
   if(ID>(int)m_vScopedConnections.size() || ID<0)  {
      delete insvec;
      LogError("DAQRecorder_mongodb - Received request for out of scope insert.");
      return -1;
   }
   
   try  {
     stringstream cS;
     cS<<m_options->mongo_database<<"."<<m_options->mongo_collection;
     //(*m_vScopedConnections[ID])->ensureIndex(m_koMongoOptions.Collection.c_str(), mongo::fromjson("{time:-1}"));
     //(*m_vScopedConnections[ID])->insert(m_koMongoOptions.Collection.c_str(),
     //(*insvec));
     (*m_vScopedConnections[ID])->insert(cS.str(),(*insvec));
   }
   catch(const mongo::DBException &e)  {
      LogError("DAQRecorder_mongodb - Caught mongodb exception");
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
   if(m_options->dynamic_run_names == 1)  { //if we have time-based filenames
     std::size_t pos;
     pos = m_options->file_path.find_first_of("*",0);   
     if(pos>0 && pos<=m_options->file_path.size())
       m_SWritePath = m_options->file_path.substr(0,pos);
     else
       m_SWritePath = m_options->file_path;
     if(m_SWritePath[m_SWritePath.size()]!='/' && 
	m_SWritePath[m_SWritePath.size()]!='_')
       m_SWritePath+="_";
     m_SWritePath+=koHelper::GetRunNumber();
   }
   else
     m_SWritePath = m_options->file_path;
   
   stringstream sstream;
   if(m_options->compression==1)
     sstream<<"pz:";
   sstream<<"n"<<m_options->file_events_per_file;
   
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


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
   m_iResetCounter = 0;
   m_bTimeOverTen  = false;
   m_bInitialized  = false;
}

DAQRecorder::DAQRecorder(koLogger *kLog)
{
   m_koLogger      = kLog;
   m_bErrorSet     = false;
   m_iResetCounter = 0;
   m_bTimeOverTen  = false;
   m_bInitialized  = false;
}

DAQRecorder::~DAQRecorder()
{
}

int DAQRecorder::GetResetCounter(u_int32_t currentTime)
{
   int a=GetCurPrevNext(currentTime);
   if(a==1) m_iResetCounter++;
   if(a==-1) return m_iResetCounter-1;
   return m_iResetCounter;
}

void DAQRecorder::ResetTimer()
{
   m_iResetCounter=0;
   m_bTimeOverTen=false;
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
   m_sErrorText = err;
   m_bErrorSet  = true;   
}

void DAQRecorder::ResetError()
{
   m_sErrorText = "";
   m_bErrorSet  = false;
}

int DAQRecorder::GetCurPrevNext(u_int32_t timestamp)
{
   if(timestamp<3E8 && m_bTimeOverTen)  { //within first 3 seconds, reset ToT
      m_bTimeOverTen = false;
      return 1;
   }
   else if(timestamp>1E9 && !m_bTimeOverTen)  //after 10 sec and ToT not set yet
     m_bTimeOverTen = true;
   else if(timestamp>2E9 && !m_bTimeOverTen)
     return -1;
   return 0;      
}

#ifdef HAS_MONGODB
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
   pthread_mutex_destroy(&m_ConnectionMutex);
}

int DAQRecorder_mongodb::Initialize(koOptions *options)
{
   if(options == NULL) return -1;
   m_koMongoOptions = options->GetMongoOptions();
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
      conn = mongo::ScopedDbConnection::
	getScopedDbConnection(m_koMongoOptions.DBAddress,10000.);
      //or
      //conn = new mongo::ScopedDbConnection(m_koMongoOptions.DBAddress,10000.);
   }
   catch(const mongo::DBException &e)  {
      stringstream err;
      err<<"DAQRecorder_mongodb::RegisterProcessor - Error connecting to mongodb "<<
	e.toString();
      LogError(err.str());
      return -1;
   }
   int lock = pthread_mutex_lock(&m_ConnectionMutex);
   if(lock!=0)  {
      conn->done();
      return -1;
   }
   m_vScopedConnections.push_back(conn);
   retval = m_vScopedConnections.size()-1;
   lock = pthread_mutex_unlock(&m_ConnectionMutex);
   //but what to do if lock!=0? I guess just ignore?
   
   return retval;
}

void DAQRecorder_mongodb::UpdateCollection(koOptions *options)
{
   m_koMongoOptions = options->GetMongoOptions();
}

void DAQRecorder_mongodb::Shutdown()
{
   CloseConnections();
   pthread_mutex_destroy(&m_ConnectionMutex);
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
      (*m_vScopedConnections[ID])->insert(m_koMongoOptions.Collection.c_str(),
					  (*insvec));
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
//
// DAQRecorder_protobuff
// 

DAQRecorder_protobuff::DAQRecorder_protobuff()
                      :DAQRecorder()
{
   pthread_mutex_init(&m_CBuffMutex,NULL);
}

DAQRecorder_protobuff::~DAQRecorder_protobuff()                     
{
   pthread_mutex_destroy(&m_CBuffMutex);
}

DAQRecorder_protobuff::DAQRecorder_protobuff(koLogger *koLog)
                      :DAQRecorder(koLog)
{
   pthread_mutex_init(&m_CBuffMutex,NULL);
}

int DAQRecorder_protobuff::Initialize(koOptions *options)
{
   return 0;
}

int DAQRecorder_protobuff::RegisterProcessor()
{
   vector<int> empty;
   return RegisterProcessor(empty);
}

int DAQRecorder_protobuff::RegisterProcessor(vector <int> Digitizers)
{
   //Take a vector of digitizer IDs. This will be used for sorting.
   int retval = -1;
   
   int lock = pthread_mutex_lock(&m_CBuffMutex);
   if(lock!=0)  {
      LogError("DAQRecorder_protobuff::RegisterProcessor - Failed to get mutex lock.");
      return retval;
   }
   ProtoChannelBuffer cBuff;
   cBuff.Digis = Digitizers;
   pthread_mutex_init(&cBuff.Mutex,NULL);
   m_vPChanBuffs.push_back(cBuff);
   retval = m_vPChanBuffs.size()-1;   
   pthread_mutex_unlock(&m_CBuffMutex);
   
   return retval;
}

void DAQRecorder_protobuff::CloseWriteBuffer()
{
   for(unsigned int x=0; x<m_vPChanBuffs.size(); x++)  {
      //delete any remaining data
      pthread_mutex_destroy(&m_vPChanBuffs[x].Mutex);
   }
   m_vPChanBuffs.clear();   
}

int DAQRecorder_protobuff::InsertThreaded(int ID)
{
   //should insert into m_vPChanBuffs[ID]
   return 0;
}

void DAQRecorder_protobuff::Shutdown()
{
   CloseWriteBuffer();
}

void DAQRecorder_protobuff::WriteToFile()
{
}


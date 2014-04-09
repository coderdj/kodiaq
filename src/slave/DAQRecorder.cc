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
#include "protBuffDef.pb.h"

DAQRecorder_protobuff::DAQRecorder_protobuff()
                      :DAQRecorder()
{
   pthread_mutex_init(&m_BuffMutex,NULL);
   pthread_mutex_init(&m_OutfileMutex,NULL);
   iEventNumber = 0;
   m_protOOut = NULL;
   m_protCOut  = NULL;
   m_SWritePath="data.dat";
   m_SWriteNumber="";
}

DAQRecorder_protobuff::~DAQRecorder_protobuff()                     
{
   pthread_mutex_destroy(&m_BuffMutex);
   pthread_mutex_destroy(&m_OutfileMutex);
}

DAQRecorder_protobuff::DAQRecorder_protobuff(koLogger *koLog)
                      :DAQRecorder(koLog)
{
   pthread_mutex_init(&m_BuffMutex,NULL);
   pthread_mutex_init(&m_OutfileMutex,NULL);
   iEventNumber = 0;
   m_protOOut = NULL;
   m_protCOut  = NULL;
   m_SWritePath="data.dat";
   m_SWriteNumber="";
}

int DAQRecorder_protobuff::Initialize(koOptions *options)
{
   Shutdown();
   
   m_FileOptions = options->GetOutfileOptions();

   // get a file name
   if(m_FileOptions.DynamicRunNames == 1)  { //if we have time-based filenames
      std::size_t pos;
      pos = m_FileOptions.Path.find_first_of("*",0);   
      if(pos>0 && pos<=m_FileOptions.Path.size())
	m_SWritePath = m_FileOptions.Path.substr(0,pos);
      else
	m_SWritePath = m_FileOptions.Path;
      if(m_SWritePath[m_SWritePath.size()]!='/' && 
	 m_SWritePath[m_SWritePath.size()]!='_')
	m_SWritePath+="_";
      m_SWritePath+=koHelper::GetRunNumber();
   }
   else
     m_SWritePath = m_FileOptions.Path;
   
   if(m_FileOptions.EventsPerFile!=-1)
     m_SWriteNumber = "0000";
   iEventNumber = 0;
   return OpenFile();
}

int DAQRecorder_protobuff::OpenFile()
{
   pthread_mutex_lock(&m_OutfileMutex);
   
   if(m_protCOut!=NULL) delete m_protCOut;
   if(m_protOOut!=NULL) delete m_protOOut;
   if(m_Outfile.is_open()) m_Outfile.close();
   
   string fileTot = m_SWritePath;
   if(m_SWriteNumber!="") {
      fileTot += ".";
      fileTot += m_SWriteNumber;
   }
   fileTot += ".kodata";
   
   m_Outfile.open(fileTot.c_str(), ios::out | ios::trunc | ios::binary);
   if(!m_Outfile.is_open())   {	
      pthread_mutex_unlock(&m_OutfileMutex);
      return -1;
   }   
   m_protOOut = new google::protobuf::io::OstreamOutputStream(&m_Outfile);
   m_protCOut  = new google::protobuf::io::CodedOutputStream(m_protOOut);   
   pthread_mutex_unlock(&m_OutfileMutex);
   return 0;
}


int DAQRecorder_protobuff::RegisterProcessor()
{
   return 0;
}

int DAQRecorder_protobuff::InsertThreaded(vector<kodiaq_data::Event*> *vInsert)
{
   //should insert into m_vPChanBuffs[ID]
   if(vInsert->size()==0) {
      delete vInsert;
      return 0;
   }
   
   pthread_mutex_lock(&m_BuffMutex);
   for(unsigned int x=0; x<vInsert->size(); x++)  {
      m_vBuffer.push_back((*vInsert)[x]);
   }
   pthread_mutex_unlock(&m_BuffMutex);
   delete vInsert;   
   
   return WriteToFile();   
}

void DAQRecorder_protobuff::IncrementFileNumber()
{
   if(m_SWriteNumber=="") return;
   int num = koHelper::StringToInt(m_SWriteNumber);
   num++;
   if(num>=10000){	
      LogError("Exceeded 10000 output files!");
      m_SWriteNumber="rest";
      m_FileOptions.EventsPerFile=-1;
      return;
   }   
   stringstream ss;
   ss << setfill('0') << setw(4) << num;
   m_SWriteNumber = ss.str();       
}

void DAQRecorder_protobuff::Shutdown()
{
   WriteToFile(); //write data if possible
   
   //clear buffer if anything is left and put size to zero
   pthread_mutex_lock(&m_BuffMutex);
   for(unsigned int x=0; x<m_vBuffer.size(); x++)    {      
      //delete any remaining data
      delete m_vBuffer[x];
   }
   m_vBuffer.clear();
   pthread_mutex_unlock(&m_BuffMutex);
   
   //Close file
   pthread_mutex_lock(&m_OutfileMutex);
   if( m_protCOut  != NULL) delete m_protCOut;   
   if( m_protOOut != NULL) delete m_protOOut;
   if(m_Outfile.is_open()) m_Outfile.close();   
   m_protOOut=NULL;
   m_protCOut=NULL;
   pthread_mutex_unlock(&m_OutfileMutex);
   
   return;
}

int DAQRecorder_protobuff::WriteToFile()
{
   if(!m_Outfile.is_open()) return -1;
   pthread_mutex_lock(&m_BuffMutex);
   pthread_mutex_lock(&m_OutfileMutex);
   for(unsigned int x=0; x<m_vBuffer.size(); x++)  {
      
      // Have to set a unique event number. This starts at zero for
      // each file
      m_vBuffer[x]->set_number(iEventNumber);
      iEventNumber++;
  
      //create string with data
      string s="";      
      m_vBuffer[x]->SerializeToString(&s);
      //write size
      m_protCOut->WriteVarint32(s.size());
      //write data
      m_protCOut->WriteRaw(s.data(),s.size());

      //check if a new file needs to be opened
      if(iEventNumber>(m_FileOptions.EventsPerFile * 
		      ((koHelper::StringToInt(m_SWriteNumber))+1)) &&
	 m_FileOptions.EventsPerFile!=-1)	{
	 IncrementFileNumber();
	 pthread_mutex_unlock(&m_OutfileMutex);
	 if(OpenFile()!=0) return -1;
	 pthread_mutex_lock(&m_OutfileMutex);
      }
      
      delete m_vBuffer[x];
   }
   pthread_mutex_unlock(&m_OutfileMutex);
   m_vBuffer.clear();
   pthread_mutex_unlock(&m_BuffMutex);
   return 0;   
}


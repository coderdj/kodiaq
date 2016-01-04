#ifndef DAQRECORDER_HH_
#define DAQRECORDER_HH_

// ***************************************************************
// 
// kodiaq Data Acquisition Software 
// 
// File      : DAQRecorder.hh
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 09.08.2013
// Updated   : 26.03.2014
// 
// Brief     : Classes for data output
//  
// ****************************************************************

#include <koLogger.hh>
#include <koOptions.hh>
#include <koHelper.hh>
#include <pthread.h>
#include <iomanip>

using namespace std;

class DAQRecorder
{
   
 public:
   
                 DAQRecorder();
   virtual      ~DAQRecorder();
                 DAQRecorder(koLogger *kLog);
   
   //
   // Virtual functions must be overwritten by derived classes. The 
   // Initialize function should prepare the recorder for writing 
   // data. The Shutdown function should close all files/connections
   // and reset the object. Register processor should do any configuration
   // necessary to interface a processing thread with the recorder.
   // 
   virtual int   Initialize(koOptions *options) = 0;
   virtual int   RegisterProcessor()            = 0;
   virtual void  Shutdown()                     = 0;
      
   // Name     : bool DAQRecorder::QueryError(string err)
   // Purpose  : Returns true if an error was logged in recording with the 
   //            error string passed by reference.
   //
   bool          QueryError(string &err);
 
 protected:
   
   // Name     : void DAQRecorder::LogError(string err)
   // Purpose  : Sets the error flag to true and sets error text to err
   // 
   void          LogError(string err);
   void           LogMessage(string message);
   void          ResetError();
   
   koLogger     *m_koLogger;
   koOptions    *m_options;
   bool          m_bInitialized;
 private:
   
   // Name     : int DAQRecorder::GetCurPrevNext(u_int32_t timestamp,
   //                                            bool b30BitTimes)
   // Purpose  : Checks to see if this timestamp is in the current, previous,
   //            or next clock cycle (0,-1,1 return value)
   // 
   bool          m_bErrorSet;
   string        m_sErrorText;
   pthread_mutex_t m_logMutex, m_resetMutex;
};

#ifdef HAVE_LIBMONGOCLIENT

#include "mongo/client/dbclient.h"

/*! \brief Derived class for recording to a mongodb database
   
      Derived class of DAQRecorder designed to insert data into a mongodb
      database. Takes in user options to determine the database parameters
      from the .ini file.
   */
class DAQRecorder_mongodb : public DAQRecorder
{
   
 public:
    
                  DAQRecorder_mongodb();
   virtual       ~DAQRecorder_mongodb();
                  DAQRecorder_mongodb(koLogger *koLog);
   
   //
   // Name      : int DAQRecorder_mongodb::Initialize(koOptions* options)
   // Purpose   : Initialize the data recorder by trying to connect to the
   //             mongodb database defined in options. Return 0 if everything
   //             worked and negative if it didn't.
   // 
   int            Initialize(koOptions *options);
   //
   // Name      : int DAQRecorder_mongodb::RegisterProcessor()
   // Purpose   : A data processor registers with the recorder using this
   //             function and received an ID value. This ID should be 
   //             used when recording data. Internally, a scoped connection
   //             to the mongo db is created just for this processor.
   // 
   int            RegisterProcessor();
   //
   // Name      : int DAQRecorder_mongodb::Shutdown()
   // Purpose   : When the DAQ is done with the mongo connection is can be
   //             terminated using this function
   // 
   void           Shutdown();
   //
   // Name      : int DAQRecorder_mongodb::InsertThreaded
   //                   (vector <mongo::BSONObj> *insvec, int ID)
   // Purpose   : Used to insert a vector of BSON documents into the mongodb
   //             using a thread-safe method from mongo
   // 
   int            InsertThreaded(vector <mongo::BSONObj> *insvec, int ID);
   //
   // Name      : int DAQRecorder_mongodb::UpdateCollection
   // Purpose   : Change the mongodb collection without disconnecting
   // 
   void           UpdateCollection(koOptions *options);
   //
   
 private:
   //
   void            CloseConnections();

   pthread_mutex_t m_ConnectionMutex;
  //vector <mongo::ScopedDbConnection*> m_vScopedConnections;   
  vector <mongo::DBClientBase*> m_vScopedConnections;
   
};
#endif

#ifdef HAVE_LIBPBF

#include <pbf/pbf_output.hh>


/*! \brief Derived class for recording to file using google protocol buffers
    
       Derived class of DAQRecorder designed to write data to file
       using google protocol buffers.
    */
class DAQRecorder_protobuff : public DAQRecorder
{   
   
 public:   
                  DAQRecorder_protobuff();
   virtual       ~DAQRecorder_protobuff();
   explicit       DAQRecorder_protobuff(koLogger *koLog);
   
   // Name      : int DAQRecorder_protobuff::Initialize(koOptions* options)
   // Purpose   : Initialize the data recorder by trying to open the output
   //             stream and set up the write class. Return 0 on success and
   //             negative on failure.
   //
   int            Initialize(koOptions *options);
   //
   // Name      : int DAQRecorder_protobuff::RegisterProcessor()
   // Purpose   : A data processor registers with the recorder using this
   //             function and receives an ID value. 
   // 
   int            RegisterProcessor();
   //
   // Name      : int InsertThreaded(vector <kodiaq::Event> *vInsert)
   // Purpose   : Insert data into the output buffer. This is thread-safe.
   //             Ownership of vInsert and its elements is passed to the 
   //             recorder.
   // 
   int            InsertThreaded();
   //
   // Name      : int DAQRecorder_protobuff::Shutdown()
   // Purpose   : When the DAQ is done with the file it can be close 
   //             using this function
   //    
   void           Shutdown();
   //
   pbf_output*    GetOutfile()  {
      return &m_outfile;
   };
   
   
 private:
   //u_int32_t           iEventNumber;
   pbf_output           m_outfile;
   map <u_int64_t, int> m_HandleMap;
   string              m_SWritePath;
};

#endif

#endif

#ifndef _DATAPROCESSOR_HH_
#define _DATAPROCESSOR_HH_

// *************************************************************
//
// kodiaq Data Acquisition Software
// 
// File      : DataProcessor.hh
// Author    : Daniel Coderre, LHEP, Universitaet Bern
// Date      : 19.09.2013
// Updated   : 25.03.2014
// 
// Brief     : Data formatting and processing. Sits between 
//             digitizer output buffer and file/db write buffer
//
// *************************************************************


#include "DAQRecorder.hh"

#define REC_NONE 0
#define REC_FILE 1
#define REC_MONGO 2

using namespace std;
class DigiInterface;

/*! \brief Class for processing data between readout and storage routines.
 
    This class should be used to format the data. The base class features 
    the ability to split CAEN data blocks into sub-blocks corresponding to 
    single triggers or even occurrences. Derived classes should override the
    WProcess function to make a pthread-compatible function that formats the 
    data into their specific data objects.
  */
class DataProcessor
{
 public:
   
              DataProcessor();
   virtual   ~DataProcessor();
   explicit   DataProcessor(DigiInterface *digi, DAQRecorder *recorder,
			   koOptions *options);
   static void* WProcess(void* data);
   
   //
   // Name     : bool DataProcessor::QueryError(string &err)
   // Purpose  : Check if there was an error in the thread. If so then fill the
   //            string err with the error and return true. 
   // 
   bool QueryError(string &err);
   
 protected:
   //
   // Data formatting functions
   //
   // Name     : void DataProcessor::SplitData(vector<u_int32_t*>*&buffvec,
   //                                            vector<u_int32_t> *&sizevec)
   // Purpose  : Splits block transfers into individual triggers for non-custom
   //            CAEN V1724 firmware
   // 
   void         SplitData(vector <u_int32_t*> *&buffvec, 
			  vector<u_int32_t> *&sizevec);
   //
   // Name     : void DataProcessor::SplitDataChannels(vector<u_int32_t*> *&buffvec,
   //                                                    vector<u_int32_t> *&sizevec,
   //                                                    vector<u_int32_t> *timestamps,
   //                                                    vector<u_int32_t> *channels)
   // Purpose  : Find block splitting for non-custom CAEN V1724 firmware. Splits
   //            zero-length-encoded data into "occurrences". The four vectors should
   //            have the same size upon return where each data buffer corresponds
   //            to a size, a timestamp, and a channel. Run SplitData on the data
   //            first if there should be multiple events per BLT
   // 
   void        SplitDataChannels(vector <u_int32_t*> *&buffvec,
				 vector <u_int32_t>  *&sizevec,
				 vector <u_int32_t>   *timestamps,
				 vector <u_int32_t>   *channels,
				 vector <u_int32_t>  *eventIndices=NULL);
   //
   // Name      : void DataProcessor::SplitDataChannelsNewFW(vector <u_int32_t*> *&buffvec,
   //                                                        vector <u_int32_t> *&sizevec,
   //                                                        vector <u_int32_t> *timestamps,
   //                                                        vector <u_int32_t> *channels)
   // Purpose   : Split data blocks into channels without any channel parsing
   //
   void           SplitDataChannelsNewFW(vector <u_int32_t*> *&buffvec,
					  vector <u_int32_t> *&sizevec,
					  vector <u_int32_t> *timestamps,
					  vector <u_int32_t> *channels);
					  
   //
   // Name      : void DataProcessor::LogError(string err)
   // Purpose   : The data processor routines can report errors using this
   //             function.
   // 
   void              LogError(string err);
   //
   // Name      : u_int32_t DataProcessor::GetTimeStamp(u_int32_t *buffer)
   // Purpose   : Send a normal CAEN buffer (with header) to this function and 
   //             it will extract the 31-bit trigger time tag.
   // 
   u_int32_t         GetTimeStamp(u_int32_t *buffer);
   

   // Access to private members
   DAQRecorder*   GetDAQRecorder(){
                  return m_DAQRecorder;
   };
   DigiInterface* GetDigiInterface(){
                  return m_DigiInterface;
   };

   koOptions        *m_koOptions;
   int              itype;
   virtual void Process()=0;
   
 private:
   
   DigiInterface    *m_DigiInterface;
   DAQRecorder      *m_DAQRecorder;
   bool              m_bErrorSet;
   string            m_sErrorText;
   
};



// Note on mongodb driver dependency: lite versions of the code
// may have no mongodb dependency defined. If this is the case
// then there will be no mongo output possible with this class.
// To use mongoDB dependency the variable HAVE_LIBMONGOCLIENT must be 
// defined.

#ifdef HAVE_LIBMONGOCLIENT

/*! \brief Derived class for creating mongodb BSON objects.
 
    Derived class of koDataProcessor designed to format data into mongodb
    BSON objects. Takes in user options to determine how the inserts are
    performed (i.e. bulk inserts, how big, what kind of processing, etc.)
 */
class DataProcessor_mongodb : public DataProcessor {

 public:
   
   DataProcessor_mongodb();
   virtual ~DataProcessor_mongodb();
   DataProcessor_mongodb(DigiInterface *digi, DAQRecorder *recorder,
			koOptions *options);
   
   //
   // Name      : virtual static void* DataProcessor_mongodb::WProcess(void* data);
   // Purpose   : Required pthread-compatible function to drive processing.
   // 
   //static void* WProcess(void* data);
   
// protected:
   
   //
   // Name      : void DataProcessor_mongodb::ProcessMongoDB();
   // Purpose   : This is called by the thread-safe function to actually do the
   //             processing
   // 
   void Process();
   
};

#endif

/* \brief Derived class for file output using google protocol buffers
 
   This class uses a custom protocol buffer class created for kodiaq data. It
   formats the raw data into these buffers and puts it to the protocol buffer
   recorder object. In the recorder object the data is sorted and writted
   to file in temporal order.
 */
class DataProcessor_protobuff : public DataProcessor {
   
 public:
   
   DataProcessor_protobuff();
   virtual ~DataProcessor_protobuff();
   DataProcessor_protobuff(DigiInterface *digi, DAQRecorder *recorder,
			  koOptions *options);

   //
   // Name      : virtual static void* koDataProcessor_protobuff::WProcess(void* data);
   // Purpose   : Required pthread-compatible function to drive processing
   // 
   //static void* WProcess(void* data);
   
// protected:
   
   //
   // Name      : void DataProcessor_protobuff::ProcessProtoBuff();
   // Purpose   : This is the function called in WProcess which actually does
   //             the formatting and processing. 
   // 
   void Process();

};


/* \brief Derived class for running without output (for debugging)                
   
 Sometimes it might be a good idea for debugging to run without saving the data
 anywhere. For example for testing performance and stability without filling 
 a disk. In this case this data processor can perform some block splitting, but
 then just delete the data instead of sending it to a recorder (the recorder
 sent in the constructor can be a NULL pointer)

*/

class DataProcessor_dump : public DataProcessor 
{
                                  
                                                                          
 public:                                                                  
                                                                          
      DataProcessor_dump();                                                        
      virtual ~DataProcessor_dump();                                               
      DataProcessor_dump(DigiInterface *digi, DAQRecorder *recorder,               
			 koOptions *options);                                       
                                                                                 
   // Name      : virtual static void* koDataProcessor_dump::WProcess(void* data)
   // Purpose   : Required pthread-compatible function to drive processing
   //                                                                     
   //static void* WProcess(void* data);                                     
   
// protected:                                                                 
   
   //                                                                     
   // Name      : void DataProcessor_dump::Process();
   // Purpose   : This is the function called in WProcess. In the end it should
   //             delete the data. It can also do formatting if required.
   //                                                                    
   void Process();                                              
   //                                                                    
   
}; 
#endif
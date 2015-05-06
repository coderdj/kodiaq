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
  void       Process();
  //
  // Name     : bool DataProcessor::QueryError(string &err)
  // Purpose  : Check if there was an error in the thread. If so then fill the
  //            string err with the error and return true. 
  // 
  bool QueryError(string &err);
  
  //
  // Data formatting functions
  //
  // Name     : void DataProcessor::SplitBlocks(vector<u_int32_t*>*&buffvec,
  //                                            vector<u_int32_t> *&sizevec)
  // Purpose  : Splits block transfers into individual triggers for non-custom
  //            CAEN V1724 firmware
  // 
  static void         SplitBlocks(vector <u_int32_t*> *&buffvec, 
			   vector<u_int32_t> *&sizevec);
  //
  // Name     : void DataProcessor::SplitChannels(vector<u_int32_t*> *&buffvec,
  //                                                    vector<u_int32_t> *&sizevec,
  //                                                    vector<u_int32_t> *timestamps,
  //                                                    vector<u_int32_t> *channels)
  // Purpose  : Fine block splitting for non-custom CAEN V1724 firmware. Splits
  //            zero-length-encoded data into "occurrences". The four vectors should
  //            have the same size upon return where each data buffer corresponds
  //            to a size, a timestamp, and a channel. Run SplitData on the data
  //            first if there should be multiple events per BLT
  // 
  static void        SplitChannels(vector <u_int32_t*> *&buffvec,
			    vector <u_int32_t>  *&sizevec,
			    vector <u_int32_t>   *timestamps,
			    vector <u_int32_t>   *channels,
				   vector <u_int32_t>  *eventIndices=NULL,
				   bool ZLE=true);
  //
  // Name      : void DataProcessor::SplitChannelsNewFW(vector <u_int32_t*> *&buffvec,
  //                                                        vector <u_int32_t> *&sizevec,
  //                                                        vector <u_int32_t> *timestamps,
  //                                                        vector <u_int32_t> *channels)
  // Purpose   : Split data blocks into channels without any channel parsing
  //
  static void           SplitChannelsNewFW(vector <u_int32_t*> *&buffvec,
				    vector <u_int32_t> *&sizevec,
				    vector <u_int32_t> *timestamps,
					   vector <u_int32_t> *channels,
					   bool &bErrorSet,
					   string &sErrorText);

private:  
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
  
  //
  // Name       : int DataProcessor::GetBufferIntegral( u_int32_t *buffvec, u_int32_t size )
  // Purpose    : Return the integral of the pulse in the buffer in digi units
  //
  int               GetBufferIntegral( u_int32_t *buffvec, u_int32_t size );
  
  // Access to private members
  //DAQRecorder*   GetDAQRecorder(){
  //return m_DAQRecorder;
  //};
  //DigiInterface* GetDigiInterface(){
  //return m_DigiInterface;
  //};
  
  koOptions        *m_koOptions;
  DigiInterface    *m_DigiInterface;
  DAQRecorder      *m_DAQRecorder;
  bool              m_bErrorSet;
  string            m_sErrorText;
  
};

#endif

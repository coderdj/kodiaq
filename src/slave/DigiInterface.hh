#ifndef _DIGIINTERFACE_HH_
#define _DIGIINTERFACE_HH_

// ******************************************************
// 
// kodiaq Data Acquisition Software
// 
// File     : DigiInterface.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 11.07.2013
// Update   : 31.03.2014
// 
// Brief    : Class for managing electronics and readout
// 
// ******************************************************

#include <koLogger.hh>
#include <pthread.h>
#include <koHelper.hh>
#include "CBV1724.hh"
#include "CBV2718.hh"
#include "DataProcessor.hh"

using namespace std;

/*! \brief Small thread holder.
 */ 
struct PThreadType
{
   pthread_t Thread;
   bool IsOpen;
};

/*! \brief Holder for processing threads.
 */ 
struct ProcThread
{
   pthread_t Thread;
   bool IsOpen;
   DataProcessor *Processor;
};

/*! \brief Control interface for all DAQ electronics.
  
    Electronics are defined in a config file which is processed by koOptions and used to initialize this object. This object then allows simple starting and stopping of runs, reading data, and access to the individual crates and boards.
 
    While the DAQ is running, the data is processed using processor threads. These threads are handled by this class and define the interface between the digitizers and the output.
 */ 

class DigiInterface
{
 public:
   DigiInterface();
   virtual ~DigiInterface();
  DigiInterface(koLogger *logger, int ID=-1);
   
  //General Control
  //
  // Name      : int DigiInterface::PreProcess
  // Input     : A 'loaded' koOptions object
  // Function  : Does baselines, noise spectra, and other preprocessing tasks
  // Return    : 0 on success, -1 on failure
  //int          PreProcess(koOptions *options);
  int          Arm(koOptions *options);
   //
   // Name     : int DigiInterface::StartRun()
   // Input    : None, but precondition is that "initialize" was called
   // Function : Starts the run via either NIM or software depending on how defined.
   //            Tells processing threads to start reading and working.
   // Output   : 0 on success, -1 if something goes wrong.
   // 
   int           StartRun();
   //
   // Name     : int DigiInterface::StopRun()
   // Input    : None, but only makes sense if run was started already
   // Function : Turns off the s-in and sets crates to inactive. Waits for threads
   //            to finish processing and closes them. Shuts down the recorder.
   // Output   : 0 on success, -1 on failure (failure not possible at the moment)
   // 
   int           StopRun();   
   //
   // Name     : u_int32_t DigiInterface::GetRate(u_int32_t &freq)
   // Input    : an empty unsigned int (or assigned, but it will be overwritten)
   // Function : Returns the transfered data size (bytes) and passes the num of 
   //            BLTs by reference
   // Output   : Data size since last call to GetRate and #BLTs since last call
   u_int32_t     GetRate(u_int32_t &freq);
   //
   // Name     : void DigiInterface::Close()
   // Input    : none
   // Function : Closes this object and resets everything.
   // Output   : none
   // 
   void          Close();
   //
   // Name     : bool DigiInterface::RunError(string &err)
   // Input    : none
   // Function : Checks data processors and DAQ recorder for runtime errors
   // Output   : true if there is an error and false otherwise. If there is an 
   //            error the string err is set.
   bool          RunError(string &err);
   //
   
   //
   //Access to crates and boards
   //
   CBV1724*  GetDigi(int x) {
      return m_vDigitizers[x];
   };
   unsigned int  GetDigis()  {
      return m_vDigitizers.size();
   };
   
   //
   //For read thread - not for user use but public since threads need to access
   //
   static void*  ReadThreadWrapper(void* digi);    

  // Get the buffer sizes of all digitizers. This corresponds roughly to the 
  // memory usage of the program (there's some overhead from the data currently 
  // being read out, object size, etc. but most of the memory usage is here). 
  // The vectors give the digitizer id's (digis) and the buffer size in each (sizes)
  // The return value is the total occupancy (sum of sizes)
  int GetBufferOccupancy( vector<int> &digis, vector<int> &sizes);
   
 private:   
  void          ReadThread();       // Only need 1 read thread since V1724
                                    // reads can't be parallelized
  void          CloseThreads(bool Completely=false);
  bool          LockRateMutex();    // Need a mutex for the rate since reads are
  bool          UnlockRateMutex();  // done in a separate thread but readout of
                                    // the rate is done in the main thread
  int           InitializeHardware(koOptions *options);

   //Threads
   vector<ProcThread>   m_vProcThreads;  
   PThreadType          m_ReadThread;
   PThreadType          m_WriteThread;
   
  // Electronics
  vector<CBV1724*>     m_vDigitizers;
  vector<int>         m_vCrateHandles;
  VMEBoard            *m_RunStartModule;
  DAQRecorder         *m_DAQRecorder;
  
  // Rate info
  unsigned int         m_iReadSize;
  unsigned int         m_iReadFreq;
  pthread_mutex_t      m_RateMutex;
  
  //kodiaq objects
  koOptions           *m_koOptions;
  koLogger            *m_koLog;
  int                  m_slaveID;
  
};

#endif

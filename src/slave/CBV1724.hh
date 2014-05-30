#ifndef _CBV1724_HH_
#define _CBV1724_HH_

// ****************************************************************
// 
// DAQ Control for Xenon-1t
// 
// File     : CBV1724.hh
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 12.07.2013
// 
// Brief    : Class for Caen V1724 digitizers
// 
// *****************************************************************

#include "VMEBoard.hh"
#include <pthread.h>

//Register definitions
#define V1724_BoardInfoReg                0x8140
#define V1724_BlockOrganizationReg        0x800C
#define V1724_BltEvNumReg                 0xEF1C
#define V1724_AcquisitionControlReg       0x8100
#define V1724_DACReg                      0x1098

/*! \brief Control class for CAEN V1724 digitizers.
 */ 
class CBV1724 : public VMEBoard {
 public: 
   CBV1724();
   virtual ~CBV1724();
   explicit CBV1724(BoardDefinition_t BoardDef, koLogger *kLog);   /*!   The preferred constructor. If you use the default constructor you have an empty board.*/


   int Initialize(koOptions *options);                             /*!<  Initialize all VME options using a XeDAQOptions object. Other run parameters are also set.*/
   unsigned int ReadMBLT();                                        /*!<  Performs a read cycle (reads from the buffer until buffer is exhausted or BERR is read) and puts the data into the CBV1724 object buffer. It is assumed that another process is clearing this object's buffer using the ReadoutBuffer function.*/
   BoardDefinition_t GetBoardDef()  {                              /*!   Return the board definition object, which holds the board's parameters as defined from the XeDAQOptions .ini file.*/
      return fBID;
   };  
   
   vector<u_int32_t*>* ReadoutBuffer(vector <u_int32_t> *&sizes);  /*!<  Passes a pointer to a vector of raw data that has been read from the board. Please note: this passes ownership of the raw data vector to the caller! That means the calling function is responsible for freeing this memory again later. A new buffer is created that will act as the board buffer until the next ReadoutBuffer call. The vector sizes contains the size in bytes of each element in the returned buffer. Ownership of sizes also passes to the caller.*/
   
   void ResetBuff();                                               /*!<  Clears and resets the object buffer.*/
   u_int32_t GetBLTSize()  {                                       /*!   Returns the block transfer size. */
      return fBLTSize;
   };

   int LockDataBuffer();                                           /*!<  This is a thread-safe program. In order to do anything with the buffer you have to lock the mutex. This function locks it for you.*/
   int UnlockDataBuffer();                                         /*!<  When a recorder function is done clearing the buffer of this object it can (and should) free the mutex so that acquisition can continue. As long as the user is accessing the buffer the digitizer cannot read any new data.*/
   int RequestDataLock();                                          /*!<  If the object's mutex is not locked this function puts it on hold using trylock. Trylock waits for a signal over a pthread_cond_t object which will indicate that the buffer is ready to be read. If the condition is meant this function returns true and the buffer can be accessed. If the condition is not meant (or if someone else was controlling the mutex) this function returns -1 and the caller should not try to access the buffer. This may seem a bit confusing, but this class was not meant to be used alone and should be accessed through the XeProcessor object, where all of these steps are done automatically.*/
   
   int DetermineBaselines();                                       /*!<  Simple baseline determination is performed. Basically this just takes data for some time and averages the value on the wire. The DAC register is adjusted iteratively until the baseline minimizes around 16000 (ADC units). There will be problems if there is a lot of activity on the channels, since obviously the baselines are not flat in this case. Do not try to call this function if there is a high rate on the channels or if a strong source is in. At best it will fail and revert back to the old baselines anyway while at worst it will determine poor baselines which can cause undefined behavior.*/
   void SetActivated(bool active);                                 /*!<  Set if this board is active (taking data).*/
   
 private:
   int                  LoadBaselines();                       //Load baselines to boards
   int                  GetBaselines(vector <int> &baselines, bool bQuiet=false);  //Get baselines from file 

   unsigned int         fReadoutThresh;
   pthread_mutex_t      fDataLock;
   pthread_cond_t       fReadyCondition;
   u_int32_t            fBufferSize;
   u_int32_t            fBLTSize;
   vector <u_int32_t>  *fSizes;
   vector <u_int32_t*> *fBuffers;
};

#endif
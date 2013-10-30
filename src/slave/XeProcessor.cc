// *************************************************************** 
//  
// DAQ Control for Xenon-1t  
//  
// File      : XeProcessor.cc
// Author    : Daniel Coderre, LHEP, Universitaet Bern 
// Date      : 19.9.2013 
//  
// Brief     : Intermediate data processor/router 
//  
// ****************************************************************

#include "XeProcessor.hh"
#include "DigiInterface.hh"
#include <snappy.h>

XeProcessor::XeProcessor()
{
   fDigiInterface = NULL;
   fDAQRecorder   = NULL;
   bErrorSet      = false;
   fErrorText     = "";
   fBLTLen        = 0;   
   fMinInsertSize = 1000000;
   fBlockSplitting= 0;
}

XeProcessor::~XeProcessor()
{
}

XeProcessor::XeProcessor(DigiInterface *digi, XeMongoRecorder *recorder)
{
   fDigiInterface = digi;
   fDAQRecorder   = recorder;
   bErrorSet      = false;
   fErrorText     = "";   
   fMinInsertSize = recorder->GetOptions().MinInsertSize;
   fBlockSplitting= recorder->GetOptions().BlockSplitting;
}

void* XeProcessor::WProcessMongoDB(void* data)
{
   XeProcessor *XeP = static_cast<XeProcessor*>(data);      
   XeP->ProcessMongoDB();
   return (void*)data;
}

void XeProcessor::LogError(string err)
{
   bErrorSet=true;
   fErrorText=err;
   return;
}

bool XeProcessor::QueryError(string &err)
{
   if(bErrorSet==false) return false;
   err=fErrorText;
   fErrorText="";
   bErrorSet=false;
   return true;
}

void XeProcessor::ProcessMongoDB()
{
   //Makes BSON objects and put them into fDAQRecorder's vector
   bool ExitCondition=false;
   u_int32_t fInsertSize=0;
   XeMongoRecorder *mongo = fDAQRecorder;
   int mongoID = mongo->RegisterProcessor();
   if(mongoID==-1){
      LogError("Failed to register mongo connection.");
      return;
   }   
   vector<u_int32_t*> *buffvec=NULL;
   vector<u_int32_t > *sizevec=NULL;
   vector <mongo::BSONObj> *fInsertVec = new vector<mongo::BSONObj>();

   while(!ExitCondition){// || !fDAQRecorder->IsOff())    {
      ExitCondition=true;              
      for(int x=0; x<fDigiInterface->NumCrates();x++)  {
	 for(unsigned int y=0;y<fDigiInterface->GetCrate(x)->GetDigitizers();y++)  {
	    CBV1724 *digi = (*fDigiInterface)(x,y);
	    if(digi->Activated())         ExitCondition=false;	   
	    if(digi->RequestDataLock()!=0) continue;
	    buffvec = digi->ReadoutBuffer(sizevec);	    
	    digi->UnlockDataBuffer();	    
	    //any data manipulation would go here
	    //
//          int dbgsizebefore=buffvec->size();
//          
//          *****************************
//          PROCESSING
//          *****************************

	    //default splitting
	    if(fBlockSplitting==1)
	      ProcessData(buffvec,sizevec);
	    
	    //channel splitting
//	    vector <u_int32_t> *channels = new vector<u_int32_t>();
//	    vector <u_int32_t> *times    = new vector<u_int32_t>();
//	    ProcessDataChannels(buffvec,sizevec,times,channels);
//	    *****************************
//	    END PROCESSING
//	    *****************************

//	    int dbgsizeafter = buffvec->size();
	    
//	    vector <mongo::BSONObj> *DataInsert = new vector<mongo::BSONObj>();
	    int ModuleNumber = digi->GetID().BoardID;
	    int nBuff        = buffvec->size();
	    
	    for(int b=0;b<nBuff;b++)  {
	       mongo::BSONObjBuilder bson;
	       u_int32_t *buff = (*buffvec)[b];
	       u_int32_t eventSize = (*sizevec)[b];
	       u_int32_t TimeStamp = GetTimeStamp(buff);//@
	       
//	       u_int32_t TimeStamp = (*times)[b];//!
//	       bson.append("channel",(*channels)[b]);//!
	       
	       int ID = mongo->GetID(TimeStamp);	       
	       long long mongoTime = ((unsigned long)ID << 31) | TimeStamp;
	       int insize = fInsertVec->size();
	       bson.append("insertsize",insize);	       
//	       bson.append("dbgsizebefore",dbgsizebefore);
//	       bson.append("dbgsizeafter",dbgsizeafter);
	       bson.append("module",ModuleNumber);
	       bson.append("triggertime",mongoTime);
//	       bson.append("block_splitting","todo");
	       if(mongo->GetOptions().ZipOutput) { //zip before sending
		  bson.append("zipped",true);
		  if(eventSize==0) { 
		     delete[] (*buffvec)[b];
		     continue;
		  }		  
		  char *compressed = new char[snappy::MaxCompressedLength(eventSize)];
		  size_t compressedLength=0;
		  snappy::RawCompress((const char*)buff,eventSize,
				      compressed,&compressedLength);
		  bson.append("datalength",(int)compressedLength);
		  fInsertSize+=compressedLength;
		  bson.appendBinData("data",(int) compressedLength,mongo::BinDataGeneral,
				  (const void*)compressed);
		  delete[] ((*buffvec)[b]);
		  delete[] compressed;
	       }//end zip output
	       else { //don't zip
		  bson.append("zipped",false);
		  bson.append("datalength",(int)eventSize);
		  fInsertSize+=eventSize;
		  bson.appendBinData("data",(int)eventSize,mongo::BinDataGeneral,
				  (const void*)buff);
		  delete[] (*buffvec)[b];
	       }	       
	       fInsertVec->push_back(bson.obj());
	       if((int)fInsertSize>fMinInsertSize)	 {		    
		  if(mongo->InsertThreaded(fInsertVec,mongoID)==0){ //PASSES OWNERSHIP $		     
		     fInsertSize=0;
		     fInsertVec = new vector<mongo::BSONObj>();
		  }
		  else      {
		     LogError("MongoDB insert error from processor thread");
		     ExitCondition=true;
		     fInsertVec=NULL;
		     break;
		  }
	       }
	       
	    }//end for through buffers	    
//	    delete channels;//!
//	    delete times;//!
	    	    	    
	    if(fInsertVec==NULL) break; 
	    if(buffvec!=NULL) {
	       delete buffvec;
	       buffvec=NULL;
	    }
	    if(sizevec!=NULL)  {
	       delete sizevec;
	       sizevec=NULL;
	    }	    
	 }//end for through digis
	 if(fInsertVec==NULL) break;
      }//end for through crates
   }//end while(!ExitCondition)
   if(fInsertVec!=NULL)
     delete fInsertVec;
   return;
}

u_int32_t XeProcessor::GetTimeStamp(u_int32_t *buffer)
{   
   int pnt=0;
   while((buffer[pnt])==0xFFFFFFFF)  
     pnt++;         
   if((buffer[pnt]>>20)==0xA00)   {	
      pnt+=3;
      return (buffer[pnt] & 0x7FFFFFFF);
   }   
   return 0;
}

void XeProcessor::ProcessData(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec)
{
   //General interface for processing the buffer
   //breaks BLT's into occurrences by locating headers and parsing
   
//   return; //if no processing   
   vector <u_int32_t*> *retbuff = new vector <u_int32_t*>();
   vector <u_int32_t > *retsize = new vector <u_int32_t >();
   
   for(unsigned int x=0; x<buffvec->size();x++)  {
      if((*sizevec)[x]==0)	{
	 delete[] (*buffvec)[x];
	 continue;
      }      
      unsigned int idx = 0;
      while(idx<((*sizevec)[x]/sizeof(u_int32_t)) && (*buffvec)[x][idx]!=0xFFFFFFFF)  {	   	   
	 if(((*buffvec)[x][idx]>>20) == 0xA00){	    
	    u_int32_t size = (*buffvec)[x][idx]&0xFFFF;
	    u_int32_t *rb = new u_int32_t[size];
//	    memcpy(rb,&((*buffvec)[x][idx])/*currentBuff[idx]*/,size*sizeof(u_int32_t));
	    copy((*buffvec)[x]+idx,(*buffvec)[x]+(idx+size),rb);
	    retbuff->push_back(rb);
	    retsize->push_back(size*sizeof(u_int32_t));
	    idx+=size;
	 }
	 else
	   idx++;	 
      }//end while
      delete[] (*buffvec)[x];           
   }
   delete buffvec;
   delete sizevec;
   buffvec=retbuff;
   sizevec=retsize;
   return;
}

void XeProcessor::ProcessDataChannels(vector<u_int32_t*> *&buffvec, vector<u_int32_t> *&sizevec,
			      vector<u_int32_t> *timeStamps, vector<u_int32_t> *channels)
{
   vector <u_int32_t*> *retbuff = new vector <u_int32_t*>();
   vector <u_int32_t>  *retsize = new vector <u_int32_t>();
   
   //timestamps,channels should be provided as pointers to empty vectors
   //ONLY WORKS WITH ZLE ON
   
   for(unsigned int x=0; x<buffvec->size();x++)  {
      //loop through main vector
      if((*sizevec)[x]==0)	{
	 delete[] (*buffvec)[x];
	 continue;
      }
      unsigned int idx=0;
      u_int32_t headerTime=0;
      u_int32_t channel=0;
      
      while((*buffvec)[x][idx]==0xFFFFFFFF) idx++;
      
      while(idx<(((*sizevec)[x])/(sizeof(u_int32_t)))){	   
	 if((*buffvec)[x][idx]==0xFFFFFFFF) break;    
	 if(((*buffvec)[x][idx]>>20)==0xA00) { //found a header
	    u_int32_t mask = ((*buffvec)[x][idx+1])&0xFF;
	    headerTime = ((*buffvec)[x][idx+3])&0x7FFFFFFF;
	    channel=0;
	    idx+=4;
	    
	    while(channel<8)  {
	       if(!((mask>>channel)&1))	 {
		  channel++;
		  continue;
	       }
	       	    
	       u_int32_t size = ((*buffvec)[x][idx]);
	    //   if(size<2)	 {	       
	//	  idx+=size;
	//	  channel++;
	//	  continue;
	  //     }	     
	       if(size<2)	 {
		  gLog->SendMessage("Bad buffer format! Possible lost data!");
		  break;
	       }
	       
	       size-=1;	       
	       idx++;
	       
	       u_int32_t *rb = new u_int32_t[size];
	       copy((*buffvec)[x]+(idx),(*buffvec)[x]+(idx+size),rb);
	       retbuff->push_back(rb);
	       retsize->push_back(size*sizeof(u_int32_t));
	       channels->push_back(channel);
	       timeStamps->push_back(headerTime);
	       channel++;
	       idx+=size;
	    }
	 }
	 else
	   idx++;
      }//end while
      delete[] (*buffvec)[x];      
   }
   delete buffvec;
   delete sizevec;
   buffvec=retbuff;
   sizevec=retsize;
   return;
}

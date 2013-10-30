// *************************************************************
// *************************************************************
// 
// File   : XeDAQOptions.cc
// Author : Daniel Coderre, LHEP, Universitaet Bern
// Date   : 27.6.2013
// 
// Brief  : Options handler for Xenon-1t DAQ software
// 
// *************************************************************
// *************************************************************

#include <iostream>

#include "XeDAQOptions.hh"

XeDAQOptions::XeDAQOptions()
{
   Reset();   
}

XeDAQOptions::~XeDAQOptions()
{
}

void XeDAQOptions::Reset()
{
   fWriteMode=0;
   fLinks.clear();
   fBoards.clear();
   fVMEOptions.clear();
   fFileDef.WritePath=""; fFileDef.WriteFormat=fFileDef.SplitMode=-1;
   fFileDef.ChunkLength=fFileDef.Overlap=fRunOptions.BLTPerBulkInsert=0;
   fRunOptions.BLTBytes=fRunOptions.WriteMode=fRunOptions.RunStart=
     fRunOptions.RunStartModule=-1;
   fMongoOptions.ZipOutput=false;
   fMongoOptions.WriteConcern=false;
   fMongoOptions.MinInsertSize=1000000;
   fMongoOptions.BlockSplitting=0;
   fProcessingThreads=1;
   fBaselineMode=1;
   fReadoutThreshold=10;
   fMongoDBName="data.test";
}

int XeDAQOptions::ProcessLine(string line, string option,int &ret)
{
   istringstream iss(line);
   vector<string> words;
   copy(istream_iterator<string>(iss),
	istream_iterator<string>(),
	back_inserter<vector<string> >(words));
   if(words.size()<2 || words[0]!=option) return -1;
   ret=XeDAQHelper::StringToInt(words[1]);
   
   return 0;
}

int XeDAQOptions::ProcessLineHex(string line,string option, int &ret)
{
   istringstream iss(line);
   vector<string> words;
   copy(istream_iterator<string>(iss),
	istream_iterator<string>(),
	back_inserter<vector<string> >(words));
   if(words.size()<2 || words[0]!=option) return -1;
   ret=XeDAQHelper::StringToHex(words[1]);
   
   return 0;
}

int XeDAQOptions::ReadParameterFile(string filename)
{
   cout<<"Enter code"<<endl;
   Reset();
   ifstream initFile;
   initFile.open(filename.c_str());
   if(!initFile){
      cout<<"init file not found."<<endl;
      return -1;
   }   
   cout<<"File opened"<<endl;
   string line;
   while(!initFile.eof())  {
//      cout<<"in initf"<<endl;
      getline(initFile,line);
      if(line[0]=='#') continue; //ignore comments
      
      //parse
      istringstream iss(line);
      vector<string> words;
      copy(istream_iterator<string>(iss),
	   istream_iterator<string>(),
	   back_inserter<vector<string> >(words));
      if(words.size()==0) continue;
//      cout<<words[0]<<endl;      
      //Now check each option
      if(words[0]=="WRITE_REGISTER")	{
	 if(words.size()<6) continue;
	 VMEOption_t temp;
	 temp.Address=(XeDAQHelper::StringToHex(words[1])&0xFFFFFFFF);
	 temp.Value=(XeDAQHelper::StringToHex(words[2]) & 0xFFFFFFFF);
	 temp.BoardID=XeDAQHelper::StringToInt(words[3]);
	 temp.CrateID=XeDAQHelper::StringToInt(words[4]);
	 temp.LinkID=XeDAQHelper::StringToInt(words[5]);
	 fVMEOptions.push_back(temp);
	 continue;
      }      
      if(words[0]=="LINK")	{
	 if(words.size()<4) continue;
	 LinkDefinition_t link;
	 link.LinkType=words[1];
	 link.LinkID=XeDAQHelper::StringToInt(words[2]);
	 link.CrateID=XeDAQHelper::StringToInt(words[3]);
	 fLinks.push_back(link);
	 continue;
      }
      if(words[0]=="BASE_ADDRESS")	{
	 if(words.size()<6) continue;
	 BoardDefinition_t board;
	 board.BoardType=words[1];
	 board.VMEAddress=(XeDAQHelper::StringToHex(words[2])&0xFFFFFFFF);
	 board.BoardID=XeDAQHelper::StringToInt(words[3]);
	 board.CrateID=XeDAQHelper::StringToInt(words[4]);
	 board.LinkID=XeDAQHelper::StringToInt(words[5]);
	 fBoards.push_back(board);
	 continue;
      }      
      if(words[0]=="FILE_DEFINITION")	{
	 cout<<"FOUND OPTIONS FILE_DEFINITION"<<endl;
	 if(words.size()<6) continue;
	 cout<<"WORDS OK"<<endl;
	 fFileDef.WriteFormat=XeDAQHelper::StringToInt(words[1]);
	 fFileDef.SplitMode=XeDAQHelper::StringToInt(words[2]);
	 fFileDef.ChunkLength=XeDAQHelper::StringToInt(words[3]);
	 fFileDef.Overlap = XeDAQHelper::StringToInt(words[4]);
	 fRunOptions.BLTPerBulkInsert = XeDAQHelper::StringToInt(words[5]);
	 cout<<"BLT Per Bulk: "<<XeDAQHelper::StringToInt(words[5])<<endl;
	 continue;
      }      
      if(words[0]=="FILE_PATH")	{
	 if(words.size()<2) continue;
	 fFileDef.WritePath=words[1];
	 continue;
      }
      if(words[0]=="WRITE_MODE")	{
	 cout<<"INWRITEMODE"<<endl;
	 if(words.size()<2) continue;
	 fRunOptions.WriteMode=XeDAQHelper::StringToInt(words[1]);
	 cout<<endl<<"WRITEMODE:"<<fRunOptions.WriteMode<<endl;
	 continue;
      }      
      if(words[0]=="BLT_SIZE"){
	 if(words.size()<2) continue;
	 fRunOptions.BLTBytes=XeDAQHelper::StringToInt(words[1]);	 
      }      
      if(words[0]=="RUN_START")	{
	 if(words.size()<3) continue;
	 fRunOptions.RunStart=XeDAQHelper::StringToInt(words[1]);
	 fRunOptions.RunStartModule=XeDAQHelper::StringToInt(words[2]);
      }      
      if(words[0]=="MONGO_OPTIONS")	{
	 if(words.size()<5) continue;
	 (words[1]=="1")?fMongoOptions.ZipOutput=true:fMongoOptions.ZipOutput=false;
	 fMongoOptions.MinInsertSize = XeDAQHelper::StringToInt(words[2]);
	 (words[3]=="1")?fMongoOptions.WriteConcern=true:fMongoOptions.WriteConcern=false;
	 fMongoOptions.BlockSplitting=XeDAQHelper::StringToInt(words[4]);
      }
      if(words[0]=="PROCESSING_THREADS")	{
	 if(words.size()<2) continue;
	 fProcessingThreads=XeDAQHelper::StringToInt(words[1]);
	 if(fProcessingThreads<=0) fProcessingThreads=1;
      }
      if(words[0]=="BASELINE_MODE")	{ //0 - off  1 - auto
	 if(words.size()<2) continue;
	 fBaselineMode = XeDAQHelper::StringToInt(words[1]);
	 if(fBaselineMode<0 || fBaselineMode>1) fBaselineMode=1;
      }      
      if(words[0]=="READOUT_THRESHOLD")	{
	 if(words.size()<2) continue;
	 fReadoutThreshold=XeDAQHelper::StringToInt(words[1]);
      }
      if(words[0]=="DB_NAME")	{
	 if(words.size()<2) continue;
	 fMongoDBName=words[1];
      }
      
   }   
   return 0;   
} 

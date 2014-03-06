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
   fRunOptions.BLTPerBulkInsert=0;
   fRunOptions.BLTBytes=fRunOptions.WriteMode=fRunOptions.RunStart=
     fRunOptions.RunStartModule=-1;
   fMongoOptions.ZipOutput=false;
   fMongoOptions.WriteConcern=false;
   fMongoOptions.MinInsertSize=1000000;
   fMongoOptions.BlockSplitting=0;
   fProcessingThreads=1;
   fBaselineMode=1;
   fReadoutThreshold=10;
   fMongoOptions.Collection="data.test";   
   fRunModeID="";
   vSumModules.clear();
   
   fDDC10Options.Sign=0; fDDC10Options.IntegrationWindow=100;
   fDDC10Options.VetoDelay=200; fDDC10Options.SignalThreshold=150;
   fDDC10Options.IntegrationThreshold=20000; fDDC10Options.WidthCut=50;
   fDDC10Options.RunMode=0; fDDC10Options.Address="";
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
	 if(words.size()<7) continue;
	 fMongoOptions.DBAddress=words[1];
	 if(words[1][words[1].size()-1]=='*') fMongoOptions.DynamicRunNames=true;
	 else fMongoOptions.DynamicRunNames=false;
	 fMongoOptions.Collection=words[2];
	 (words[3]=="1")?fMongoOptions.ZipOutput=true:fMongoOptions.ZipOutput=false;
	 fMongoOptions.MinInsertSize = XeDAQHelper::StringToInt(words[4]);
	 (words[5]=="1")?fMongoOptions.WriteConcern=true:fMongoOptions.WriteConcern=false;
	 fMongoOptions.BlockSplitting=XeDAQHelper::StringToInt(words[6]);
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
      if(words[0]=="SUM_MODULE")	{
	 if(words.size()<2) continue;
	 vSumModules.push_back(XeDAQHelper::StringToInt(words[1]));
      }
      if(words[0]=="RUN_MODE")	{
	 if(words.size()<2) continue;
	 fRunModeID=words[1];
      }
      
   }   
   initFile.close();
   return 0;   
} 

int XeDAQOptions::ReadFileMaster(string filename)
{
   ifstream initFile;
   initFile.open(filename.c_str());
   if(!initFile){	
      cout<<"init file not found."<<endl;
      return -1;
   }   
   cout<<"File opened"<<endl;
   string line;
   while(!initFile.eof())    {
      getline(initFile,line);
      if(line[0]=='#') continue; //ignore comments
      //parse
      istringstream iss(line);
      vector<string> words;
      copy(istream_iterator<string>(iss),
	   istream_iterator<string>(),
	   back_inserter<vector<string> >(words));
      if(words.size()==0) continue;
      
      if(words[0] == "DDC10_OPTIONS")	{
	 if(words.size()<9) continue;
	 fDDC10Options.Address = words[1];
	 fDDC10Options.RunMode = XeDAQHelper::StringToInt(words[2]);
	 fDDC10Options.Sign    = XeDAQHelper::StringToInt(words[3]);
	 fDDC10Options.IntegrationWindow = XeDAQHelper::StringToInt(words[4]);
	 fDDC10Options.VetoDelay    = XeDAQHelper::StringToInt(words[5]);
	 fDDC10Options.SignalThreshold = XeDAQHelper::StringToInt(words[6]);
	 fDDC10Options.IntegrationThreshold    = XeDAQHelper::StringToInt(words[7]);
	 fDDC10Options.WidthCut = XeDAQHelper::StringToInt(words[8]);		 
      }
      if(words[0]=="MONGO_OPTIONS")     	{	   
	 if(words.size()<7) continue;
	 fMongoOptions.DBAddress=words[1];
	 fMongoOptions.Collection=words[2];
	 (words[2][words[2].size()-1]=='*')?fMongoOptions.DynamicRunNames=true:fMongoOptions.DynamicRunNames=false;
	 (words[3]=="1")?fMongoOptions.ZipOutput=true:fMongoOptions.ZipOutput=false;
	 fMongoOptions.MinInsertSize = XeDAQHelper::StringToInt(words[4]);
	 (words[5]=="1")?fMongoOptions.WriteConcern=true:fMongoOptions.WriteConcern=false;
	   fMongoOptions.BlockSplitting=XeDAQHelper::StringToInt(words[6]);
      }
      if(words[0]=="WRITE_MODE")        	{	 
	 cout<<"INWRITEMODE"<<endl;
	 if(words.size()<2) continue;
	 fRunOptions.WriteMode=XeDAQHelper::StringToInt(words[1]);
	 cout<<endl<<"WRITEMODE:"<<fRunOptions.WriteMode<<endl;
	 continue;
      }
      
   }//end while through file
   initFile.close();
   return 0;
}

// *************************************************************
//
// kodiaq Data Acquisition Software
//  
// File   : koOptions.cc
// Author : Daniel Coderre, LHEP, Universitaet Bern
// Date   : 27.06.2013
// Update : 27.03.2014
// 
// Brief  : Options handler for Xenon-1t DAQ software
// 
// *************************************************************

#include <iostream>

#include "koOptions.hh"

koOptions::koOptions()
{
   Reset();   
}

koOptions::~koOptions()
{
}

void koOptions::Reset()
{
   fLinks.clear();
   fBoards.clear();
   fVMEOptions.clear();
   
   //Reset run options
   fRunOptions.BLTPerBulkInsert=fRunOptions.BaselineMode=0;
   fRunOptions.BLTBytes=fRunOptions.WriteMode=fRunOptions.RunStart=
     fRunOptions.RunStartModule=-1;
   
   //Reset mongodb options
   fMongoOptions.ZipOutput=fMongoOptions.WriteConcern=
     fMongoOptions.DynamicRunNames=false;
   fMongoOptions.MinInsertSize=1000000;
   fMongoOptions.ReadoutThreshold=1;
   fMongoOptions.Collection="data.test";
   
   //Reset processing options
   fProcessingOptions.NumThreads=1;
   fProcessingOptions.Mode=0;
      
   //Reset file options
   fOutfileOptions.Path="./data.out";
   fOutfileOptions.DynamicRunNames=false;
   
   fRunModeID="";
   vSumModules.clear();
   
   //Reset ddc10 options
   fDDC10Options.Sign=0; fDDC10Options.IntegrationWindow=100;
   fDDC10Options.VetoDelay=200; fDDC10Options.SignalThreshold=150;
   fDDC10Options.IntegrationThreshold=20000; fDDC10Options.WidthCut=50;
   fDDC10Options.RunMode=0; fDDC10Options.Address="";
}

int koOptions::ProcessLine(string line, string option,int &ret)
{
   istringstream iss(line);
   vector<string> words;
   copy(istream_iterator<string>(iss),
	istream_iterator<string>(),
	back_inserter<vector<string> >(words));
   if(words.size()<2 || words[0]!=option) return -1;
   ret=koHelper::StringToInt(words[1]);
   
   return 0;
}

int koOptions::ProcessLineHex(string line,string option, int &ret)
{
   istringstream iss(line);
   vector<string> words;
   copy(istream_iterator<string>(iss),
	istream_iterator<string>(),
	back_inserter<vector<string> >(words));
   if(words.size()<2 || words[0]!=option) return -1;
   ret=koHelper::StringToHex(words[1]);
   
   return 0;
}

int koOptions::ReadParameterFile(string filename)
{
   Reset();
   ifstream initFile;
   initFile.open(filename.c_str());
   if(!initFile){
      cout<<"init file not found."<<endl;
      return -1;
   }   
   string line;
   while(!initFile.eof())  {
      getline(initFile,line);
      if(line[0]=='#') continue; //ignore comments
      
      //parse
      istringstream iss(line);
      vector<string> words;
      copy(istream_iterator<string>(iss),
	   istream_iterator<string>(),
	   back_inserter<vector<string> >(words));
      if(words.size()==0) continue;

      if(words[0]=="WRITE_REGISTER")	{
	 if(words.size()<6) continue;
	 VMEOption_t temp;
	 temp.Address=(koHelper::StringToHex(words[1])&0xFFFFFFFF);
	 temp.Value=(koHelper::StringToHex(words[2]) & 0xFFFFFFFF);
	 temp.BoardID=koHelper::StringToInt(words[3]);
	 temp.CrateID=koHelper::StringToInt(words[4]);
	 temp.LinkID=koHelper::StringToInt(words[5]);
	 fVMEOptions.push_back(temp);
	 continue;
      }      
      if(words[0]=="LINK")	{
	 if(words.size()<4) continue;
	 LinkDefinition_t link;
	 link.LinkType=words[1];
	 link.LinkID=koHelper::StringToInt(words[2]);
	 link.CrateID=koHelper::StringToInt(words[3]);
	 fLinks.push_back(link);
	 continue;
      }
      if(words[0]=="BASE_ADDRESS")	{
	 if(words.size()<6) continue;
	 BoardDefinition_t board;
	 board.BoardType=words[1];
	 board.VMEAddress=(koHelper::StringToHex(words[2])&0xFFFFFFFF);
	 board.BoardID=koHelper::StringToInt(words[3]);
	 board.CrateID=koHelper::StringToInt(words[4]);
	 board.LinkID=koHelper::StringToInt(words[5]);
	 fBoards.push_back(board);
	 continue;
      }      
      if(words[0]=="WRITE_MODE")	{
	 if(words.size()<2) continue;
	 fRunOptions.WriteMode=koHelper::StringToInt(words[1]);
	 continue;
      }      
      if(words[0]=="BLT_SIZE"){
	 if(words.size()<2) continue;
	 fRunOptions.BLTBytes=koHelper::StringToInt(words[1]);	 
      }      
      if(words[0]=="RUN_START")	{
	 if(words.size()<3) continue;
	 fRunOptions.RunStart=koHelper::StringToInt(words[1]);
	 fRunOptions.RunStartModule=koHelper::StringToInt(words[2]);
      }      
      if(words[0]=="MONGO_OPTIONS")	{
	 if(words.size()<7) continue;
	 fMongoOptions.DBAddress=words[1];
	 if(words[2][words[2].size()-1]=='*') fMongoOptions.DynamicRunNames=true;
	 else fMongoOptions.DynamicRunNames=false;
	 fMongoOptions.Collection=words[2];
	 (words[3]=="1")?fMongoOptions.ZipOutput=true:fMongoOptions.ZipOutput=false;
	 fMongoOptions.MinInsertSize = koHelper::StringToInt(words[4]);
	 (words[5]=="1")?fMongoOptions.WriteConcern=true:fMongoOptions.WriteConcern=false;
	 fMongoOptions.ReadoutThreshold=koHelper::StringToInt(words[6]);
      }
      if(words[0]=="PROCESSING_OPTIONS")	{
	 if(words.size()<3) continue;
	 fProcessingOptions.NumThreads=koHelper::StringToInt(words[1]);
	 if(fProcessingOptions.NumThreads<=0) fProcessingOptions.NumThreads=1;
	 fProcessingOptions.Mode = koHelper::StringToInt(words[1]);
	 
      }
      if(words[0]=="OUTFILE_OPTIONS")	{
	 if(words.size()<2) continue;
	 fOutfileOptions.Path=words[1];
	 if(words[1][words[1].size()-1]=='*') fOutfileOptions.DynamicRunNames=true;
	 else fOutfileOptions.DynamicRunNames=false;
      }      
      if(words[0]=="BASELINE_MODE")	{ //0 - off  1 - auto
	 if(words.size()<2) continue;
	 fRunOptions.BaselineMode = koHelper::StringToInt(words[1]);
	 if(fRunOptions.BaselineMode<0 || fRunOptions.BaselineMode>1) 
	   fRunOptions.BaselineMode=1;
      }      
      if(words[0]=="SUM_MODULE")	{
	 if(words.size()<2) continue;
	 vSumModules.push_back(koHelper::StringToInt(words[1]));
      }
      if(words[0]=="RUN_MODE")	{
	 if(words.size()<2) continue;
	 fRunModeID=words[1];
      }
      if(words[0] == "DDC10_OPTIONS")   {	   
	 if(words.size()<9) continue;
	 fDDC10Options.Address = words[1];
	 fDDC10Options.RunMode = koHelper::StringToInt(words[2]);
	 fDDC10Options.Sign    = koHelper::StringToInt(words[3]);
	 fDDC10Options.IntegrationWindow = koHelper::StringToInt(words[4]);
	 fDDC10Options.VetoDelay    = koHelper::StringToInt(words[5]);
	 fDDC10Options.SignalThreshold = koHelper::StringToInt(words[6]);
	 fDDC10Options.IntegrationThreshold    = koHelper::StringToInt(words[7]);
	 fDDC10Options.WidthCut = koHelper::StringToInt(words[8]);
      }
      
   }   
   initFile.close();
   return 0;   
} 



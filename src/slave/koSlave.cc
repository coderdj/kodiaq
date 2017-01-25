// ****************************************************
// 
// kodiaq - DAQ Control for Xenon-1T
// 
// File     : koSlave.cc
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 28.10.2013
// 
// Brief    : Network-controlled DAQ driver
// 
// *****************************************************

#include <koLogger.hh>
#include <iostream>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <koNetClient.hh>
#include "DigiInterface.hh"
#include "koSysmon.hh"
#include <config.h>
#include "koSysmon.hh"

//If using the light version we need user input functions
//#ifdef KLITE
//#include <kbhit.hh>
//#endif

using namespace std;

int ReadIniFile(string filepath, string &SERVERADDR, int &PORT, 
		int &DATAPORT, string &NODENAME, int &ID,
		string &DB_USER, string &DB_PASSWORD, int &PROFILING, int &CORES);

#ifdef KLITE
// *******************************************************************************
// 
//           STANDALONE CODE FOR KODIAQ_LITE, SINGLE-INSTANCE DAQ
//           
// *******************************************************************************
#include "NCursesUI.hh"
#include "StandaloneToolbox.hh"

int StandaloneMain(string fOptionsPath, string comment)
{   
   koLogger fLog("log/slave.log");
   fLog.Message("Started standalone reader");
   DigiInterface *fElectronics = new DigiInterface(&fLog);
   koOptions *fDAQOptions = new koOptions();   
   StandaloneToolbox STool("mongodb://localhost:27017", "run", "runs");
   string fRunName="";
program_start:
   
   char input='a';
   //cout<<"Welcome to kodiaq lite! Press 's' to start the run, 'q' to quit."<<endl;
   //cout<<"To arm over and over again (5 seconds in between) press 'l'"<<endl;
   while(!kbhit())
     usleep(100);
   cin.get(input);

   if(input=='q') {   
     if(fRunName!=""){
       STool.UpdateRunDoc(fRunName);
       fRunName = "";
     }

     delete fElectronics;
     delete fDAQOptions;
     return 0;
   }
   else if(input=='b')  {
      if(fDAQOptions->ReadParameterFile(fOptionsPath)!=0)
	return -1;

      // Count how many times it cycles
      int n=0;

      while(input!='q' && input != 'p')	{
	if(fElectronics->Arm(fDAQOptions)==0)
	  cout<<fLog.GetTimeString()<<"Initialized. Counter = "<<n<<"."<<endl;
	else{
	  cout<<koLogger::GetTimeString()<<" Initialization failed!"<<endl;	  
	  goto program_start; // goto ftw
	}	 
	n++;
	int counter=0;
	while(!kbhit() && counter<1000)  {
	  usleep(1000);
	  counter++;
	}
	if(counter<1000)
	  cin.get(input);	        
      }      
      if(input == 'p'){
	if(fRunName!=""){
	  STool.UpdateRunDoc(fRunName);
	  fRunName = "";
	}
	goto program_start;
      }
      return -1;
   }   
   else if(input=='f'){
     cout<<koLogger::GetTimeString()<<" Firmware check not yet implemented"<<endl;
     goto program_start;
   }

   else if(input!='s')  goto program_start;
   
   //load options
   cout<<"Reading file "<<fOptionsPath<<endl;

   if(fDAQOptions->ReadParameterFile(fOptionsPath)!=0)   {	
     cout<<koLogger::GetTimeString()<<" Error opening .ini file!"<<endl;
     goto program_start; 
   }   
   //start digi interface
   cout<<koLogger::GetTimeString()<<" Initializing electronics"<<endl;

   mongo_option_t mopts = fDAQOptions->GetMongoOptions();
   string runname = mopts.collection;
   fRunName = runname;

   if(comment == "" && fDAQOptions->HasField("comment"))
     comment = fDAQOptions->GetString("comment");

   STool.InsertRunDoc(fDAQOptions, comment, runname);
   
   if(fElectronics->Arm(fDAQOptions)!=0)  {	
     cout<<koLogger::GetTimeString()<<" Error initializing electronics"<<endl;
     goto program_start;
   }   
   
   //start run
   if(fElectronics->StartRun()!=0)    {	
     cout<<koLogger::GetTimeString()<<"Error starting run"<<endl;
     goto program_start;
   }
   cout<<koLogger::GetTimeString()<<" Start of run"<<endl;

   input='a';
   time_t prevTime = koLogger::GetCurrentTime();
   while(1)  {	
      if(kbhit()) cin.get(input);
      if(input=='q') {
        cout << "Please stop the run with p." << endl;
        // Clear input
        input = 'a';
       }
      if(input=='p'){
	if(fRunName!=""){
	  STool.UpdateRunDoc(fRunName);
	  fRunName = "";
        }
	fElectronics->StopRun();
	cout<<koLogger::GetTimeString()<<" Run stopped"<<endl;
	goto program_start;
      }
      time_t currentTime = koLogger::GetCurrentTime();
      
      //updates every second
      time_t tdiff;
      if((tdiff = difftime(currentTime,prevTime))>=1.0)  {	 
	
	// Check for run time errors
	string errmess;
	if ( fElectronics->RunError(errmess) ){
	  cout<<koLogger::GetTimeString()<<" DAQ Error "<<errmess<<endl;
	  goto program_start;
	}
	prevTime=currentTime;
	unsigned int iFreq=0;
	unsigned int iRate = fElectronics->GetRate(iFreq);
	double rate = (double)iRate;
	double freq = (double)iFreq;
	rate = rate/tdiff;
	rate/=1048576;
	freq=freq/tdiff;
	
	vector <int> digis;
	vector <int> sizes;
	vector <int> counts;
	vector <string> readout_reports;
	int BufferSize = fElectronics->GetBufferOccupancy(digis, sizes, counts,
							  readout_reports);
	//theGUI.UpdateBufferSize(BufferSize);

	/*if( fDAQOptions.buffer_size_kill != -1 && 
	    BufferSize/1000000 > fDAQOptions.buffer_size_kill ){
	  theGUI.AddMessage(koLogger::GetTimeString() + "Buffer size too large!",3);
	  theGUI.UpdateRunDisplay(2, 0., 0.);
	  goto program_start;
	  }*/
	cout<<"DAQ running @ "<<rate<<" MB/s with "<<freq<<" Hz and "
	    <<digis.size()<<" digitizers."<<endl;
	cout<<"Buffer: "<<BufferSize<<" bytes."<<endl;
	cout<<"Press p to stop the run."<<endl;
	 //cout<<setprecision(2)<<"Rate: "<<rate<<"MB/s   Freq: "<<freq<<"Hz             "<<'\r';//   Averaged over "<<tdiff<<"s"<<'\r';//endl;
	 //cout.flush();
      }      
   }      
   cout<<koLogger::GetTimeString()<<" End of run."<<endl;
   delete fElectronics;
   delete fDAQOptions;
   exit(0);//return 0;
}

#endif     

// *******************************************************************************
// 
//           NETWORKED DAQ SLAVE CODE
//           
// *******************************************************************************
int main(int argc, char *argv[])
{

  string filepath = "DAQConfig.ini";
  string comment = "";
#ifdef KLITE
  // Get command line option 
  char c;
  while(1){
    static struct option long_options[] =
      {
        {"ini_file", required_argument, 0, 'i'},
	{"comment", required_argument, 0, 'c'},
	{"help", no_argument, 0, 'h'},
        {0,0,0,0}
      };    
    int option_index = 0;
    c = getopt_long(argc, argv, "hc:i:", long_options, &option_index);
    if( c == -1) break;    
    switch(c){
    case 'i':
      filepath = optarg;
      break;
    case 'c': 
      comment = optarg;
      break;
    case 'h':
      cout<<"./koSlave -i {ini_file} -c {comment} "<<c<<endl;
      exit(0);
    default:
      break;
    }
  }  
  return StandaloneMain(filepath, comment);
#endif
         
   //Set up objects
   koLogger      *koLog = new koLogger("log/slave.log");
   koNetClient    fNetworkInterface(koLog);
   
   filepath = "SlaveConfig.ini";
   string SERVERADDR = "xedaq02";
   int PORT = 2002, DATAPORT = 2003, ID = 1;
   string DB_USER="", DB_PASSWORD="";
   string NODENAME = "DAQ01";
   int PROFILING = 0;
   int CORES=8;
   if(ReadIniFile(filepath, SERVERADDR, PORT, DATAPORT, NODENAME, ID,
		  DB_USER, DB_PASSWORD, PROFILING, CORES)!=0){
     cout<<"Error reading .ini file, does it exist at src/slave/SlaveConfig.ini?"<<endl;
     return -1;
   }

   
   fNetworkInterface.Initialize(SERVERADDR,PORT,DATAPORT,ID,NODENAME); 

   DigiInterface  *fElectronics = new DigiInterface(koLog, ID, DB_USER, 
						    DB_PASSWORD, CORES, PROFILING);
   koOptions   *fDAQOptions  = new koOptions();
   koRunInfo_t    fRunInfo;
   
   string         fOptionsPath = "DAQConfig.ini";
   time_t         fPrevTime = koLogger::GetCurrentTime();
   bool           bArmed=false, bRunning=false, bConnected=false,
     bERROR=false;//, bRdy=false;
   
   //
   koLog->Message("Started koSlave module.");
   koSysmon sysmon;
   
   //try to connect. this is the idle state where the module will revert to if reset.
connection_loop:
   while(!bConnected)  {
      if(fNetworkInterface.Connect()==0) bConnected=true;      
      sleep(1);
   }
   cout<<"CONNECTED"<<endl;
   
   //While connected listen for commands and broadcast status
   while(bConnected){
      usleep(100);
      //watch for remote commands
      string command="0",sender;
      int id;      
      if(fNetworkInterface.ListenForCommand(command,id,sender)==0)	{
	cout<<"GOT COMMAND "<<command<<endl;
	if(command=="ARM")  {
	  //if(!bRdy || bRunning)
	  if(bRunning)
	    continue;	     
	  //bRdy=false;
	  bArmed=false;
	  bERROR=false;
	  //fElectronics->Close();
	  if(fNetworkInterface.ReceiveOptions(fOptionsPath)==0)  {
	    if(fDAQOptions->ReadParameterFile(fOptionsPath)!=0)	 {
	      koLog->Error("koSlave - error loading options");
	      fNetworkInterface.SlaveSendMessage("Error loading options!");
	      continue;
	    }
	    int ret;
	    if((ret=fElectronics->Arm(fDAQOptions))==0){
	      fNetworkInterface.SlaveSendMessage("Boards armed successfully.");
	      koLog->Message("Armed boards successfully");
	      bArmed=true;
	    }	       
	    else{
	      if(ret==-2)
		bERROR=true;
	      fNetworkInterface.SlaveSendMessage("Error initializing electronics!");
	      koLog->Error("koSlave - error initializing electronics.");		  
	      continue;
	    }	       
	  }
	  else   {
	    koLog->Error("koSlave - error receiving options!");
	    fNetworkInterface.SlaveSendMessage("Error receiving options!");
	    continue;
	  }	    
	 }	 
	/*if(command=="DBUPDATE") { //Change the write database. Done in case of dynamic write modes
	    string newCollection="none";	    
	    int count=0;
	    while(fNetworkInterface.ListenForCommand(newCollection,id,sender)!=0)  {
	       usleep(500);
	       count++;
	       if(count==20)
		 break;
	    }
	    if(count==20)  {
	       koLog->Error("koSlave - error receiving new mongodb collection.");
	       continue;
	    }	    
	    continue;
	    //change the write collection
	    cout<<"CHANGING COLLECTION"<<endl;
	    if(fDAQOptions->GetInt("write_mode")==WRITEMODE_MONGODB) {
	      fDAQOptions->SetString("mongo_collection", newCollection);
	      fElectronics->UpdateRecorderCollection(fDAQOptions);	    
	    }	      
	    cout<<"DONE CHANGING COLLECTION"<<endl;
	    }*/	 
	 if(command=="SLEEP")  {
	    if(bRunning) continue;
	    bArmed=false;
	    fElectronics->Close();	    
	 }	 
	 if(command=="START")  {
	   cout<<"GOT START"<<endl;
	   koLog->Message("Starting run");
	    if(!bArmed || bRunning) continue;
	    cout<<"STARTING"<<endl;
	    //sleep(10);
	    fElectronics->StartRun();
	    bRunning=true;
	    cout<<"STARTED"<<endl;
	 }
	 if(command=="STOP")  {
	   if(!bArmed || !bRunning) continue;
	   fElectronics->StopRun();
	   //fElectronics->Close();
	    bRunning=false;
	    bArmed=false;
	    //	    bRdy = false;
	 }
	 
	 
      }//end if listen for command      
      
      //Send status updates
      double tdiff;
      time_t fCurrentTime=koLogger::GetCurrentTime();
      if((tdiff=difftime(fCurrentTime,fPrevTime))>=1.0){
	 fPrevTime=fCurrentTime;
	 int status=KODAQ_IDLE;
	 if(bArmed && !bRunning) status=KODAQ_ARMED;
	 //if(bRdy && !bArmed && !bRunning) status = KODAQ_RDY;
	 if(bRunning) status=KODAQ_RUNNING;
	 if(bERROR) status=KODAQ_ERROR;
	 double rate=0.,freq=0.,nBoards=fElectronics->GetDigis();
	 unsigned int iFreq=0;	 
	 unsigned int iRate=fElectronics->GetRate(iFreq);
	 //cout<<"Got rate as "<<iRate<<endl;
	 rate=(double)iRate;
	 freq=(double)iFreq;
	 rate=rate/tdiff;
	 rate/=1048576;
	 freq=freq/tdiff;

	 vector <int> digis;
	 vector <int> sizes;
	 vector <int> counts;
	 vector <string> readout_reports;
	 int BufferSize = fElectronics->GetBufferOccupancy(digis, sizes, counts,
							   readout_reports);

	 // System monitor
	 koSysInfo_t systemInfo = sysmon.Get();	 
	 cout<<"CPU: "<<systemInfo.cpuPct<<" RAM_tot: "<<systemInfo.availableRAM<<
	   " RAM_used: "<<systemInfo.usedRAM<<endl;
	 cout<<"rate: "<<rate<<" freq: "<<freq<<" iRate: "<<iRate<<" tdiff: "
	     <<tdiff<<" status: ";

	 if(status == KODAQ_ARMED) cout<<"ARMED";
	 else if(status == KODAQ_RUNNING) cout<<"RUNNING";
	 else if(status == KODAQ_RDY) cout<<"READY";
	 else if(status == KODAQ_IDLE) cout<<"IDLE";
	 else cout<<"ERROR";
	 cout<<" Buffer: "<<BufferSize<<endl;
	 for(unsigned int digi=0; digi<sizes.size(); digi+=1)
	   cout<<digis[digi]<<": "<<sizes[digi]<<"("<<counts[digi]<<") ";	 
	 cout<<endl;

	 // Check for errors in threads
	 string err;
	 if( fElectronics->RunError( err ) ){
	   cout<<"ERROR "<<err;	   
	   koLog->Error("koSlave::main - [ ERROR ] From threads: " + err );
	   fNetworkInterface.SlaveSendMessage(err);
	   status = KODAQ_ERROR;
	   fNetworkInterface.SendStatusUpdate(status,rate,freq,nBoards, systemInfo);
	   //fElectronics->StopRun();
	   //fElectronics->Close();
	   continue;
	 }

	 if(fNetworkInterface.SendStatusUpdate(status,rate,
					       freq,nBoards, systemInfo)!=0){
	   bConnected=false;
	   koLog->Error("koSlave::main - [ FATAL ERROR ] Could not send status update. Going to idle state!");
	 }	 
      }
      
      
   }   
   //Close everything
   if(bRunning)
     fElectronics->StopRun();
   if(bArmed)
     fElectronics->Close();
   bArmed=false;
   bRunning=false;
   //bRdy=false;
   fNetworkInterface.Disconnect();
   bConnected=false;
   goto connection_loop;
   
   delete koLog;
   delete fElectronics;
   delete fDAQOptions;
   return 0;
   
   
}

int ReadIniFile(string filepath, string &SERVERADDR, int &PORT, 
		int &DATAPORT, string &NODENAME, int &ID, 
		string &DB_USER, string &DB_PASSWORD, int &PROFILING, int &CORES)
{
  ifstream inifile;
  inifile.open( filepath.c_str() );
  if( !inifile ) return -1;

  string line;
  while ( !inifile.eof() ){
    getline( inifile, line );
    if ( line[0] == '#' ) continue; //ignore comments                                

    //parse                                                                          
    istringstream iss(line);
    vector<string> words;
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string> >(words));
    if(words.size()<2) continue;

    if( words[0] == "COM_PORT" )
      PORT = koHelper::StringToInt(words[1]);
    else if ( words[0] == "DATA_PORT" )
      DATAPORT = koHelper::StringToInt(words[1]);
    else if ( words[0] == "NAME" )
      NODENAME=words[1];
    else if ( words[0] == "ID" )
      ID = koHelper::StringToInt(words[1]);
    else if ( words[0] == "SERVERADDR" )
      SERVERADDR = words[1];
    else if ( words[0] == "DB_USER" )
      DB_USER = words[1];
    else if ( words[0] == "DB_PASSWORD" )
      DB_PASSWORD = words[1];
    else if ( words[0] == "PROFILING" )
      PROFILING = koHelper::StringToInt(words[1]);
    else if ( words[0] == "CORES" )
      CORES = koHelper::StringToInt(words[1]);
  }
  inifile.close();
  return 0;
  
}

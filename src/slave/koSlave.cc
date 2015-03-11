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
#include <iomanip>
#include <koNetClient.hh>
#include "DigiInterface.hh"
#include <config.h>

//If using the light version we need user input functions
//#ifdef KLITE
//#include <kbhit.hh>
//#endif

using namespace std;

int ReadIniFile(string filepath, string &SERVERADDR, int &PORT, 
		int &DATAPORT, string &NODENAME, int &ID);

#ifdef KLITE
// *******************************************************************************
// 
//           STANDALONE CODE FOR KODIAQ_LITE, SINGLE-INSTANCE DAQ
//           
// *******************************************************************************
#include "NCursesUI.hh"

int StandaloneMain()
{   
   koLogger fLog("log/slave.log");
   fLog.Message("Started standalone reader");
   DigiInterface *fElectronics = new DigiInterface(&fLog);
   koOptions fDAQOptions;
   string fOptionsPath = "DAQConfig.ini";

   
   // Not yet ready
   NCursesUI theGUI;
   theGUI.Initialize();
   theGUI.DrawBottomBar(0);
   string mess = fLog.GetTimeString();
   mess += "Started kodiaq";
   theGUI.AddMessage(mess, 0);
   //char killit;
   //cin>>killit;
   //return 0;

   theGUI.UpdateRunDisplay(0, 0., 0.);
   theGUI.UpdateBufferSize(0);

program_start:
   theGUI.UpdateOutput("","");
   //   theGUI.UpdateRunDisplay(0, 0., 0.);
   theGUI.DrawBottomBar(0);
   //theGUI.UpdateBufferSize(0);

   char input='a';
   //cout<<"Welcome to kodiaq lite! Press 's' to start the run, 'q' to quit."<<endl;
   //cout<<"To arm over and over again (5 seconds in between) press 'l'"<<endl;
   while(!kbhit())
     usleep(100);
   cin.get(input);
   if(input=='q') {
     theGUI.Close();
     return 0;
   }
   else if(input=='b')  {
      if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)
	return -1;
      int n=0;
      theGUI.UpdateRunDisplay(3, 0., 0.);
      theGUI.DrawBottomBar(1);

      while(input!='q' && input != 'p')	{
	stringstream messstream;
	if(fElectronics->Initialize(&fDAQOptions)==0){
	  messstream<<fLog.GetTimeString()<<"Initialized. Counter = "<<n<<".";
	  theGUI.AddMessage(messstream.str(), 0);
	}
	 else{
	   mess = koLogger::GetTimeString();
	   mess+= "Initialization failed!";
	   theGUI.AddMessage( mess, 3 );
	   goto program_start; // yup, I did it
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
      if(input == 'p')
	goto program_start;
      return -1;
   }   
   else if(input=='f'){
     theGUI.AddMessage( koLogger::GetTimeString() + "Firmware check not yet implemented");
     goto program_start;
   }
   else if(input == 'i'){
     theGUI.InputBox("Input your .ini file path:", fOptionsPath);
     theGUI.DrawBottomBar(0);
     goto program_start;
   }
   else if(input!='s')  goto program_start;
   
   //load options
   mess = koLogger::GetTimeString();
   mess += "Reading file ";
   mess += fOptionsPath;
   theGUI.AddMessage( mess, 0);
   if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)   {	
     theGUI.AddMessage(koLogger::GetTimeString() +  "Error opening .ini file!", 3 );
     goto program_start; 
   }   
   //start digi interface
   mess = koLogger::GetTimeString();
   mess+="Initializing electronics";
   theGUI.AddMessage(mess,0);
   if(fElectronics->Initialize(&fDAQOptions)!=0)  {	
     theGUI.AddMessage(koLogger::GetTimeString() + "Error initializing electronics", 3);
     goto program_start;
   }   
   
   // Set output to display
   if(fDAQOptions.write_mode == 0)
     theGUI.UpdateOutput("none","");
   else if(fDAQOptions.write_mode == 1)
     theGUI.UpdateOutput("file", fDAQOptions.file_path);
   else if(fDAQOptions.write_mode == 2)
     theGUI.UpdateOutput("MongoDB:", fDAQOptions.mongo_address + ":" + fDAQOptions.mongo_database + "." + fDAQOptions.mongo_collection);

   //start run
   if(fElectronics->StartRun()!=0)    {	
     theGUI.AddMessage(koLogger::GetTimeString() + "Error starting run", 3);
     goto program_start;
   }
   mess = koLogger::GetTimeString();
   mess+= "Start of run";
   theGUI.AddMessage(mess, 1);
   theGUI.DrawBottomBar(1);

   input='a';
   time_t prevTime = koLogger::GetCurrentTime();
   while(1)  {	
      if(kbhit()) cin.get(input);
      if(input=='q') break;
      if(input=='p'){
	fElectronics->StopRun();
	theGUI.AddMessage(koLogger::GetTimeString() + "Run stopped",1);
	theGUI.DrawBottomBar(0);
	// update text display to zero
	goto program_start;
      }
      time_t currentTime = koLogger::GetCurrentTime();
      
      //updates every second
      time_t tdiff;
      if((tdiff = difftime(currentTime,prevTime))>=1.0)  {	 
	
	// Check for run time errors
	string errmess;
	if ( fElectronics->RunError(errmess) ){
	  theGUI.AddMessage(koLogger::GetTimeString() + "DAQ Error " +errmess,3);
	  theGUI.UpdateRunDisplay(2, 0., 0.);
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
	int BufferSize = fElectronics->GetBufferOccupancy(digis, sizes);
	theGUI.UpdateBufferSize(BufferSize);

	if( fDAQOptions.buffer_size_kill != -1 && 
	    BufferSize/1000000 > fDAQOptions.buffer_size_kill ){
	  theGUI.AddMessage(koLogger::GetTimeString() + "Buffer size too large!",3);
	  theGUI.UpdateRunDisplay(2, 0., 0.);
	  goto program_start;
	}
	theGUI.UpdateRunDisplay(1, rate, freq);
	
	 //cout<<setprecision(2)<<"Rate: "<<rate<<"MB/s   Freq: "<<freq<<"Hz             "<<'\r';//   Averaged over "<<tdiff<<"s"<<'\r';//endl;
	 //cout.flush();
      }      
   }      
   mess = koLogger::GetTimeString();
   mess += "End of run.";
   theGUI.AddMessage( mess, 1 );
   theGUI.Close();
   exit(0);//return 0;
}

#endif     

// *******************************************************************************
// 
//           NETWORKED DAQ SLAVE CODE
//           
// *******************************************************************************
int main()
{
#ifdef KLITE
   return StandaloneMain();
#endif
         
   //Set up objects
   koLogger      *koLog = new koLogger("log/slave.log");
   koNetClient    fNetworkInterface(koLog);
   
   string filepath = "SlaveConfig.ini";
   string SERVERADDR = "xedaq02";
   int PORT = 2002, DATAPORT = 2003, ID = 1;
   string NODENAME = "DAQ01";
   if(ReadIniFile(filepath, SERVERADDR, PORT, DATAPORT, NODENAME, ID)!=0){
     cout<<"Error reading .ini file, does it exist at src/slave/SlaveConfig.ini?"<<endl;
     return -1;
   }
   fNetworkInterface.Initialize(SERVERADDR,PORT,DATAPORT,ID,NODENAME); 

   DigiInterface  *fElectronics = new DigiInterface(koLog);
   koOptions   fDAQOptions;
   koRunInfo_t    fRunInfo;
   
   string         fOptionsPath = "DAQConfig.ini";
   time_t         fPrevTime = koLogger::GetCurrentTime();
   bool           bArmed=false,bRunning=false,bConnected=false,bERROR=false;
   //
   koLog->Message("Started koSlave module.");
   
   
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
	 if(command=="ARM")  {
	    bArmed=false;
	    bERROR=false;
	    fElectronics->Close();
	    if(fNetworkInterface.ReceiveOptions(fOptionsPath)==0)  {
	       if(fDAQOptions.ReadParameterFile(fOptionsPath)!=0)	 {
		  koLog->Error("koSlave - error loading options");
		  fNetworkInterface.SlaveSendMessage("Error loading options!");
		  continue;
	       }
	       int ret;
	       if((ret=fElectronics->Initialize(&fDAQOptions))==0){
		  fNetworkInterface.SlaveSendMessage("Boards armed successfully.");
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
	 if(command=="DBUPDATE") { //Change the write database. Done in case of dynamic write modes
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
	    //change the write collection
	    cout<<"CHANGING COLLECTION"<<endl;
	    if(fDAQOptions.write_mode==WRITEMODE_MONGODB) {
	      fDAQOptions.mongo_collection = newCollection;
	      fElectronics->UpdateRecorderCollection(&fDAQOptions);	    
	    }	      
	    cout<<"DONE CHANGING COLLECTION"<<endl;
	 }	 
	 if(command=="SLEEP")  {
	    if(bRunning) continue;
	    bArmed=false;
	    fElectronics->Close();	    
	 }	 
	 if(command=="START")  {
	   cout<<"GOT START"<<endl;
	    if(!bArmed || bRunning) continue;
	    cout<<"STARTING"<<endl;
	    fElectronics->StartRun();
	    bRunning=true;
	    cout<<"STARTED"<<endl;
	 }
	 if(command=="STOP")  {
	    if(!bRunning) continue;
	    fElectronics->StopRun();
	    bRunning=false;
	 }
	 
	 
      }//end if listen for command      
      
      //Send status
      double tdiff;
      time_t fCurrentTime=koLogger::GetCurrentTime();
      if((tdiff=difftime(fCurrentTime,fPrevTime))>=1.0){
	 fPrevTime=fCurrentTime;
	 int status=KODAQ_IDLE;
	 if(bArmed && !bRunning) status=KODAQ_ARMED;
	 if(bRunning) status=KODAQ_RUNNING;
	 if(bERROR) status=KODAQ_ERROR;
	 double rate=0.,freq=0.,nBoards=fElectronics->GetDigis();
	 unsigned int iFreq=0;	 
	 unsigned int iRate=fElectronics->GetRate(iFreq);
	 cout<<"Got rate as "<<iRate<<endl;
	 rate=(double)iRate;
	 freq=(double)iFreq;
	 rate=rate/tdiff;
	 rate/=1048576;
	 freq=freq/tdiff;
	 cout<<"rate: "<<rate<<" freq: "<<freq<<" iRate: "<<iRate<<" tdiff: "<<tdiff<<endl;
	 if(fNetworkInterface.SendStatusUpdate(status,rate,freq,nBoards)!=0){
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
   fNetworkInterface.Disconnect();
   bConnected=false;
   goto connection_loop;
   
   delete koLog;
   delete fElectronics;
   return 0;
   
   
}

int ReadIniFile(string filepath, string &SERVERADDR, int &PORT, 
		int &DATAPORT, string &NODENAME, int &ID)
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
  }
  inifile.close();
  return 0;
  
}

/*
 * Name:     NCursesUI.cc
 * Author:   Daniel Coderre, LHEP Uni Bern                                     
 * Date:     19.02.2015  
 *                                                                            
 * Brief:    NCurses interface for standalone slave deployments               
 *                                                                            
*/

#include <NCursesUI.hh>
#include <iostream>
#include <iomanip>
#include <sstream>

NCursesUI::NCursesUI(){ 
  bStandalone = true;
  mCurrentDet = "default";
  mMessID = 0;

  // Defaults
  mSTARTY = 0;
  mSTARTX = 0;
  mSTATUS_HEIGHT = 8;
  mSTATUS_WIDTH = 80;
  mOUTPUT_HEIGHT = 3;
  mOUTPUT_WIDTH = 80;
  mMESSAGE_HEIGHT = 11;
  mMESSAGE_WIDTH = 80;
  mCONTROL_HEIGHT = 1;
  mCONTROL_WIDTH = 80;
  
  mSTATUS_INDEX = mSTARTY;
  mOUTPUT_INDEX = mSTATUS_INDEX + mSTATUS_HEIGHT;
  mMESSAGE_INDEX = mOUTPUT_INDEX + mOUTPUT_HEIGHT;
  mCONTROL_INDEX = mMESSAGE_INDEX + mMESSAGE_HEIGHT;

  mMessageIndex = 0;
  mMongoOnline = false;
}

NCursesUI::~NCursesUI(){
}

void NCursesUI::Close(){
  //  delete kEmpty;
  endwin();
}

int NCursesUI::Initialize( bool standalone )
{
  initscr();
  getmaxyx(stdscr, mWINDOW_HEIGHT, mWINDOW_WIDTH);

  curs_set(0);
  start_color();

  // Define some colors
  init_pair(1, COLOR_WHITE, COLOR_BLUE );  
  init_pair(2, COLOR_RED, COLOR_BLACK );
  init_pair(3, COLOR_YELLOW, COLOR_BLACK );
  init_pair(4, COLOR_GREEN, COLOR_BLACK );
  init_pair(5, COLOR_BLACK, COLOR_WHITE );
  init_pair(6, COLOR_YELLOW, COLOR_BLUE );
  init_pair(7, COLOR_BLUE, COLOR_BLACK );
  init_pair(8, COLOR_BLUE, COLOR_WHITE );

  // set variables
  bStandalone = standalone;
  //koStatusPacket_t *kEmpty = new koStatusPacket_t;
  //koHelper::InitializeStatus( *kEmpty );
  mMessages.clear();
  mOutputModes.emplace( "all", "none" );


  // set some ncurses stuff
  noecho();
  keypad(stdscr, TRUE);
  
  mSTATUS_WIDTH = mOUTPUT_WIDTH = mMESSAGE_WIDTH = mCONTROL_WIDTH = 
    mWINDOW_WIDTH;

  STATUS_WIN = create_newwin( mSTATUS_HEIGHT, mSTATUS_WIDTH, 
			      mSTATUS_INDEX, mSTARTX );
  OUTPUT_WIN = create_newwin( mOUTPUT_HEIGHT, mOUTPUT_WIDTH, 
			      mOUTPUT_INDEX, mSTARTX );
  mMESSAGE_HEIGHT = mWINDOW_HEIGHT - ( mSTARTY + mSTATUS_HEIGHT + 
				       mOUTPUT_HEIGHT + mCONTROL_HEIGHT );
  mCONTROL_INDEX = mWINDOW_HEIGHT - 1;
  MESSAGE_WIN = create_newwin( mMESSAGE_HEIGHT, mMESSAGE_WIDTH, 
			       mMESSAGE_INDEX, mSTARTX );
  CONTROL_WIN = create_newwin( mCONTROL_HEIGHT, mCONTROL_WIDTH, 
			       mCONTROL_INDEX, mSTARTX );
  //  WindowResize();
  //  Refresh();
  return 0;
}

int NCursesUI::WindowResize() {

  getmaxyx(stdscr, mWINDOW_HEIGHT,mWINDOW_WIDTH);

  mSTATUS_WIDTH = mOUTPUT_WIDTH = mMESSAGE_WIDTH = mCONTROL_WIDTH =
    mWINDOW_WIDTH;  
  mMESSAGE_HEIGHT = mWINDOW_HEIGHT - ( mSTARTY + mSTATUS_HEIGHT +
                                       mOUTPUT_HEIGHT + mCONTROL_HEIGHT );
  if( mMESSAGE_HEIGHT < 0 ) mMESSAGE_HEIGHT = 0;
  mCONTROL_INDEX = mWINDOW_HEIGHT - 1;   

  wresize( STATUS_WIN, mSTATUS_HEIGHT, mSTATUS_WIDTH );
  wresize( OUTPUT_WIN, mOUTPUT_HEIGHT, mOUTPUT_WIDTH );    
  wresize( MESSAGE_WIN, mMESSAGE_HEIGHT, mMESSAGE_WIDTH );
  mvwin ( CONTROL_WIN, mCONTROL_INDEX, mSTARTX );
  wresize ( CONTROL_WIN, mCONTROL_HEIGHT, mCONTROL_WIDTH );  

  Refresh();
  return 0;
}

void NCursesUI::Refresh()
{
  char go;
  DrawOutputBox( );
  refresh();
  DrawMessageWin( );
  refresh();
  UpdateRunDisplay( );
  refresh();
  DrawBottomBar( );
  refresh();
  doupdate();
  refresh();
  return;
}

int NCursesUI::DrawBottomBar( )
{
  // status
  // 0- idle
  // 1- running
  string detector = mCurrentDet;

  int  status = mLatestStatus[detector]->DAQState;

  if( detector == "all" )
    status = 100;
  if( !mbConnected[mCurrentDet] && !bStandalone )
    status = 99;
  map <string, string> optionsList;

  switch(status){

  case KODAQ_IDLE:
    optionsList.emplace( "1 ", "- Start DAQ" );
    optionsList.emplace( "4 ", "- Change ini");
    optionsList.emplace( "5 ", "- Toggle Detector");
    optionsList.emplace( "7 ", "- Disconnect");
    optionsList.emplace( "8 ", "- Quit");    
    break;
  case KODAQ_RUNNING:
  case KODAQ_ARMED:
    optionsList.emplace( "2 ", "- Stop DAQ" );
    optionsList.emplace( "5 ", "- Toggle Detector" );
    break;
  case 100:
    optionsList.emplace( "1 ", "- Start All" );
    optionsList.emplace( "2 ", "- Stop All" );
    optionsList.emplace( "4 ", "- Change ini" );
    optionsList.emplace( "5 ", "- Toggle Detector" );
    optionsList.emplace( "8 ", "- Quit" );
    break;
  case 99:
    optionsList.emplace( "1 ", "- Connect" );
    optionsList.emplace( "4 ", "- Change ini" );
    optionsList.emplace( "5 ", "- Toggle Detector" );
    optionsList.emplace( "8 ", "- Quit");
    break;
  default:
    optionsList.emplace( "8 ", "- Quit" );
    break;

  }

  wclear( CONTROL_WIN );

  int index = mSTARTX;
  for ( map<string,string>::iterator it = optionsList.begin();
        it != optionsList.end(); it++ ) {
    
    wattron( CONTROL_WIN, A_BOLD );
    wattron( CONTROL_WIN, COLOR_PAIR(6) );
    mvwprintw( CONTROL_WIN, 0, index, (*it).first.c_str() );
    wattroff( CONTROL_WIN, COLOR_PAIR(6) );
    wattron( CONTROL_WIN, COLOR_PAIR(1) );
    index += (*it).first.size();
    mvwprintw( CONTROL_WIN, 0, index, (*it).second.c_str() );
    index += (*it).second.size();
    mvwprintw( CONTROL_WIN, 0, index, " ");
    index++;
    wattroff( CONTROL_WIN, COLOR_PAIR(1) );
    wattroff( CONTROL_WIN, A_BOLD );

  }
  if( mCONTROL_WIDTH - index > 0){
    string blanks(mCONTROL_WIDTH - index, ' ');
    wattron( CONTROL_WIN, COLOR_PAIR(6) );
    mvwprintw( CONTROL_WIN, 0, index, blanks.c_str());
    wattroff( CONTROL_WIN, COLOR_PAIR(6) );
  }

  
  wrefresh( CONTROL_WIN );

  return 0;
}

/*int NCursesUI::UpdateBufferSize(int size)
{
  mvprintw( 3, 50, "                              ");
  stringstream ratestring;
  ratestring<<"  Buffer: "<<size<<" bytes";
  mvprintw( 3, 50, ratestring.str().c_str() );
  refresh();
  return 0;

  }*/

int NCursesUI::AddMessage( string message, int highlight )
{
  mMessages.push_back(koLogger::GetTimeString() + message);
  mHighlights.push_back(highlight);
  mMessageIndex = mMessages.size() -1;
  if( mMessageIndex < 0 )
    mMessageIndex = 0;
  return DrawMessageWin( );
}

int NCursesUI::DrawMessageWin( )
{
  wclear( MESSAGE_WIN );
  
  wattron( MESSAGE_WIN, COLOR_PAIR(1) );
  wattron( MESSAGE_WIN, A_BOLD );
  stringstream titlestream;

  int endIndex = mMessageIndex - ( mMESSAGE_HEIGHT - 1 );
  if ( endIndex < 0 ) endIndex = 0;

  titlestream<<" Messages ("<<mMessageIndex<<" - "<<endIndex<<")";
  mvwprintw( MESSAGE_WIN, 0, 0, titlestream.str().c_str());
  string empty( mMESSAGE_WIDTH - titlestream.str().size(), ' '  );
  mvwprintw( MESSAGE_WIN, 0, titlestream.str().size(), empty.c_str() );	     
  wattroff( MESSAGE_WIN, A_BOLD );
  wattroff( MESSAGE_WIN, COLOR_PAIR(1) );

  if( mMessages.size() > 0 ){
    
    int mLine = 1;
    for ( int x = mMessageIndex; x >= endIndex; x-- ){
      // highlighting here (not yet in)
      //map<string, int>::iterator it = mMessages.begin();
      //advance( it, x );
      //string message = it->first;
      string message = mMessages[x];

      if( (int)message.size() > mMESSAGE_WIDTH - ( mSTARTX + 1 ) ) 
	message = message.substr( 0, mMESSAGE_WIDTH-( mSTARTX + 1 ) );
      mvwprintw( MESSAGE_WIN, mLine, mSTARTX + 1, message.c_str() );
      if( (int)message.size() == mMESSAGE_WIDTH - ( mSTARTX + 1 ) )
	mvwprintw( MESSAGE_WIN, mLine, mMESSAGE_WIDTH - 3, "..." );
      //end highlighting here
      
      mLine ++;
      if( mLine > mMESSAGE_INDEX + mMESSAGE_HEIGHT ) 
	break;
    }
  }
  mvwprintw( MESSAGE_WIN, mMESSAGE_HEIGHT - 1, 1, "END MESSAGES");
  wrefresh( MESSAGE_WIN );
  return 0;
}

void NCursesUI::MoveMessagesUp(){
  mMessageIndex += mMESSAGE_HEIGHT - 1;
  if( mMessageIndex > ((int)mMessages.size()-1 )){// - mMESSAGE_HEIGHT ) ) {
    //    mMessageIndex = mMessages.size() - mMESSAGE_HEIGHT;
    mMessageIndex = mMessages.size()-1;
  }
  if(mMessageIndex < 0)
    mMessageIndex = 0;
  DrawMessageWin();
  return;
}

void NCursesUI::MoveMessagesDown(){
  mMessageIndex -= mMESSAGE_HEIGHT - 1;
  if( mMessageIndex < mMESSAGE_HEIGHT - 1 )
    mMessageIndex = mMESSAGE_HEIGHT - 1;
  if( mMessageIndex > ((int)mMessages.size()-1))
    mMessageIndex = mMessages.size() - 1;
  DrawMessageWin();
  return;
}

int NCursesUI::AddOutputMode( string detector, string mode, string path )
{
  string savestring = mode + " - " + path;
  mOutputModes[detector] = savestring;
  return 0;
}
int NCursesUI::DrawOutputBox( )
{
  map<string,string>::iterator it = mOutputModes.find(mCurrentDet);
  string oMode = "none";
  if(it != mOutputModes.end())        
    oMode = it->second;
    
  wclear( OUTPUT_WIN );
  wattron( OUTPUT_WIN, A_BOLD );
  mvwprintw( OUTPUT_WIN, 1, 1, "Output mode: " );  
  mvwprintw( OUTPUT_WIN, 1, mOUTPUT_WIDTH/2, oMode.c_str() );
  mvwprintw( OUTPUT_WIN, 2, 1, "Initialization file: ");
  mvwprintw( OUTPUT_WIN, 2, mOUTPUT_WIDTH/2, mIniFiles[mCurrentDet].c_str() );
  wattroff( OUTPUT_WIN, A_BOLD );
  wrefresh( OUTPUT_WIN );
  return 0;
}

string NCursesUI::GetFillString( double rate )
{
  // Our max rate is 80 MB/s nominally
  int numSlash = (int)((16*rate)/80.);
  string retstring;
  if ( rate!= 0. )
    retstring = "|";
  else
    retstring = " ";
  for ( int x=0; x<16; x++ ){
    if ( x < numSlash ) 
      retstring += "|";
    else 
      retstring += " ";
  }
  return retstring;
}

int NCursesUI::AddStatusPacket( koStatusPacket_t *DAQStatus, string detector ){
  mLatestStatus [ detector ] = DAQStatus;
  //mLatestUpdate [ detector ] = koLogger::GetCurrentTime();
  return 0;
}
int NCursesUI::UpdateRunDisplay( )
{  
  wclear( STATUS_WIN );
  int CurrentLine = 2;
  int col         = -1;

  // Draw which detector is selected
  wattron( STATUS_WIN, A_BOLD );
  wattron( STATUS_WIN, COLOR_PAIR(1) );
  mvwprintw( STATUS_WIN, 0, 0, " Selected Detector:  ");
  wattroff( STATUS_WIN, COLOR_PAIR(1) );
  int barindex = 0;
  for( unsigned int x=0; x<mDetectors.size(); x++ ){
    string writeString = "  ";
    if( mDetectors[x] == mCurrentDet )
      wattron( STATUS_WIN, COLOR_PAIR(8) );
    writeString += mDetectors[x];
    writeString += "  ";
    mvwprintw( STATUS_WIN, 0, 22+barindex, writeString.c_str() );
    barindex += writeString.size();
    if( mDetectors[x] == mCurrentDet )
      wattroff( STATUS_WIN, COLOR_PAIR(8) );
  }
  wattroff( STATUS_WIN, A_BOLD );
  double totalRate = 0.;
  // Nested For Loops, first loop detectors
  for ( map<string,koStatusPacket_t*>::iterator det_it = mLatestStatus.begin(); 
	det_it != mLatestStatus.end(); det_it++ ) {
    if( !( mCurrentDet == "all" || mCurrentDet == (*det_it).first ) )
      continue;
    // second loop nodes within detector
    for ( unsigned int node = 0; node < (*det_it).second->Slaves.size();
	  node++ ) {
      
      if( node == 0 )
	col+=1;
      if ( node!=0 && node % 6 == 0) {
	col += 40;
	CurrentLine = 2;
      }
      // CurrentLine
      mvwprintw( STATUS_WIN, CurrentLine, col, "   [");
      string fill = GetFillString( (*det_it).second->Slaves[node].Rate );
      totalRate += (*det_it).second->Slaves[node].Rate;
      mvwprintw( STATUS_WIN, CurrentLine, col+4, fill.c_str() );
      mvwprintw( STATUS_WIN, CurrentLine, col+20, "] ");
      // Color det name to indicate status
      if( (*det_it).second->Slaves[node].status == KODAQ_RUNNING )
	wattron( STATUS_WIN, COLOR_PAIR(4) );
      mvwprintw( STATUS_WIN, CurrentLine, col+22, (*det_it).second->Slaves[node].name.c_str() );
      if( (*det_it).second->Slaves[node].status == KODAQ_RUNNING )
        wattroff( STATUS_WIN, COLOR_PAIR(4) );

      CurrentLine++;
    } // end for through nodes

  } // end for through dets

  // Display general run info
  if( col != -1 )
    col += 40;
  else
    col = 0;
  wattron( STATUS_WIN, A_BOLD );
  mvwprintw( STATUS_WIN, 2, col+1, "DAQ Network : ");
  mvwprintw( STATUS_WIN, 3, col+1, "Mongodb     : ");
  mvwprintw( STATUS_WIN, 4, col+1, "Status      : ");
  stringstream ratestream;
  ratestream<<"Total rate  : "<<setw(2)<<totalRate;
  mvwprintw( STATUS_WIN, 5, col+1, ratestream.str().c_str() );
 
  if( bStandalone ){
    mvwprintw( STATUS_WIN, 2, col+1+15, "NA" );
    mvwprintw( STATUS_WIN, 3, col+1+15, "NA" );
  }
  else if( mbConnected[mCurrentDet] && 
	   mLatestStatus[mCurrentDet]->Slaves.size() != 0 ){
    wattron( STATUS_WIN, COLOR_PAIR(4) );
    mvwprintw( STATUS_WIN, 2, col+1+15,"CONNECTED");
    wattroff( STATUS_WIN, COLOR_PAIR(4) );
  }
  else {
    mbConnected[mCurrentDet] = false;
    wattron( STATUS_WIN, COLOR_PAIR(2) );
    mvwprintw( STATUS_WIN, 2, col+1+15,"OFFLINE");
    wattroff( STATUS_WIN, COLOR_PAIR(2) );
  }
  if( !bStandalone && mMongoOnline ){
    wattron( STATUS_WIN, COLOR_PAIR(4) );
    mvwprintw( STATUS_WIN, 3, col+1+15,"ONLINE");
    wattroff( STATUS_WIN, COLOR_PAIR(4) );
  }
  else if( !bStandalone ) {
    wattron( STATUS_WIN, COLOR_PAIR(2) );
    mvwprintw( STATUS_WIN, 3, col+1+15,"OFFLINE");
    wattroff( STATUS_WIN, COLOR_PAIR(2) );
  }
  if( mLatestStatus[mCurrentDet]->DAQState == KODAQ_RUNNING ){
    wattron( STATUS_WIN, COLOR_PAIR(4) );
    mvwprintw( STATUS_WIN, 4, col+1+15,"RUNNING");
    wattroff( STATUS_WIN, COLOR_PAIR(4) );
  }
  else if( mLatestStatus[mCurrentDet]->DAQState == KODAQ_IDLE || 
	   mLatestStatus[mCurrentDet]->DAQState == KODAQ_ARMED ) {
    wattron( STATUS_WIN, COLOR_PAIR(3) );
    mvwprintw( STATUS_WIN, 4, col+1+15,"IDLE");
    wattroff( STATUS_WIN, COLOR_PAIR(3) );
  }
  else{
    wattron( STATUS_WIN, COLOR_PAIR(2) );
    mvwprintw( STATUS_WIN, 4, col+1+15,"ERROR");
    wattroff( STATUS_WIN, COLOR_PAIR(2) );
  }
  
  wattroff( STATUS_WIN, A_BOLD );
    

  
  /*// Run Display. Lines 0-8. 
  // status : 0 = idle, 1 = run, 2 = ERR
  if(status == 0){
    attron(COLOR_PAIR(2));
    attron(A_BOLD);
    mvprintw(2, 16, "IDLE     ");
    attroff(COLOR_PAIR(2));
    attroff(A_BOLD);
  }
  else if(status == 1){
    attron(COLOR_PAIR(4));
    attron(A_BOLD);
    mvprintw(2, 16, "RUNNING  ");
    attroff(COLOR_PAIR(4));
    attroff(A_BOLD);
  }
  else if(status == 2){
    attron(COLOR_PAIR(3));
    attron(A_BOLD);
    mvprintw(2, 16, "ERROR    ");
    attroff(COLOR_PAIR(3));
    attroff(A_BOLD);
  }
  else if(status == 3){
    attron(COLOR_PAIR(3));
    attron(A_BOLD);
    mvprintw(2, 16, "BASELINES");
    attroff(A_BOLD);
    attroff(COLOR_PAIR(3));
  }
  mvprintw( 2, 50, "                              ");
  stringstream ratestring;
  ratestring<<"  "<<freq<<" Hz @ "<<rate<<" MB/s       ";
  mvprintw( 2, 50, ratestring.str().c_str() );
  */
  wrefresh( STATUS_WIN );
  return 0;    
}

int NCursesUI::InputBox( string prompt, string &reply )
{
  WINDOW *local_win;
  local_win = newwin( 5, 70, 8, 5);
  //box(local_win, 0, 0);
  wbkgd(local_win,COLOR_PAIR(5));
  wattron(local_win, A_BOLD);
  mvwprintw(local_win, 1, 1, prompt.c_str());
  wattroff(local_win, A_BOLD);
  mvwprintw(local_win, 3, 2, ">");
  wrefresh(local_win);

  // Type in your input
  bool getout=false;
  string message = "";
  while(!getout)      {
    int c=wgetch(local_win);
    switch(c) { 
    case 10: getout=true;
      break;
    case KEY_BACKSPACE:
    case 127:   
      if(message.size()>0){
	message.erase(message.end()-1);
	wclear(local_win);
	wattron(local_win, A_BOLD);
	mvwprintw(local_win, 1, 1, prompt.c_str());
	wattroff(local_win, A_BOLD);
      mvwprintw(local_win, 3, 2, ">");
      mvwprintw(local_win, 3, 3, message.c_str());     
      wrefresh(local_win);
      }
      break;
    default: 
      if(message.size()<=60)
	message.push_back((char)c);
      else break;
      mvwprintw(local_win,3,3,message.c_str());
      wrefresh(local_win);
      break;
    }   
  } 
  reply = message;
  wborder(local_win, ' ', ' ', ' ',' ',' ',' ',' ',' ');
  wrefresh(local_win);
  delwin(local_win);
  // Draw .ini file              
  AddMessage("Updated .ini file");
  mvprintw(4, 16, "                                                                      ");
  mvprintw(4, 16, message.c_str());

  Refresh();
  return 0;
}

WINDOW* NCursesUI::create_newwin(int height, int width,
				 int starty, int startx)
{
  WINDOW *local_win;
  local_win = newwin(height, width, starty, startx);
  //box(local_win, 1, 1);
  /* 0, 0 gives default characters                       
   *  * for the vertical and horizontal         
   *  * lines
   */

  wrefresh(local_win);
  return local_win;
}

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
#include <sstream>

NCursesUI::NCursesUI(){ 
  bStandalone = true;
  mCurrentDet = "default";

  // Defaults
  mSTARTY = 1;
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
}

NCursesUI::~NCursesUI(){
}

void NCursesUI::Close(){
  endwin();
}

int NCursesUI::Initialize( bool standalone, 
			   koStatusPacket_t (*f)(string), 
			   vector <string> detectors )
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
  init_pair(6, COLOR_WHITE, COLOR_BLUE );
  init_pair(7, COLOR_BLUE, COLOR_BLACK );

  // set variables
  bStandalone = standalone;
  koStatusPacket_t kEmpty;
  koHelper::InitializeStatus( kEmpty );
  mLatestStatus.clear();
  mLatestUpdate.clear();
  mMessages.clear();
  for ( unsigned int x=0; x<detectors.size(); x++ ){
    mLatestStatus.emplace( detectors[x], kEmpty );
    mLatestUpdate.emplace( detectors[x], koLogger::GetCurrentTime() );
  }
  mLatestStatus.emplace( "all", kEmpty );
  mLatestUpdate.emplace( "all", koLogger::GetCurrentTime() );
  mOutputModes.emplace( "all", "none" );

  if( detectors.size() > 0 )
    mCurrentDet = detectors[0];

  // set some ncurses stuff
  noecho();
  keypad(stdscr, FALSE);
  
  mSTATUS_WIDTH = mOUTPUT_WIDTH = mMESSAGE_WIDTH = mCONTROL_WIDTH = 
    mWINDOW_WIDTH;

  STATUS_WIN = create_newwin( mSTATUS_HEIGHT, mSTATUS_WIDTH, 
			      mSTATUS_INDEX, mSTARTX );
  OUTPUT_WIN = create_newwin( mOUTPUT_HEIGHT, mOUTPUT_WIDTH, 
			      mOUTPUT_INDEX, mSTARTX );
  mMESSAGE_HEIGHT = mWINDOW_HEIGHT - ( mSTARTY + mSTATUS_HEIGHT + 
				       mOUTPUT_HEIGHT + mCONTROL_HEIGHT );
  mCONTROL_INDEX = mMESSAGE_HEIGHT + mMESSAGE_INDEX;
  MESSAGE_WIN = create_newwin( mMESSAGE_HEIGHT, mMESSAGE_WIDTH, 
			       mMESSAGE_INDEX, mSTARTX );
  CONTROL_WIN = create_newwin( mCONTROL_HEIGHT, mCONTROL_WIDTH, 
			       mCONTROL_INDEX, mSTARTX );

  Refresh();
  return 0;
}

int NCursesUI::WindowResize() {

  getmaxyx(stdscr, mWINDOW_WIDTH, mWINDOW_HEIGHT);
  mSTATUS_WIDTH = mOUTPUT_WIDTH = mMESSAGE_WIDTH = mCONTROL_WIDTH =
    mWINDOW_WIDTH;
  mMESSAGE_HEIGHT = mWINDOW_HEIGHT - ( mSTARTY + mSTATUS_HEIGHT +
                                       mOUTPUT_HEIGHT + mCONTROL_HEIGHT );
  mCONTROL_INDEX = mMESSAGE_HEIGHT + mMESSAGE_INDEX;
  

  wresize( STATUS_WIN, mSTATUS_HEIGHT, mSTATUS_WIDTH );
  wresize( OUTPUT_WIN, mOUTPUT_HEIGHT, mOUTPUT_WIDTH );    
  wresize( MESSAGE_WIN, mMESSAGE_HEIGHT, mMESSAGE_WIDTH );
  wmove ( CONTROL_WIN, mCONTROL_INDEX, mSTARTX );
  wresize ( CONTROL_WIN, mCONTROL_HEIGHT, mCONTROL_WIDTH );
  Refresh();
  return 0;
}

void NCursesUI::Refresh()
{
  DrawBottomBar( );
  DrawOutputBox( );
  DrawMessageWin( );
  UpdateRunDisplay( );
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

  int  status = mLatestStatus[detector].DAQState;
  if( detector == "all" )
    status = 100;
  map <string, string> optionsList;

  switch(status){

  case KODAQ_IDLE:
    optionsList.emplace( "F1", "Start DAQ" );
    optionsList.emplace( "F5", "Change ini");
    optionsList.emplace( "F9", "Toggle Detector");
    optionsList.emplace(" F12", "Quit");
    break;
  case KODAQ_RUNNING:
  case KODAQ_ARMED:
    optionsList.emplace( "F2", "Stop DAQ" );
    optionsList.emplace( "F9", "Toggle Detector" );
    break;
  case 100:
    optionsList.emplace( "F1", "Start All" );
    optionsList.emplace( "F2", "Stop All" );
    optionsList.emplace( "F9", "Toggle Detector" );
    optionsList.emplace( "F12", "Quit" );
    break;
  default:
    optionsList.emplace( "F12", "Quit" );
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
  mMessages.emplace( message, highlight );
  mMessageIndex = mMessages.size();
  return DrawMessageWin( );
}

int NCursesUI::DrawMessageWin( )
{
  wclear( MESSAGE_WIN );
  
  int endIndex = mMessageIndex - ( mMESSAGE_HEIGHT - 1 );
  if ( endIndex < 0 ) endIndex = 0;
  
  int mLine = 1;
  for ( int x = mMessageIndex; x > endIndex; x-- ){
    // highlighting here (not yet in)
    map<string, int>::iterator it = mMessages.begin();
    advance( it, x );
    string message = it->first;

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
  wrefresh( MESSAGE_WIN );
  return 0;
}

void NCursesUI::MoveMessagesUp(){
  mMessageIndex += mMESSAGE_HEIGHT - 1;
  if( mMessageIndex > (int)mMessages.size() ) {
    mMessageIndex = mMessages.size();
  }
  DrawMessageWin();
  return;
}

void NCursesUI::MoveMessagesDown(){
  mMessageIndex -= mMESSAGE_HEIGHT - 1;
  if( mMessageIndex < 0 )
    mMessageIndex = 0;
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
  wattroff( OUTPUT_WIN, A_BOLD );
  wrefresh( OUTPUT_WIN );
  return 0;
}

string NCursesUI::GetFillString( double rate )
{
  // Our max rate is 80 MB/s nominally
  int numSlash = (int)((17*rate)/80.);
  string retstring = "";
  for ( int x=0; x<17; x++ ){
    if ( x < numSlash ) 
      retstring += "|";
    else 
      retstring += " ";
  }
  return retstring;
}

int NCursesUI::AddStatusPacket( koStatusPacket_t DAQStatus, string detector ){
  mLatestStatus [ detector ] = DAQStatus;
  mLatestUpdate [ detector ] = koLogger::GetCurrentTime();
  return 0;
}
int NCursesUI::UpdateRunDisplay( )
{  
  wclear( STATUS_WIN );
  mvwprintw( STATUS_WIN, 0, 0, "TEST");
  int CurrentLine = 1;

  // Nested For Loops, first loop detectors
  for ( map<string,koStatusPacket_t>::iterator det_it = mLatestStatus.begin(); 
	det_it != mLatestStatus.end(); det_it++ ) {
    
    // second loop nodes within detector
    for ( unsigned int node = 0; node < (*det_it).second.Slaves.size();
	  node++ ) {
      
      if ( node > 6 ) 
	continue;
      // CurrentLine
      mvwprintw( STATUS_WIN, CurrentLine, 3, "[");
      string fill = GetFillString( (*det_it).second.Slaves[node].Rate );
      mvwprintw( STATUS_WIN, CurrentLine, 4, fill.c_str() );
      mvwprintw( STATUS_WIN, CurrentLine, 20, ("] " 
		 + (*det_it).second.Slaves[node].name).c_str() );
      CurrentLine++;
    } // end for through nodes

  } // end for through dets
  
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

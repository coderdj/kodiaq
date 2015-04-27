#ifndef _NCURSESUI_HH_
#define _NCURSESUI_HH_
/*
 *
 * Name:     NCursesUI.hh
 * Author:   Daniel Coderre, LHEP Uni Bern
 * Date:     19.02.2015
 *
 * Brief:    General NCurses UI
 *
*/

#include <ncurses.h>
#include <menu.h>
#include <string>
#include <vector>
#include <map>
#include <kbhit.hh>
#include "koHelper.hh" 

using namespace std;


class NCursesUI
{
public:
  
  NCursesUI();
  virtual ~NCursesUI();

  // Initialize draws the main menu
  // if standalone is true draw for standalone mode. 
  // otherwise master mode.
  int Initialize( bool standalone );

  // Add message  
  // highlight ( 0=none, 1=blue, 2=yellow, 3=red, 4=green )  
  int AddMessage( string message, int highlight=0 );

protected:
  
  // Refresh refreshes the entire screen 
  void Refresh();

  // DispBar displays a message on the bottom bar
  // Should be available commands
  int DrawBottomBar( );

  // Pops up an input box for user input. User input is put in reply
  int InputBox( string prompt, string &reply );

  // Updates display with text information
  int UpdateRunDisplay( );

  //int UpdateRunDisplay( int status, double rate, double freq );
  
  int DrawOutputBox();

  int DrawMessageWin();

  // update the output mode
  int UpdateOutput( string mode, string path );
  
  // update the memory usage in board buffers. Size in bytes
  // only for standalone deployment
  //int UpdateBufferSize( int size );

  int AddStatusPacket( koStatusPacket_t *DAQStatus, string detector );

  int AddOutputMode( string detector, string mode, string path );
  WINDOW* create_newwin(int height, int width, int starty, int startx);

  void Close();

  int WindowResize();
  void MoveMessagesUp();
  void MoveMessagesDown();

  string GetFillString( double rate );

  bool    bStandalone;
  map< string, bool >  mbConnected;
  string  mCurrentDet;
  vector <string> mDetectors;
  WINDOW *STATUS_WIN, *OUTPUT_WIN, *MESSAGE_WIN, *CONTROL_WIN;
  vector <string> mMessages;
  vector <int> mHighlights;
  map<string, koStatusPacket_t*> mLatestStatus; // latest status for each det
  map<string, string> mIniFiles;
  // needs all
  //map<string, time_t> mLatestUpdate; // latest update each det
  // needs all
  map<string, string> mOutputModes;
  // needs all

  // dimensions. Should be easy to change
  int mSTATUS_HEIGHT, mSTATUS_WIDTH, mCONTROL_HEIGHT, mCONTROL_WIDTH;
  int mOUTPUT_HEIGHT, mOUTPUT_WIDTH, mMESSAGE_HEIGHT, mMESSAGE_WIDTH;
  int mSTATUS_INDEX, mOUTPUT_INDEX, mCONTROL_INDEX, mMESSAGE_INDEX;
  int mSTARTX, mSTARTY;

  int mWINDOW_WIDTH, mWINDOW_HEIGHT; 

  // messages info
  int mMessageIndex;
  int mMessID = 0;
  bool mMongoOnline;
};

#endif
  
  

#ifndef _NCURSESUI_HH_
#define _NCURSESUI_HH_
/*
 *
 * Name:     NCursesUI.hh
 * Author:   Daniel Coderre, LHEP Uni Bern
 * Date:     19.02.2015
 *
 * Brief:    NCurses interface for standalone slave deployments
 *
*/

#include <ncurses.h>
#include <string>
#include <vector>
#include <kbhit.hh>

using namespace std;

class NCursesUI
{
public:
  
  NCursesUI();
  virtual ~NCursesUI();
  
  // Initialize draws the main menu
  int Initialize();
  
  // DispBar displays a message on the bottom bar
  // Should be available commands
  int DrawBottomBar( int status );

  // Pops up an input box for user input. User input is put in reply
  int InputBox( string prompt, string &reply );

  // Updates display with text information
  int UpdateRunDisplay( int status, double rate, double freq );
  
  // Add message
  // highlight ( 0=none, 1=blue, 2=yellow, 3=red, 4=green )
  int AddMessage( string message, int highlight=0 );

  // update the output mode
  int UpdateOutput( string mode, string path );
  
  // update the memory usage in board buffers. Size in bytes
  int UpdateBufferSize( int size );

  void Close();
  int Redraw();
private:
  WINDOW *main_win;
  vector<string> messages;
  vector<int> highlights;
};

#endif
  
  

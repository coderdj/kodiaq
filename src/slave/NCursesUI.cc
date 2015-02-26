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

NCursesUI::NCursesUI(){ }

NCursesUI::~NCursesUI(){
}

void NCursesUI::Close(){
  endwin();
}

int NCursesUI::Initialize()
{
  initscr();
  curs_set(0);
  start_color();
  init_pair(1, COLOR_WHITE, COLOR_BLUE );  
  init_pair(2, COLOR_RED, COLOR_BLACK );
  init_pair(3, COLOR_YELLOW, COLOR_BLACK );
  init_pair(4, COLOR_GREEN, COLOR_BLACK );
  init_pair(5, COLOR_BLACK, COLOR_WHITE );
  init_pair(6, COLOR_WHITE, COLOR_BLUE );
  noecho();
  keypad(stdscr, FALSE);
  main_win = newwin(24, 80, 0, 0);
  return Redraw();
}

int NCursesUI::Redraw()
{
  clear();
  // Draw main title
  attron(COLOR_PAIR(1));
  attron(A_BOLD);
  mvprintw(0, 0, " kodiaq - Data Acquisition Software                             Standalone Mode ");
  attroff(A_BOLD); 
  attroff(COLOR_PAIR(1));
  wrefresh(main_win);

  // Draw dummy rate
  attron(A_BOLD);
  attron(COLOR_PAIR(1));
  mvprintw(2, 0, "   Status: ");
  mvprintw(2, 35, "   Rate:  ");
  attroff(A_BOLD);
  attroff(COLOR_PAIR(1));
  attron(COLOR_PAIR(2));
  attron(A_BOLD);
  mvprintw(2, 16, "IDLE");
  attroff(COLOR_PAIR(2));
  attroff(A_BOLD);


  // Draw .ini file
  attron(A_BOLD);
  attron(COLOR_PAIR(1));
  mvprintw(4,0, " ini file: ");
  attroff(A_BOLD);
  attroff(COLOR_PAIR(1));
  mvprintw(4, 16, "./DAQConfig.ini");

  // Draw write path
  attron(A_BOLD);
  attron(COLOR_PAIR(1));
  mvprintw(6,0, "   Output: ");
  attroff(A_BOLD);
  attroff(COLOR_PAIR(1));

  // Draw messages display
  attron(A_BOLD);
  attron(COLOR_PAIR(1));
  mvprintw(8,0, " Messages: ");
  attroff(A_BOLD);
  attroff(COLOR_PAIR(1));
  
  refresh();

  return 0;
}

int NCursesUI::DrawBottomBar( int status )
{
  // status
  // 0- idle
  // 1- running

  attron(A_BOLD);
  attron(COLOR_PAIR(1));

  if(status == 0)
    mvprintw(23,0," Options: (S)tart (f) Get digi info (i) Change .ini (b) Baseline test    (Q)uit  ");

  else if(status == 1)
    mvprintw(23,0," Options: (p) stop run                                                   (Q)uit  ");
  attroff(A_BOLD);
  attroff(COLOR_PAIR(1));
  refresh();
  return 0;
}

int NCursesUI::UpdateBufferSize(int size)
{
  mvprintw( 3, 50, "                              ");
  stringstream ratestring;
  ratestring<<"  Buffer: "<<size<<" bytes";
  mvprintw( 3, 50, ratestring.str().c_str() );
  refresh();
  return 0;

}

int NCursesUI::AddMessage( string message, int highlight )
{
  messages.push_back( message );
  highlights.push_back( highlight );

  int startindex = 0;
  if(messages.size() > 13 ) startindex = messages.size() - 13;
  
  for ( unsigned int x=startindex; x < messages.size(); x++ ){
    // highlighting here
    if(messages[x].size() > 78 ) 
      messages[x] = messages[x].substr(0,78);
    mvprintw( (x-startindex)+10, 2, messages[x].c_str() );
    if(messages[x].size() == 78)
      mvprintw( (x-startindex)+10, 77, "..." );
    //end highlighting here
  }
  refresh();
  return 0;
}

int NCursesUI::UpdateOutput( string mode, string path )
{
  attron(A_BOLD);
  mvprintw(6, 16, mode.c_str() );
  attroff(A_BOLD);
  mvprintw(6, 26, path.c_str() );
  refresh();
  return 0;
}


int NCursesUI::UpdateRunDisplay( int status, double rate, double freq )
{  
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
  refresh();
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
  Redraw();
  AddMessage("Updated .ini file");
  mvprintw(4, 16, "                                                                      ");
  mvprintw(4, 16, message.c_str());

  wrefresh(main_win);
  return 0;
}

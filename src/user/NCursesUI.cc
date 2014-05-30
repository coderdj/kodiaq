// ****************************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : NCursesUI.cc
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.06.2013
// Update  : 28.03.2014
// 
// Brief   : Functions for creating the ncurses UI
// 
// *****************************************************************

#include <sstream>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <form.h>
#include "NCursesUI.hh"


NCursesUI::NCursesUI()
{
   bInitialized=false;
   fBlockCount=0;   
   main_win=status_win=notify_win=title_win=NULL;
   fNotificationLower=0;
   fNotificationUpper=30;
   fLog=NULL;
}

NCursesUI::~NCursesUI()
{
   endwin();
}

NCursesUI::NCursesUI(koLogger *logger)
{
   fLog=logger;
   bInitialized=false;
   fBlockCount=0;
   main_win=status_win=notify_win=title_win=NULL;
   fNotificationLower=0;
   fNotificationUpper=30;
}

int NCursesUI::GetLoginInfo(string &name, int &port, int &dataport,
			    string &hostname,string UIMessage)
{
   initscr();
   start_color();
   init_pair(1,COLOR_GREEN,COLOR_BLACK);
   init_pair(2,COLOR_RED,COLOR_BLACK);
   init_pair(3,COLOR_YELLOW,COLOR_BLACK);
   init_pair(4,COLOR_WHITE,COLOR_BLACK);
   init_pair(5,COLOR_YELLOW,COLOR_BLUE);
   init_pair(6,COLOR_BLACK,COLOR_WHITE);
   init_pair(7,COLOR_WHITE,COLOR_BLUE);
   init_pair(8,COLOR_RED,COLOR_BLUE);
   init_pair(9,COLOR_GREEN,COLOR_BLUE);
   init_pair(10,COLOR_BLACK,COLOR_WHITE); 
   refresh();
   keypad(stdscr,TRUE);
   noecho();
   
   FIELD *field[5];
   FORM *my_form;
   field[0] = new_field(1,40,4,34,0,0);
   field[1] = new_field(1,40,6,34,0,0);
   field[2] = new_field(1,40,8,34,0,0);
   field[3] = new_field(1,40,12,34,0,0);
   field[4] = NULL;
   
   set_field_buffer(field[0],0,"xedaq00");
   field_opts_off(field[0],O_AUTOSKIP);   
   set_field_buffer(field[1],0,"3000");   
   field_opts_off(field[1],O_AUTOSKIP);
   set_field_buffer(field[2],0,"3001");
   field_opts_off(field[2],O_AUTOSKIP);
   field_opts_off(field[3],O_AUTOSKIP);
   set_field_type(field[1],TYPE_INTEGER,0,2000,32768);
   set_field_type(field[2],TYPE_INTEGER,0,2000,32768);
      
   my_form = new_form(field);
   int rows,cols;
   scale_form(my_form,&rows,&cols);
   WINDOW *loginwin = newwin(rows+8,cols+12,8,22);
   wbkgd(loginwin,COLOR_PAIR(7));
   wattron(loginwin,A_BOLD);
   keypad(loginwin,TRUE);
   set_form_win(my_form,loginwin);
   set_form_sub(my_form,derwin(loginwin,rows,cols,2,2));
   post_form(my_form);
   wrefresh(loginwin);
   refresh();
   wattron(loginwin,COLOR_PAIR(10));   
   mvwprintw(loginwin,0,0," kodiaq - Data Acquisition Software                              Software version 0.9 ");
   mvwprintw(loginwin,rows+7,0," Use up/down arrows to change fields and enter to accept settings.      ctl-c to quit ");
   wattroff(loginwin,COLOR_PAIR(10));
   wattron(loginwin,COLOR_PAIR(5));
   mvwprintw(loginwin,2,14,"Please enter your network options:");   
   wattroff(loginwin,COLOR_PAIR(5));
   wattron(loginwin,COLOR_PAIR(8));
   mvwprintw(loginwin,3,14,UIMessage.c_str());
   wattroff(loginwin,COLOR_PAIR(8));
   
   wattron(loginwin,COLOR_PAIR(7));      
   mvwprintw(loginwin,6,14,"Server Address:");
   mvwprintw(loginwin,8,14,"Server Port:");
   mvwprintw(loginwin,10,14,"Data Port:");
   mvwprintw(loginwin,14,14,"Your Name:");
   wmove(loginwin,14,36);
   wattroff(loginwin,COLOR_PAIR(7));
   refresh();
//   int c;
   bool getout=false; 
   stringstream converter1,converter2;
//   int cportint,cdportint;
   char choice;
//   const char *cport,*cdport;
   form_driver(my_form,REQ_PREV_FIELD);
   int posy=0,posx=0;
   while(!getout)  {	
      int c=wgetch(loginwin);
      form_driver(my_form,REQ_VALIDATION);
      string buff(field_buffer(field[0],0),strlen(field_buffer(field[0],0)));
      string buff2(field_buffer(field[1],0),strlen(field_buffer(field[1],0)));
      string buff3(field_buffer(field[2],0),strlen(field_buffer(field[2],0)));
      string buff4(field_buffer(field[3],0),strlen(field_buffer(field[3],0)));
      switch(c) {	     
       case 10: 
	 //legitimacy check
	 buff.erase( std::remove(buff.begin(), buff.end(), ' '), buff.end() );
	 buff2.erase( std::remove(buff2.begin(), buff2.end(), ' '), buff2.end() );
	 buff3.erase( std::remove(buff3.begin(), buff3.end(), ' '), buff3.end() );
	 buff4.erase( std::remove(buff4.begin(), buff4.end(), ' '), buff4.end() );
	 getyx(loginwin,posy,posx);
	 unsigned int cportint,cdportint;
	 cportint=koHelper::StringToInt(buff2); 
	 cdportint=koHelper::StringToInt(buff3);
	 if(cportint<2000 || cdportint<2000 || cportint>32768 || cdportint>32768
	    || buff=="" || buff4=="" || cportint==cdportint)  {
	    wattron(loginwin,COLOR_PAIR(8));
	    mvwprintw(loginwin,4,14,"Sorry, your input is not valid. Fix it.");
	    wattroff(loginwin,COLOR_PAIR(8));
	    wmove(loginwin,posy,posx);
	    break;
	 }	 
	 wattron(loginwin,COLOR_PAIR(9));
	 mvwprintw(loginwin,4,14,"Start program with these settings? y/n ");
	 wattroff(loginwin,COLOR_PAIR(9));
	 choice=wgetch(loginwin);
	 if(choice=='y'){	      
	    getout=true;
	    port=(int)cportint;
	    dataport=(int)cdportint;
	    name=buff4;
	    hostname=buff;
	    stringstream logmess;
	    logmess<<"Start with (curses) "<<name<<" "<<hostname<<" "<<port<<" "<<dataport;
	    fLog->Message(logmess.str());
	 }	 
	 mvwprintw(loginwin,4,14,"                                        ");
	 wmove(loginwin,posy,posx);
	 refresh();
	 break;
       case KEY_UP:
	 form_driver(my_form,REQ_PREV_FIELD);
	 form_driver(my_form,REQ_END_LINE);
	 break;
       case KEY_DOWN:
       case '\t':
	 form_driver(my_form,REQ_NEXT_FIELD);
	 form_driver(my_form,REQ_END_LINE);
	 break;
       case KEY_BACKSPACE:
       case 127:
	 form_driver(my_form,REQ_DEL_PREV);
	 break;
       default:
	 form_driver(my_form,c);
	 break;
      }
   }
   unpost_form(my_form);
   free_form(my_form);
   free_field(field[0]);
   free_field(field[1]);
   free_field(field[2]);
   free_field(field[3]);
   wattroff(loginwin,A_BOLD);
   endwin();
   return 0;          
}

int NCursesUI::Initialize(koStatusPacket_t *DAQStatus, 
			  vector <string> history, 
			  string name)
{  
   fDAQStatus=DAQStatus;
   fRunInfo=&(DAQStatus->RunInfo);
   int startx_status=95, starty_status=1, width_status=37, height_status=42;
   int startx_main=0, starty_main=1, width_main=95, height_main=9;
   int startx_title=0,starty_title=0,width_title=132,height_title=1;
   int startx_not = 0,starty_not=10, width_not=95, height_not = 33;
   initscr();
   start_color();
   refresh();
   status_win = create_newwin(height_status,width_status,starty_status,startx_status);
   main_win = create_newwin(height_main,width_main,starty_main,startx_main);
   title_win = create_newwin(height_title,width_title,starty_title,startx_title);
   notify_win = create_newwin(height_not,width_not,starty_not,startx_not);
   refresh();

   scrollok(title_win,TRUE);
   
   curs_set(0); //0 - invisible, 1- normal, 2- very visible
   keypad(stdscr,TRUE);
   noecho();

   /* //BLUE BG
   init_pair(1,COLOR_GREEN,COLOR_BLUE);
   init_pair(2,COLOR_RED,COLOR_BLUE);
   init_pair(3,COLOR_YELLOW,COLOR_BLUE);
   init_pair(4,COLOR_WHITE,COLOR_BLUE);
   init_pair(5,COLOR_YELLOW,COLOR_CYAN);
   init_pair(6,COLOR_BLACK,COLOR_WHITE);
   init_pair(7,COLOR_WHITE,COLOR_CYAN);   
   */
   //BLACK BG
   init_pair(1,COLOR_GREEN,COLOR_BLACK);
   init_pair(2,COLOR_RED,COLOR_BLACK);
   init_pair(3,COLOR_YELLOW,COLOR_BLACK);
   init_pair(4,COLOR_WHITE,COLOR_BLACK);
   init_pair(5,COLOR_YELLOW,COLOR_BLUE);
   init_pair(6,COLOR_BLACK,COLOR_WHITE);
   init_pair(7,COLOR_WHITE,COLOR_BLUE); 
   init_pair(8,COLOR_WHITE,COLOR_GREEN);
   init_pair(9,COLOR_WHITE,COLOR_YELLOW);
   init_pair(10,COLOR_WHITE,COLOR_RED);
   init_pair(11,COLOR_BLUE,COLOR_BLACK);
   init_pair(12,COLOR_BLUE,COLOR_WHITE);
   
   wbkgd(title_win,COLOR_PAIR(5));   
   wattron(title_win,A_BOLD);
   mvwprintw(title_win,0,1,"kodiaq - DAQ for XENON");
   wattron(title_win,COLOR_PAIR(7));
   mvwprintw(title_win,0,55,"Logged in as: ");
   mvwprintw(title_win,0,70,name.c_str());
   mvwprintw(title_win,0,111,"Software Version 0.9");
   wattroff(title_win,COLOR_PAIR(7));
   wattroff(title_win,A_BOLD);
   
//   DrawStartMenu();
   bInitialized=true;
   
   SidebarRefresh();
   SetHistory(history);
   PrintNotify("Program started");
   wnoutrefresh(main_win);
   wnoutrefresh(status_win);
   wnoutrefresh(title_win);
   wnoutrefresh(notify_win);
   doupdate();
   refresh();
   return 0;
}

void NCursesUI::DrawBlockMenu(int n)
{
   wclear(main_win);
//   box(main_win, 0 , 0);
   wbkgd(main_win,COLOR_PAIR(4));
   
   string dot = " .";
   string message="Waiting for network response";
   if(n==0) fBlockCount=0;
   else fBlockCount++;
   if(fBlockCount>5) fBlockCount=0;
   int t=fBlockCount;
   while(t>0) { message+=dot; t--;}
   wattron(main_win,A_BOLD);
   mvwprintw(main_win,3,20,message.c_str());
   wattroff(main_win,A_BOLD);
   wnoutrefresh(main_win);
   Update();
   return;
}

int NCursesUI::DrawMainMenu(MainMenu_t config, bool drawBox)
{
   if(!bInitialized) return -1;
   wbkgd(main_win,COLOR_PAIR(4));
   wclear(main_win);
   
   if(drawBox)
     box(main_win, 0 , 0);   
   
   //Draw window title
   wattron(main_win,A_BOLD);   
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,1,0,config.TitleString.c_str());
   wattroff(main_win,COLOR_PAIR(6));
   wattroff(main_win,A_BOLD);
   for(unsigned int x=0;x<config.MenuItemStrings.size();x++)  {
      wattron(main_win, A_BOLD);
      if(x%2==0) wattron(main_win,COLOR_PAIR(7));
      else wattron(main_win,COLOR_PAIR(6));
      string print = " ";
      print+=config.MenuItemIDs[x];
      print+=" ";
      mvwprintw(main_win,x+1,30,print.c_str());
      if(x%2==0) wattroff(main_win,COLOR_PAIR(7));
      else wattroff(main_win,COLOR_PAIR(6));
      wattroff(main_win,A_BOLD);
      string item = config.MenuItemStrings[x];
      while(item.size()<35)
	item+=" ";
      mvwprintw(main_win,x+1,35,item.c_str());
   }
   wnoutrefresh(main_win);
   return 0;
}


int NCursesUI::NotificationsScroll(bool up)
{
   unsigned int stepsize=5;
   if(!bInitialized) return -1;
   if(up){	
      if(fNotifications.size()>fNotificationUpper && 
	 fNotifications.size()-fNotificationUpper>stepsize &&
	 fNotifications.size()-fNotificationUpper>0){
	 fNotificationUpper+=stepsize;
	 fNotificationLower+=stepsize;
      }      
      else if(fNotifications.size()>=30){
	 fNotificationUpper=fNotifications.size();
	 fNotificationLower=fNotifications.size()-30;
      }
      else {
	 fNotificationLower=0;
	 fNotificationUpper=30;
      }      
   }
   else  {
      if(fNotificationLower>=stepsize)	{
	 fNotificationLower-=stepsize;
	 fNotificationUpper-=stepsize;
      }
      else	{
	 fNotificationLower=0;
	 fNotificationUpper=30;
      }            
   }   
   DrawNotifications(fNotificationLower,fNotificationUpper);
   return 0;
}

void NCursesUI::SetHistory(vector<string> history)
{   
   string space=" ";
   for(unsigned int x=0;x<history.size();x++){	
      string message;
      int id;
      id=koHelper::StringToInt(history[x].substr(0,1));
      message=history[x];
      message.erase(0,2);
      PrintNotify(message,id,false,true);      
   }
   if(fNotifications.size()!=0 && fNotificationPriorities.size()!=0){	
      fNotifications.pop_back();
      fNotificationPriorities.pop_back();
   }   
   return;   
}

int NCursesUI::PrintNotify(string message, int priority, bool print, bool hasdate)
{
   if(!bInitialized) return -1;
   wclear(notify_win);
//   box(notify_win, 0 , 0);
   wbkgd(notify_win,COLOR_PAIR(4));
   string fullmessage=message;
   if(!hasdate){	
      fullmessage = koLogger::GetTimeString();
      fullmessage+=message;
   }
   
   string space=" ";
   if(fullmessage.size()>90)  { //maxlength=90. header=25     
      string recmessage=fullmessage.substr(0,90);
      fullmessage.erase(0,90);
      fNotifications.push_back(recmessage);
      fNotificationPriorities.push_back(priority);
      string buffer="                        ";
      do {
	 recmessage=buffer;
	 if(fullmessage.size()<65) {
	    recmessage+=fullmessage;
	    fullmessage.erase();
	    while(recmessage.size()<90) recmessage+=space;
	 }	 
	 else  {
	    recmessage+=fullmessage.substr(0,65);
	    fullmessage.erase(0,65);
	 }
	 fNotifications.push_back(recmessage);
	 fNotificationPriorities.push_back(priority);	 
      }while(fullmessage.size()>0);      
   }
   else {
      while(fullmessage.size()<90) fullmessage+=space;
      fNotificationPriorities.push_back(priority);
      fNotifications.push_back(fullmessage);
   }

   //maximum is 30
   while(fNotifications.size()>999) {
      fNotifications.erase(fNotifications.begin());
      fNotificationPriorities.erase(fNotificationPriorities.begin());
   }
   
   fNotificationUpper=fNotifications.size();
   if(fNotifications.size()<30) fNotificationUpper=30;
   if(fNotifications.size()<30) fNotificationLower=0;
   else fNotificationLower=fNotificationUpper-30;
   if(print)
     DrawNotifications(fNotificationLower,fNotificationUpper);
   return 0;
}

void NCursesUI::DrawNotifications(unsigned int lower,unsigned int upper)
{
   if(!bInitialized) return;
   wattron(notify_win,A_BOLD);
   wattron(notify_win,COLOR_PAIR(7));
   mvwprintw(notify_win,0,0," Messages: (%3i-%3i of %3i)                                                                    ",lower,upper,fNotifications.size());
   wattroff(notify_win,COLOR_PAIR(7));
   wattroff(notify_win,A_BOLD);
   wattron(notify_win,COLOR_PAIR(7));
   mvwprintw(notify_win,1,94,"^");
   mvwprintw(notify_win,32,94,"v");
   wattroff(notify_win,COLOR_PAIR(7));
   if(lower<=0 || fNotifications.size()<=0)
     mvwprintw(notify_win,1,0,"[end of buffer. older messages available in curses log.] ");
   else mvwprintw(notify_win,1,0,"                                                          ");
   for(unsigned int x=lower;x<upper;x++){//fNotifications.size();x++){
      if(x<0 || fNotifications.size()==0) continue;

      double binSize=1.;
      if(fNotifications.size()>30) binSize = (double)(fNotifications.size()-30)/30.;
      int currentBin = 0;
      if(fNotifications.size()>=30) currentBin = (int)(((double)lower)/binSize);
      
      if(currentBin==30) currentBin--;
      //scroll bar
      if(x-lower == (unsigned int) currentBin ||
	 x-lower== (unsigned int) currentBin+1 || 
	 x-lower==(unsigned int) currentBin-1)	{
	 wattron(notify_win,COLOR_PAIR(9));
	 mvwprintw(notify_win,x-lower+2.0,94," ");
	 wattroff(notify_win,COLOR_PAIR(9));
      }
      else	{
	 wattron(notify_win,COLOR_PAIR(7));
	 mvwprintw(notify_win,x-lower+2.0,94," ");
	 wattroff(notify_win,COLOR_PAIR(7));
      }
      //end scroll bar
      
      if(x>=fNotifications.size()) continue;
      
      if(fNotificationPriorities[x]==KOMESS_WARNING){//yellow	   
	 wattron(notify_win,COLOR_PAIR(3));
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(3));
      }    
      else if(fNotificationPriorities[x]==KOMESS_ERROR){//red	   
	 wattron(notify_win,COLOR_PAIR(2));
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(2));
      }
      else if(fNotificationPriorities[x]==KOMESS_NORMAL|| 
	      fNotificationPriorities[x]==KOMESS_UPDATE)	{
	 wattron(notify_win,COLOR_PAIR(4)); //white
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(4));
      }      
      else if(fNotificationPriorities[x]==KOMESS_BROADCAST){
	 wattron(notify_win,COLOR_PAIR(11));//blue
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(11));
      }      
      else if(fNotificationPriorities[x]==KOMESS_STATE)	{
	 wattron(notify_win,COLOR_PAIR(1));//green
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(1));
      }     
      else { 
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
//	 mvwprintw(notify_win,x-lower+2,0,"%i",fNotificationPriorities[x]);
      }
      
    }//1g4w11b   
   wnoutrefresh(notify_win);
   doupdate();
   refresh();
   return;
}

int NCursesUI::SidebarRefresh()
{   
   if(!bInitialized || fDAQStatus==0) return -1;

   wclear(status_win);
   wbkgd(status_win,COLOR_PAIR(4));
   
   wattron(status_win,COLOR_PAIR(7));
   wattron(status_win,A_BOLD);
   mvwprintw(status_win,1,1,"            DAQ Status             ");
   wattroff(status_win,COLOR_PAIR(7));
   
   mvwprintw(status_win,3,2,"Network:");
   mvwprintw(status_win,4,2,"Acquisition:"); 
   wattroff(status_win,A_BOLD);
   
   wattron(status_win,A_BOLD);
   if(fDAQStatus->NetworkUp) {	
      wattron(status_win,COLOR_PAIR(1));
      mvwprintw(status_win,3,18,"Connected   ");
      wattroff(status_win,COLOR_PAIR(1));
   }      
   else   {      
      wattron(status_win,COLOR_PAIR(2));
      mvwprintw(status_win,3,18,"Disconnected");
      wattroff(status_win,COLOR_PAIR(2));
   }   
   if(fDAQStatus->DAQState==KODAQ_RUNNING)     {  
      wattron(status_win,COLOR_PAIR(1));
      mvwprintw(status_win,4,18,"Running");
      wattroff(status_win,COLOR_PAIR(1));
      
   }   
   else if(fDAQStatus->DAQState==KODAQ_ARMED)  {
      wattron(status_win,COLOR_PAIR(3));
      mvwprintw(status_win,4,18,"Armed  ");
      wattroff(status_win,COLOR_PAIR(3));
   }   
   else if(fDAQStatus->DAQState==KODAQ_MIXED) {	
      wattron(status_win,COLOR_PAIR(11));
      mvwprintw(status_win,4,18,"Mixed  ");
      wattroff(status_win,COLOR_PAIR(11));
   }                      
   else if(fDAQStatus->DAQState==KODAQ_IDLE)  {
      wattron(status_win,COLOR_PAIR(2));
      mvwprintw(status_win,4,18,"Idle   ");
      wattroff(status_win,COLOR_PAIR(2));
   }
   else if(fDAQStatus->DAQState==KODAQ_ERROR){
     wattron(status_win,COLOR_PAIR(2));
     mvwprintw(status_win,4,18,"ERROR   ");
     wattroff(status_win,COLOR_PAIR(2));
   }
   wattroff(status_win,A_BOLD);
   
   wattron(status_win,A_BOLD);   
   mvwprintw(status_win,5,2,"DAQ Mode:");
   mvwprintw(status_win,5,18,fDAQStatus->RunMode.c_str());
   wattroff(status_win,A_BOLD);
   
   wattron(status_win,A_BOLD);
   mvwprintw(status_win,7,1," Current Run:   ");
   wattron(status_win,COLOR_PAIR(11));
   mvwprintw(status_win,7,18,fRunInfo->RunNumber.c_str());
   wattroff(status_win,COLOR_PAIR(11));
   mvwprintw(status_win,8,2,"Started by:");
   wattroff(status_win,A_BOLD);
   mvwprintw(status_win,8,18,fRunInfo->StartedBy.c_str());   
   
   wattron(status_win,A_BOLD);
   wattron(status_win,COLOR_PAIR(7));
   mvwprintw(status_win,11,1,"         Slave Data Rates          ");
   wattroff(status_win,COLOR_PAIR(7));
   wattroff(status_win,A_BOLD);
   double totalRate=0.0;
   for(unsigned int x=0;x<fDAQStatus->Slaves.size();x++){      
      wattron(status_win,A_BOLD);
      //draw slave name in color to indicate status
      if(fDAQStatus->Slaves[x].status==KODAQ_IDLE) wattron(status_win,COLOR_PAIR(2));
      if(fDAQStatus->Slaves[x].status==KODAQ_MIXED) wattron(status_win,COLOR_PAIR(11));
      if(fDAQStatus->Slaves[x].status==KODAQ_ARMED) wattron(status_win,COLOR_PAIR(3));
      if(fDAQStatus->Slaves[x].status==KODAQ_RUNNING) wattron(status_win,COLOR_PAIR(1));
      mvwprintw(status_win,13+(4*x),2,(fDAQStatus->Slaves[x].name).c_str());
      if(fDAQStatus->Slaves[x].status==KODAQ_IDLE) wattroff(status_win,COLOR_PAIR(2));
      if(fDAQStatus->Slaves[x].status==KODAQ_MIXED) wattroff(status_win,COLOR_PAIR(11));
      if(fDAQStatus->Slaves[x].status==KODAQ_ARMED) wattroff(status_win,COLOR_PAIR(3));
      if(fDAQStatus->Slaves[x].status==KODAQ_RUNNING) wattroff(status_win,COLOR_PAIR(1));	
      
      wattroff(status_win,A_BOLD);
      mvwprintw(status_win,13+(4*x),2+fDAQStatus->Slaves[x].name.size()+1," ");	
      string numnodes = "Num. boards: "; 
      numnodes+=koHelper::IntToString(fDAQStatus->Slaves[x].nBoards);
      mvwprintw(status_win,13+(4*x)+1,2+fDAQStatus->Slaves[x].name.size()+6,"Rate:     %2.2f MB/s",
		fDAQStatus->Slaves[x].Rate);
      mvwprintw(status_win,13+(4*x)+2,2+fDAQStatus->Slaves[x].name.size()+6,"Boards:   %1i",
	       fDAQStatus->Slaves[x].nBoards);
      mvwprintw(status_win,13+(4*x)+3,2+fDAQStatus->Slaves[x].name.size()+6,"Freq:     %2.2f Hz",
	       fDAQStatus->Slaves[x].Freq);
      double maxrate=90.0; double maxspace=43.-(fDAQStatus->Slaves[x].name.size()+12);
      double binsize=maxrate/maxspace; int nbins = fDAQStatus->Slaves[x].Rate/binsize;
      totalRate+=fDAQStatus->Slaves[x].Rate;
      if(nbins==0 && fDAQStatus->Slaves[x].Rate!=0.) nbins=1;
      for(int y=0;y<nbins;y++)	{
	 if(y>maxspace*.85)  	   {	      
	    wattron(status_win,COLOR_PAIR(10));
	    mvwprintw(status_win,13+(4*x),2+fDAQStatus->Slaves[x].name.size()+2+y," ");
	    wattroff(status_win,COLOR_PAIR(10));
	 }       
	 else if(y>maxspace/2.)  {
	    wattron(status_win,COLOR_PAIR(7));
	    mvwprintw(status_win,13+(4*x),2+fDAQStatus->Slaves[x].name.size()+2+y," ");
	    wattroff(status_win,COLOR_PAIR(7));
	 }
	 else  {
	    wattron(status_win,COLOR_PAIR(7));
	    mvwprintw(status_win,13+(4*x),2+fDAQStatus->Slaves[x].name.size()+2+y," ");
	    wattroff(status_win,COLOR_PAIR(7));	    
	 }	 	             
      }
      mvwprintw(status_win,13+(4*x),34," ");
   }      
   mvwprintw(status_win,38,1," _________________________________");
   mvwprintw(status_win,39,2,"Total Rate:");
   mvwprintw(status_win,39,23,"%3.2f MB/s",totalRate);
   wattron(status_win,COLOR_PAIR(7));
   mvwprintw(status_win,41,1," Need help?   xe-daq@lngs.infn.it   ");
   wattroff(status_win,COLOR_PAIR(7));
   
   wnoutrefresh(status_win);   
   
   
   return 0;
}


int NCursesUI::PasswordPrompt()
{
   string password = EnterMessage("root",42,true);
   if(password[0]!='f' || koHelper::EasyPassHash(password)!=955) return -1;
   return 0;
}

string NCursesUI::EnterMessage(string user, int UID, bool pw)
{
   wclear(main_win);
   wbkgd(main_win,COLOR_PAIR(4));
   wattron(main_win,A_BOLD);
   wattron(main_win,COLOR_PAIR(7));
   if(!pw)
     mvwprintw(main_win,1,0,"     Message:    ");
   else
     mvwprintw(main_win,1,0,"     Password:   ");
   wattroff(main_win,COLOR_PAIR(7));
   wattroff(main_win,A_BOLD);
   if(pw) mvwprintw(main_win,2,21," Please authenticate yourself ");
   noecho();
   string message;
   
   bool getout=false;
   while(!getout)      {	
      int c=wgetch(main_win);
      switch(c) {	 
       case 10: getout=true;
	 break;
       case KEY_BACKSPACE:	
       case 127:	   
	 if(message.size()>0)
	   message.erase(message.end()-1);
	 if(pw) break;
	 wclear(main_win);

	 //	 box(main_win, 0 , 0);
	 wattron(main_win,A_BOLD);
	 wattron(main_win,COLOR_PAIR(7));
	 mvwprintw(main_win,1,0,"     Message:    ");
	 wattroff(main_win,COLOR_PAIR(7));
	 wattroff(main_win,A_BOLD);
	
	 if(message.size()<50)
	   mvwprintw(main_win,2,21,message.c_str());
	 else {	      
	    mvwprintw(main_win,2,21,(message.substr(0,50)).c_str());
	    if(message.size()<100) mvwprintw(main_win,3,21,(message.substr(50)).c_str());
	    else  {	       
	       mvwprintw(main_win,3,21,(message.substr(50,50)).c_str());
	       mvwprintw(main_win,4,21,(message.substr(100)).c_str());
	    }	    
	 }	 
	 break;
       default:	 
	 if(message.size()<=150)
	   message.push_back((char)c);
	 if(pw) break;
	 if(message.size()<50)
	   mvwprintw(main_win,2,21,message.c_str());
	 else {
	    mvwprintw(main_win,2,21,(message.substr(0,50)).c_str());
	    if(message.size()<100) mvwprintw(main_win,3,21,(message.substr(50)).c_str());
	    else  {
	       mvwprintw(main_win,3,21,(message.substr(50,50)).c_str());
	       mvwprintw(main_win,4,21,(message.substr(100)).c_str());	       
	    }
	    
	 }	 
	 break;
      }      
   }   
   stringstream messagess;
   if(!pw)
     messagess<<"Message from "<<user<<"("<<UID<<")"<<": "<<message;
   else 
     messagess<<message;
   return messagess.str();
   
}

string NCursesUI::DropDownMenu(vector <string> RMLabels, string title)
{   
   wclear(main_win);
//   box(main_win, 0 , 0);
   wbkgd(main_win,COLOR_PAIR(4));  
   wattron(main_win,A_BOLD);
   mvwprintw(main_win,1,1,title.c_str());
   mvwprintw(main_win,2,1,"(backspace to exit)");
   wattroff(main_win,A_BOLD);
   
   MENU *RunModeMenu;
   ITEM **MenuItems;
   keypad(main_win,TRUE);
   
   int n_choices = RMLabels.size();
   MenuItems = (ITEM**)calloc(n_choices+1,sizeof(ITEM*));
   
   for(unsigned int a=0;a<RMLabels.size();a++)  {
      char empty=' ';    
      MenuItems[a] = new_item(RMLabels[a].c_str(),&empty);      
   }     
   MenuItems[n_choices] = (ITEM*)NULL;
   RunModeMenu = new_menu((ITEM**)MenuItems);
   set_menu_win(RunModeMenu,main_win);
   set_menu_sub(RunModeMenu,derwin(main_win,n_choices+2,70,1,22));
 
   set_menu_mark(RunModeMenu,"*");
   refresh();
   int highlight=1;
   post_menu(RunModeMenu);
   wrefresh(main_win);
   int c;
   bool bExit=false;
   string rVal;
   while(!bExit)  {
      c=wgetch(main_win);
      switch(c)	{
       case KEY_DOWN:
	 if(highlight!=n_choices) highlight++;
	 menu_driver(RunModeMenu,REQ_DOWN_ITEM);
	 break;
       case KEY_UP:
	 if(highlight!=1) highlight--;
	 menu_driver(RunModeMenu,REQ_UP_ITEM);
	 break;	 
       case KEY_BACKSPACE:
       case 127:
	 rVal = "ERR";
	 return rVal;
	 break;
       case 10:       
	 string print="You chose: ";
	 print+=RMLabels[highlight-1];
	 string end=". Please press y to confirm.";
	 print+=end;
	 PrintNotify(print);
	 wrefresh(main_win); 	 char ch=getch();
	 if(ch!='y') { 
	    bExit=true; 	    
	    rVal="ERR";	      
	    break;	    
	 }	 
	 rVal = RMLabels[highlight-1];
	 bExit=true;
	 break;
      }
      wrefresh(main_win);      
   }
   unpost_menu(RunModeMenu);
   free_menu(RunModeMenu);
   for(int i=0;i<n_choices;i++)
     free_item(MenuItems[i]);
   return rVal;
}

WINDOW* NCursesUI::create_newwin(int height, int width, 
				 int starty, int startx)
{   
   WINDOW *local_win;
   local_win = newwin(height, width, starty, startx);
   wrefresh(local_win);
   return local_win;
}



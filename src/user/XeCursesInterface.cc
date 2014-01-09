#include <sstream>
#include <cstring>
#include <iterator>
#include <algorithm>
#include <form.h>
#include "XeCursesInterface.hh"


XeCursesInterface::XeCursesInterface()
{
   bInitialized=false;
   fBlockCount=0;   
   main_win=status_win=notify_win=title_win=NULL;
   fNotificationLower=0;
   fNotificationUpper=30;
}

XeCursesInterface::~XeCursesInterface()
{
   endwin();
}

XeCursesInterface::XeCursesInterface(XeDAQLogger *logger)
{
   fLog=logger;
   bInitialized=false;
   fBlockCount=0;
   main_win=status_win=notify_win=title_win=NULL;
   fNotificationLower=0;
   fNotificationUpper=30;
}

int XeCursesInterface::GetLoginInfo(string &name, int &port, int &dataport,string &hostname,string UIMessage)
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
   
   set_field_buffer(field[0],0,"xedaq02");
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
   mvwprintw(loginwin,0,0," kodiaq - DAQ for XENON                                          Software version 0.9 ");
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
   int c;
   bool getout=false; 
   stringstream converter1,converter2;
   int cportint,cdportint;
   char choice;
   const char *cport,*cdport;
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
	 cportint=XeDAQHelper::StringToInt(buff2); 
	 cdportint=XeDAQHelper::StringToInt(buff3);
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
	 //name.push_back((char)c);
//	              mvwprintw(main_win,4,21,name.c_str());
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

int XeCursesInterface::Initialize(XeStatusPacket_t *DAQStatus, XeRunInfo_t *RunInfo, vector <string> history, string name)
{  
   fDAQStatus=DAQStatus;
   fRunInfo=RunInfo;
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
   
   DrawStartMenu();
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

int XeCursesInterface::DrawStartMenu()
{
   wclear(main_win);
//   box(main_win, 0 , 0);
   wbkgd(main_win,COLOR_PAIR(4));
   wattron(main_win,COLOR_PAIR(7));
   wattron(main_win,A_BOLD);
   mvwprintw(main_win,1,0,"   Start menu:   ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,3,30," C ");
   wattroff(main_win,COLOR_PAIR(6));
   wattron(main_win,COLOR_PAIR(7));   
   mvwprintw(main_win,4,30," B ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));   
   mvwprintw(main_win,5,30," Q ");
    wattroff(main_win,COLOR_PAIR(6));   
   wattroff(main_win,A_BOLD);
   mvwprintw(main_win,3,35,"Connect                              ");
   mvwprintw(main_win,4,35,"Broadcast Message                    ");
   mvwprintw(main_win,5,35,"Quit                                 ");
   wnoutrefresh(main_win);
   return 0;
}

void XeCursesInterface::DrawBlockMenu(int n)
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

int XeCursesInterface::PrintDAQRunScreen()
{
   
   wclear(main_win);
//   box(main_win, 0 , 0);
   wbkgd(main_win,COLOR_PAIR(4));
   wattron(main_win,A_BOLD);
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,1,0,"   Runtime menu: ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,2,30," P ");
   wattroff(main_win,COLOR_PAIR(6));
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,3,30," B ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,4,30," Q "); 
   wattroff(main_win,COLOR_PAIR(6));
   wattroff(main_win,A_BOLD);
   mvwprintw(main_win,2,35,"Stop acquisition                     ");
   mvwprintw(main_win,3,35,"Broadcast message                    ");
   mvwprintw(main_win,4,35,"Sign out                             ");
   wnoutrefresh(main_win);
   return 0;
}



int XeCursesInterface::DrawMainMenu()
{
   if(!bInitialized) return -1;
   wbkgd(main_win,COLOR_PAIR(4));
   
   wclear(main_win);
   wattron(main_win,A_BOLD);
//   box(main_win, 0 , 0);
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,1,0,"   Main menu:    ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,1,30," S ");
   wattroff(main_win,COLOR_PAIR(6));
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,2,30," M ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,3,30," X ");
   wattroff(main_win,COLOR_PAIR(6));
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,4,30," B ");
   wattroff(main_win,COLOR_PAIR(7));
   wattron(main_win,COLOR_PAIR(6));
   mvwprintw(main_win,5,30," R ");
   wattroff(main_win,COLOR_PAIR(6));
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,6,30," Q ");
   wattroff(main_win,COLOR_PAIR(7));
   wattroff(main_win,A_BOLD);
   mvwprintw(main_win,1,35,"Start DAQ                          ");
   mvwprintw(main_win,2,35,"Choose operation mode              ");
   mvwprintw(main_win,3,35,"Put DAQ to standby mode            ");
   mvwprintw(main_win,4,35,"Broadcast message                  ");
   mvwprintw(main_win,5,35,"Reset DAQ                          ");
   mvwprintw(main_win,6,35,"Quit                               ");   
   wnoutrefresh(main_win);
   return 0;
}

int XeCursesInterface::NotificationsScroll(bool up)
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

void XeCursesInterface::SetHistory(vector<string> history)
{   
   string space=" ";
   for(unsigned int x=0;x<history.size();x++){	
      string message;
      int id;
      id=XeDAQHelper::StringToInt(history[x].substr(0,1));
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

int XeCursesInterface::PrintNotify(string message, int priority, bool print, bool hasdate)
{
   if(!bInitialized) return -1;
   fLog->Message("Enter printnotify");
   wclear(notify_win);
//   box(notify_win, 0 , 0);
   wbkgd(notify_win,COLOR_PAIR(4));
   string fullmessage=message;
   if(!hasdate){	
      fullmessage = XeDAQLogger::GetTimeString();
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

void XeCursesInterface::DrawNotifications(unsigned int lower,unsigned int upper)
{
   if(!bInitialized) return;
   wattron(notify_win,A_BOLD);
   wattron(notify_win,COLOR_PAIR(7));
   mvwprintw(notify_win,0,0," Messages: (%3i-%3i of %3i)                                                                    ",lower,upper,fNotifications.size());
   wattroff(notify_win,COLOR_PAIR(7));
   wattroff(notify_win,A_BOLD);
//   if(fNotifications.size()>upper) mvwprintw(notify_win,32,0,"v");
//   else mvwprintw(notify_win,32,0," ");
   //if(lower>0 && fNotifications.size()>0) 
   wattron(notify_win,COLOR_PAIR(7));
   mvwprintw(notify_win,1,94,"^");
   mvwprintw(notify_win,32,94,"v");
   wattroff(notify_win,COLOR_PAIR(7));
   if(lower<=0 || fNotifications.size()<=0)
     mvwprintw(notify_win,1,0,"[end of buffer. older messages available in curses log.] ");
   else mvwprintw(notify_win,1,0,"                                                          ");
   for(unsigned int x=lower;x<upper;x++){//fNotifications.size();x++){
      if(x<0 || fNotifications.size()==0) continue;
      int binSize=1;
      if(fNotifications.size()>30) binSize = fNotifications.size()/30;
      int currentBin = 0;
      if(fNotifications.size()>=30) currentBin = lower/binSize;      
      if(x-lower == currentBin ||x-lower==currentBin+1 || x-lower==currentBin-1)	{
	 wattron(notify_win,COLOR_PAIR(9));
	 mvwprintw(notify_win,x-lower+2.0,94," ");
	 wattroff(notify_win,COLOR_PAIR(9));
      }
      else	{
	 wattron(notify_win,COLOR_PAIR(7));
	 mvwprintw(notify_win,x-lower+2.0,94," ");
	 wattroff(notify_win,COLOR_PAIR(7));
      }
      
      if(x>=fNotifications.size()) continue;
      
      if(fNotificationPriorities[x]==XEMESS_WARNING){//yellow	   
	 wattron(notify_win,COLOR_PAIR(3));
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(3));
      }    
      else if(fNotificationPriorities[x]==XEMESS_ERROR){//red	   
	 wattron(notify_win,COLOR_PAIR(2));
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(2));
      }
      else if(fNotificationPriorities[x]==XEMESS_NORMAL|| fNotificationPriorities[x]==XEMESS_UPDATE)	{
	 wattron(notify_win,COLOR_PAIR(4)); //white
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(4));
      }      
      else if(fNotificationPriorities[x]==4){
	 wattron(notify_win,COLOR_PAIR(11));
	 mvwprintw(notify_win,x-lower+2,0,"%s",fNotifications[x].c_str());
	 wattroff(notify_win,COLOR_PAIR(11));
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

int XeCursesInterface::SidebarRefresh()
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
   if(fDAQStatus->DAQState==XEDAQ_RUNNING)     {  
      wattron(status_win,COLOR_PAIR(1));
      mvwprintw(status_win,4,18,"Running");
      wattroff(status_win,COLOR_PAIR(1));
      
   }   
   else if(fDAQStatus->DAQState==XEDAQ_ARMED)  {
      wattron(status_win,COLOR_PAIR(3));
      mvwprintw(status_win,4,18,"Armed  ");
      wattroff(status_win,COLOR_PAIR(3));
   }   
   else if(fDAQStatus->DAQState==XEDAQ_MIXED) {	
      wattron(status_win,COLOR_PAIR(11));
      mvwprintw(status_win,4,18,"Mixed  ");
      wattroff(status_win,COLOR_PAIR(11));
   }                      
   else if(fDAQStatus->DAQState==XEDAQ_IDLE)  {
      wattron(status_win,COLOR_PAIR(2));
      mvwprintw(status_win,4,18,"Idle   ");
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
      if(fDAQStatus->Slaves[x].status==XEDAQ_IDLE) wattron(status_win,COLOR_PAIR(2));
      if(fDAQStatus->Slaves[x].status==XEDAQ_MIXED) wattron(status_win,COLOR_PAIR(11));
      if(fDAQStatus->Slaves[x].status==XEDAQ_ARMED) wattron(status_win,COLOR_PAIR(3));
      if(fDAQStatus->Slaves[x].status==XEDAQ_RUNNING) wattron(status_win,COLOR_PAIR(1));
      mvwprintw(status_win,13+(4*x),2,(fDAQStatus->Slaves[x].name).c_str());
      if(fDAQStatus->Slaves[x].status==XEDAQ_IDLE) wattroff(status_win,COLOR_PAIR(2));
      if(fDAQStatus->Slaves[x].status==XEDAQ_MIXED) wattroff(status_win,COLOR_PAIR(11));
      if(fDAQStatus->Slaves[x].status==XEDAQ_ARMED) wattroff(status_win,COLOR_PAIR(3));
      if(fDAQStatus->Slaves[x].status==XEDAQ_RUNNING) wattroff(status_win,COLOR_PAIR(1));	
      
      wattroff(status_win,A_BOLD);
      mvwprintw(status_win,13+(4*x),2+fDAQStatus->Slaves[x].name.size()+1," ");	
      string numnodes = "Num. boards: "; numnodes+=XeDAQHelper::IntToString(fDAQStatus->Slaves[x].nBoards);
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

string XeCursesInterface::EnterName()
{
   wclear(main_win);
   wbkgd(main_win,COLOR_PAIR(4));   
   wattron(main_win,A_BOLD);
   mvwprintw(main_win,4,3,"Name for run log:");
   wattroff(main_win,A_BOLD);
   string name;
   bool getout=false;
   while(!getout)  {
      int c=wgetch(main_win);
      switch(c)	{
       case 10: getout=true; 
	 break;
       case KEY_BACKSPACE:
       case 127:
	 if(name.size()>0)
	   name.erase(name.end()-1);
	 wclear(main_win);
	 wattron(main_win,A_BOLD);
	 mvwprintw(main_win,4,3,"Name for run log:");
	 wattroff(main_win,A_BOLD);
	 mvwprintw(main_win,4,21,name.c_str());	 	 
	 break;	 
       default:
	 name.push_back((char)c);
	 mvwprintw(main_win,4,21,name.c_str());
	 break;
      }           
   }
   return name;
   
}

string XeCursesInterface::BroadcastMessage(string user, int UID)
{
   wclear(main_win);
   wbkgd(main_win,COLOR_PAIR(4));
   wattron(main_win,A_BOLD);
   wattron(main_win,COLOR_PAIR(7));
   mvwprintw(main_win,1,0,"     Message:    ");
   wattroff(main_win,COLOR_PAIR(7));
   wattroff(main_win,A_BOLD);
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
	 wclear(main_win);
//	 box(main_win, 0 , 0);
	 wattron(main_win,A_BOLD);
	 wattron(main_win,COLOR_PAIR(7));
	 mvwprintw(main_win,1,0,"     Message:    ");
	 wattroff(main_win,COLOR_PAIR(7));
	 wattroff(main_win,A_BOLD);
	 if(message.size()<50)
	   mvwprintw(main_win,2,21,message.c_str());
	 else   {	      
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
	 if(message.size()<50)
	   mvwprintw(main_win,2,21,message.c_str());
	 else   {
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
   messagess<<"Message from "<<user<<"("<<UID<<")"<<": "<<message;
   return messagess.str();
   
}

string XeCursesInterface::EnterRunModeMenu(vector <string> RMLabels)
{   
   wclear(main_win);
//   box(main_win, 0 , 0);
   wbkgd(main_win,COLOR_PAIR(4));  
   wattron(main_win,A_BOLD);
   mvwprintw(main_win,2,2,"Choose run mode:");
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
   set_menu_sub(RunModeMenu,derwin(main_win,n_choices+2,40,2,20));
 
   set_menu_mark(RunModeMenu,"*");
   refresh();
   int highlight=1;
   post_menu(RunModeMenu);
   wrefresh(main_win);
   int c;
   bool bExit=false;
   string rVal;
   time_t start = XeDAQLogger::GetCurrentTime();
   while(!bExit)  {
      time_t now = XeDAQLogger::GetCurrentTime();
      if(difftime(now,start)>20.)	{
	 rVal = "TIMEOUT";
	 break;
      }
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
       case 10:       
	 string print="Picked mode ";
	 print+=RMLabels[highlight-1];
	 string end=". Please press y to confirm.";
	 print+=end;
	 PrintNotify(print);
	 wrefresh(main_win); 	 char ch=getch();
	 if(ch!='y') { 
	    bExit=true; 	    
	    rVal="None";	      
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

WINDOW* XeCursesInterface::create_newwin(int height, int width, 
					 int starty, int startx)
{   
   WINDOW *local_win;
   local_win = newwin(height, width, starty, startx);
//   box(local_win, 0 , 0);
   wrefresh(local_win);
   return local_win;
}



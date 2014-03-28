#ifndef _NCURSESUI_HH_
#define _NCURSESUI_HH_

// ****************************************************************
// 
// kodiaq Data Acquisition Software
// 
// File    : NCursesUI.hh
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.06.2013
// Update  : 28.03.2014
// 
// Brief   : Functions for creating the ncurses UI
// 
// *****************************************************************


#include <ncurses.h>
#include <menu.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <kbhit.hh>

#include <koHelper.hh>
#include <koLogger.hh>

using namespace std;

//
// Struct     : MainMenu_t
// Desciption : Holds all options to be drawn in a user menu.
//              The item IDs should be the key the user has to 
//              press to select that option.
//              
struct MainMenu_t {
   string          TitleString;
   vector <string> MenuItemStrings;
   vector <string> MenuItemIDs;
};


class NCursesUI
{
   
 public:
                NCursesUI();
   virtual     ~NCursesUI();
   explicit     NCursesUI(koLogger *logger);
   
   //
   // Name    : Initialize
   // Purpose : Initialize the UI at the beginning of the session
   // 
   int          Initialize(koStatusPacket_t *DAQStatus,koRunInfo_t *RunInfo, 
			   vector <string> history, string name);
   //
   // Name    : DrawMainMenu
   // Purpose : Draw the main menu according to the MainMenu_t argument
   // 
   int          DrawMainMenu(MainMenu_t config, bool drawBox=false);
   //
   // Name    : DrawBlockMenu
   // Purpose : Draws a "please wait" type menu with little dots to show
   //           that the DAQ is doing something. The argument defines the dots
   // 
   void         DrawBlockMenu(int n=1);
   //
   // Name    : DropDownMenu
   // Purpose : Let the user select something from a list (the selected item
   //           is returned). 
   // 
   string       DropDownMenu(vector <string> RMLabels, string title);
   //
   // Name    : EnterMessage
   // Purpose : Allow a user to enter a message. If the "pw" flag is set to
   //           true then the UI will ask for a password (and not print it
   //           to the screen)
   // 
   string       EnterMessage(string name,int UID, bool pw=false);
   //
   // Name    : PasswordPrompt
   // Purpose : Ask the user for a password and return 0 if he got it right
   //
   int          PasswordPrompt();
   //
   // Name    : SidebarRefesh
   // Purpose : Update the sidebar with the info currently in fDAQStatus   
   // 
   int          SidebarRefresh();
   //
   //
   // Messages window
   // 
   // Name    : PrintNotify
   // Purpose : Print a notification message. Priority determines highlighting.
   //           Priority should be KOMESS_X enum (see koHelper). 
   // 
   int          PrintNotify(string message,int priority=1, 
			    bool print=true, bool hasdate=false);
   //
   // Name    : NotificationsScroll
   // Purpose : Scroll the notifications window up (up==true) or down (up==false)
   // 
   int          NotificationsScroll(bool up);
   //
   // Name    : DrawNotifications
   // Purpose : Draw the notifications window with lower and upper bounds defined.
   // 
   void         DrawNotifications(unsigned int lower,unsigned int upper);
   //
   // Name    : SetHistory
   // Purpose : Set the messages buffer with a string vector
   // 
   void         SetHistory(vector<string> history);
   //
   //
   // Name    : GetLoginInfo
   // Purpose : This is the start screen that defines the connection to the master.
   //           The user can input a name as well as the connection info.
   // 
   int GetLoginInfo(string &name, int &port, int &dataport,
		    string &hostname,string UIMessage);
   //
   
   
   //
   // Name    : Update
   // Purpose : Update the display (refresh)
   // 
   void         Update()  {
                  doupdate();
   };
   //
   // Name    : Close
   // Purpose : Close the ncurse display. Should return the terminal to normal
   // 
   void         Close(){
                  endwin();
   };   
   
 private:
   bool bInitialized;
   WINDOW *main_win, *status_win, *notify_win, *title_win;
   WINDOW *create_newwin(int height, int width, int starty, int startx);
   
   vector <string> fNotifications;
   vector <int> fNotificationPriorities;
   koStatusPacket_t *fDAQStatus;
   koRunInfo_t *fRunInfo;
   int fBlockCount;
   unsigned int fNotificationLower,fNotificationUpper;
   koLogger *fLog;
};
#endif

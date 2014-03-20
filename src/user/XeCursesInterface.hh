#ifndef _XECURSESINTERFACE_HH_
#define _XECURSESINTERFACE_HH_

// ****************************************************************
// 
// DAQ Control for Xenon-1t
// 
// Author  : Daniel Coderre, LHEP, Universitaet Bern
// Date    : 24.06.2013
// File    : XeCursesInterface.hh
// 
// Brief   : Draws NCurses UI for DAQ control
// 
// *****************************************************************


#include <ncurses.h>
#include <menu.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <kbhit.hh>

#include <XeDAQHelper.hh>
#include <XeDAQLogger.hh>

using namespace std;

class XeCursesInterface
{
 public:
   XeCursesInterface();
   virtual ~XeCursesInterface();
   explicit XeCursesInterface(XeDAQLogger *logger);
   
   int Initialize(XeStatusPacket_t *DAQStatus,XeRunInfo_t *RunInfo, vector <string> history, string name);   
   int DrawStartMenu();
   int DrawMainMenu();
   int SidebarRefresh();
   
   int PrintNotify(string message,int priority=1, bool print=true, bool hasdate=false);
   int NotificationsScroll(bool up);
   void DrawNotifications(unsigned int lower,unsigned int upper);
   void SetHistory(vector<string> history);
   int GetLoginInfo(string &name, int &port, int &dataport,string &hostname,string UIMessage);
   
   int PrintDAQRunScreen();
   int DrawAdminWindow(); 
   void DrawBlockMenu(int n=1);
   
   string EnterRunModeMenu(vector <string> RMLabels, bool banner=false);
   string EnterName();
   string BroadcastMessage(string name,int UID, bool pw=false);
   int PasswordPrompt();
   
   void Update()  {
      doupdate();
   };
   void Close(){
      endwin();
   };   
 private:
   bool bInitialized;
   WINDOW *main_win, *status_win, *notify_win, *title_win;
   WINDOW *create_newwin(int height, int width, int starty, int startx);
   
//   map <string,string> fRunModes;
   vector <string> fNotifications;
   vector <int> fNotificationPriorities;
   XeStatusPacket_t *fDAQStatus;
   XeRunInfo_t *fRunInfo;
   int fBlockCount;
   unsigned int fNotificationLower,fNotificationUpper;
   XeDAQLogger *fLog;
};
#endif

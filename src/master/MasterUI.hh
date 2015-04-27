// *****************************************************
// 
// DAQ Control for XENON1T
//
// File       : MasterUI.hh
// Author     : Daniel Coderre, LHEP, Uni Bern
// Date       : 15.04.2015
//
// Brief      : Derived UI module for koMaster
//
// *****************************************************


#ifndef _MASTER_UI_HH_
#define _MASTER_UI_HH_

#include "DAQMonitor.hh"
#include "MasterMongodbConnection.hh"
#include <NCursesUI.hh>

class MasterUI : public NCursesUI {

public: 
  MasterUI();
  virtual ~MasterUI();
  int StartLoop( koLogger *LocalLog,
		 map<string, DAQMonitor*> *dMonitors,
		 map<string, koNetServer*> *dNetworks,
                 MasterMongodbConnection *MongoDB
	       );

private:
  bool MapCheckAllTrue( map<string,bool> themap);
  bool MapCheckAllFalse( map<string,bool> themap);


};

#endif

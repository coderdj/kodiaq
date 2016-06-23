#ifndef _KOSYSMON_HH_
#define _KOSYSMON_HH_

// *********************************************************
// 
// kodiaq Data Acquisition Software
// 
// Author   : Daniel Coderre, LHEP, Universitaet Bern
// Date     : 23.06.2016
// File     : koSysmon.hh
// 
// Brief    : System monitoring for kodiaq
//       Thanks to http://stackoverflow.com/a/64166    
// **********************************************************

#include <sys/types.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

using namespace std;

//
// Object   : koSysInfo_t
// Brief    : Holds the system information
// 
struct koSysInfo_t{
  double cpuPct;
  int availableRAM;
  int usedRAM;
};


class koSysmon
{
 public:
   koSysmon();
   virtual ~koSysmon();

  koSysInfo_t Get();

 private:
  unsigned long long lastTotalUser, lastTotalUserLow, lastTotalSys, lastTotalIdle;

};
#endif

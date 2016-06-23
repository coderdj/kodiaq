#include <cstring>
#include "koSysmon.hh"
#include <iostream>
koSysmon::koSysmon(){
  lastTotalUser=lastTotalUserLow=lastTotalSys=lastTotalIdle=0;
}
koSysmon::~koSysmon(){}

koSysInfo_t koSysmon::Get(){
  // again, see http://stackoverflow.com/a/64166  
  
  char *name = "";
  static unsigned long long value, tot=0, freemem=0;
  FILE* file = fopen("/proc/meminfo", "r");
  int suc = fscanf(file, "MemTotal: %llu kB\n",  &tot);                 
  suc = fscanf(file, "MemFree: %llu kB\n",  &freemem);
  suc = fscanf(file, "MemAvailable: %llu kB\n",  &freemem);
  tot/=1024;
  freemem/=1024;
  fclose(file);
  
  // Now CPU
  double usage = -1.0;  

  if(lastTotalUser == 0 || lastTotalUserLow == 0 || lastTotalSys == 0 || 
     lastTotalIdle==0){
    file = fopen("/proc/stat", "r");
    suc = fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserLow,
		 &lastTotalSys, &lastTotalIdle);
    fclose(file);
    usage=0.;
  }
  else{
    
    // Needs some time for CPU to work ahead
    // Can spare 1/100 second in main thread
    unsigned long long totalUser, totalUserLow, totalSys, totalIdle, total;    
    file = fopen("/proc/stat", "r");
    int suc = fscanf(file, "cpu %llu %llu %llu %llu", &totalUser, &totalUserLow,
	   &totalSys, &totalIdle);
    fclose(file);

    if (totalUser < lastTotalUser || totalUserLow < lastTotalUserLow ||
	totalSys < lastTotalSys || totalIdle < lastTotalIdle){
      //Overflow detection. Just skip this value.
      usage = 0.;
    }
    else{
      total = (totalUser - lastTotalUser) + (totalUserLow - lastTotalUserLow) +
	(totalSys - lastTotalSys);
      usage = total;
      total += (totalIdle - lastTotalIdle);
      usage /= total;
      usage *= 100;
    }    
    
    lastTotalUser = totalUser;
    lastTotalUserLow = totalUserLow;
    lastTotalSys = totalSys;
    lastTotalIdle = totalIdle;
  }
  
  koSysInfo_t ret;
  ret.cpuPct = usage;
  ret.availableRAM = tot;//totalPhysMem;
  ret.usedRAM = tot-freemem;//physMemUsed;
  return ret;
}


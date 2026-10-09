#include "types.h"
#ifndef _PSTAT_H_
#define _PSTAT_H_

enum procstate { UNUSED, USED, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

struct rusage{
    uint cputime;
};

struct pstat {
  int pid;              
  enum procstate state; 
  uint64 size;    
  // Parent process ID     
  int ppid;      
  // Parent command name       
  char name[16];   
  int priority;
  uint readytime;     
};

#endif

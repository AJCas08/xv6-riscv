#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("initial priority: %d\n", getpriority());

  if(setpriority(3) < 0){
    printf("setpriority failed\n");
    exit(1);
  }
  printf("after setpriority(3): %d\n", getpriority());

  int pid = fork();
  if(pid == 0){
    printf("child inherited priority: %d\n", getpriority());
    setpriority(7);
    // stay alive so ps can see us
    sleep(10);          
    exit(0);
  }
  // give the child time to set its priority
  sleep(10);             
  char *args[] = { "ps", 0 };
  if(fork() == 0){
    exec("ps", args);
    exit(1);
  }
  // ps
  wait(0);   
  // sleeping child            
  wait(0);               
  exit(0);
}
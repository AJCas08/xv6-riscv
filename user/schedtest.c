#include "kernel/types.h"
#include "user/user.h"

void
spin(void)
{
  volatile int x = 0;
  for(int i = 0; i < 100000000; i++)   // tune so each child takes a few seconds
    x += i;
}

int
main(void)
{
  int prios[3] = { 20, 10, 5 };

  for(int i = 0; i < 3; i++){
    int pid = fork();
    if(pid < 0){
      fprintf(2, "fork failed\n");
      exit(1);
    }
    if(pid == 0){
      setpriority(prios[i]);
      spin();
      printf("child with priority %d finished at tick %d\n",
             prios[i], uptime());
      exit(0);
    }
  }
  for(int i = 0; i < 3; i++)
    wait(0);
  exit(0);
}
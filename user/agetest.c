
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NPROC_TEST 10
#define WORK       850

int fds[2];

void
spin(int n)
{
  volatile int x = 0;
  for(int j = 0; j < n; j++)
    for(int i = 0; i < 1000000; i++)
      x += i;
}

int
spawn(int prio, int work)
{
  setpriority(prio);
  uint born = uptime();

  int pid = fork();

  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(pid == 0){
    int rec[3];
    uint started = uptime();

    printf("START  PID %d | Priority %d | Tick %d\n",
           getpid(), prio, started);

    spin(work);

    uint done = uptime();

    printf("FINISH PID %d | Priority %d | Tick %d\n",
           getpid(), prio, done);

    rec[0] = prio;
    rec[1] = started - born;
    rec[2] = done - born;

    write(fds[1], rec, sizeof(rec));
    exit(0);
  }

  setpriority(30);
  return pid;
}

int
main(void)
{
  int rec[3];
  int i;
  int sumresp = 0, sumturn = 0;

  int testpriorities[NPROC_TEST] = {
    1, 2, 3, 4, 5, 1, 2, 3, 4, 5
  };

  pipe(fds);
  setpriority(30);

  printf("\n=== PRIORITY SCHEDULER TEST ===\n\n");

  for(i = 0; i < NPROC_TEST; i++){
    spawn(testpriorities[i], WORK);
  }

  for(i = 0; i < NPROC_TEST; i++){
    if(read(fds[0], rec, sizeof(rec)) != sizeof(rec)){
      fprintf(2, "pipe read failed\n");
      exit(1);
    }

    sumresp += rec[1];
    sumturn += rec[2];
  }

  for(i = 0; i < NPROC_TEST; i++){
    wait(0);
  }

  printf("\n=== RESULTS ===\n");
  printf("Average response: %d ticks\n",
         sumresp / NPROC_TEST);
  printf("Average turnaround: %d ticks\n",
         sumturn / NPROC_TEST);
  printf("Processes tested: %d\n", NPROC_TEST);

  exit(0);
}

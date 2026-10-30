
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NPROC_TEST  10
#define SAMPLE      10
#define WORK        850

int fds[2];

int pids[NPROC_TEST];
int priorities[NPROC_TEST];

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

    printf("START: PID %d | Priority %d | Tick %d\n",
           getpid(), prio, started);

    spin(work);

    uint done = uptime();

    printf("FINISH: PID %d | Priority %d | Tick %d\n",
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

void
monitor(void)
{
  struct pstat ps[NPROC];

  for(;;){
    int n = getprocs(ps);

    for(int p = 0; p < NPROC_TEST; p++){
      for(int i = 0; i < n; i++){
        if(ps[i].pid == pids[p] &&
           ps[i].state == RUNNABLE){

          uint age = uptime() - ps[i].readytime;

          printf("AGING: PID %d | Priority %d | %d ticks\n",
                 pids[p], priorities[p], age);
        }
      }
    }

    pause(SAMPLE);
  }
}

int
main(void)
{
  int rec[3];
  int i, monpid;
  int sumresp = 0, sumturn = 0;

  int testpriorities[NPROC_TEST] = {
    1, 2, 3, 4, 5, 1, 2, 3, 4, 5
  };

  pipe(fds);
  setpriority(30);

  printf("\n=== PRIORITY AGING TEST ===\n\n");

  // Create processes with different priorities.
  for(i = 0; i < NPROC_TEST; i++){
    priorities[i] = testpriorities[i];
    pids[i] = spawn(testpriorities[i], WORK);
  }

  monpid = fork();

  if(monpid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(monpid == 0){
    monitor();
    exit(0);
  }

  // Collect process results.
  for(i = 0; i < NPROC_TEST; i++){
    if(read(fds[0], rec, sizeof(rec)) != sizeof(rec)){
      fprintf(2, "pipe read failed\n");
      exit(1);
    }

    sumresp += rec[1];
    sumturn += rec[2];
  }

  kill(monpid);

  for(i = 0; i < NPROC_TEST + 1; i++)
    wait(0);

  printf("\n=== FINAL RESULTS ===\n");
  printf("Average response: %d ticks\n",
         sumresp / NPROC_TEST);
  printf("Average turnaround: %d ticks\n",
         sumturn / NPROC_TEST);
  printf("Processes tested: %d\n", NPROC_TEST);

  exit(0);
}

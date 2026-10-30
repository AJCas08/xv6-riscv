
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NHIGH     10
#define GAP       15
#define SAMPLE    10
#define HIGHWORK  850
#define LOWWORK   850
#define HIGHPRIO  5
#define LOWPRIO   1

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

    printf("START: PID %d | Priority %d | Waited %d ticks\n",
           getpid(), prio, started - born);

    spin(work);

    uint done = uptime();

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
monitor(int lowpid)
{
  struct pstat ps[NPROC];

  for(;;){
    int n = getprocs(ps);
    uint now = uptime();
    struct pstat *low = 0;

    for(int i = 0; i < n; i++){
      if(ps[i].pid == lowpid)
        low = &ps[i];
    }

    if(low == 0 || low->state == ZOMBIE)
      exit(0);

    if(low->state == RUNNABLE){
      printf("AGING: LOW PID %d | Aged %d ticks\n",
             lowpid, now - low->readytime);
    }

    pause(SAMPLE);
  }
}

int
main(void)
{
  int n = NHIGH + 2;
  int rec[3], i, lowpid, monpid;
  int sumresp = 0, sumturn = 0;

  pipe(fds);
  setpriority(30);

  printf("\n=== AGING TEST STARTED ===\n\n");

  spawn(HIGHPRIO, HIGHWORK);
  lowpid = spawn(LOWPRIO, LOWWORK);

  monpid = fork();

  if(monpid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }

  if(monpid == 0){
    monitor(lowpid);
    exit(0);
  }

  for(i = 0; i < NHIGH; i++){
    spawn(HIGHPRIO, HIGHWORK);
    pause(GAP);
  }

  for(i = 0; i < n; i++){
    if(read(fds[0], rec, sizeof(rec)) != sizeof(rec)){
      fprintf(2, "pipe read failed\n");
      exit(1);
    }

    sumresp += rec[1];
    sumturn += rec[2];
  }

  kill(monpid);

  for(i = 0; i < n + 1; i++)
    wait(0);

  printf("\n=== FINAL RESULTS ===\n");
  printf("Average response: %d ticks\n", sumresp / n);
  printf("Average turnaround: %d ticks\n", sumturn / n);
  printf("Processes tested: %d\n", n);

  exit(0);
}

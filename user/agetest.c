#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/pstat.h"
#include "user/user.h"

#define NHIGH     10
#define GAP       15     // ticks between high-priority arrivals
#define SAMPLE    10     // ticks between monitor samples (= AGING_INTERVAL)
#define HIGHWORK  850    // millions of loop iterations; tune with calib
#define LOWWORK   850
#define HIGHPRIO  5
#define LOWPRIO   1

int fds[2];

static char *states[] = {
  [UNUSED]   "unused  ",
  [USED]     "used    ",
  [SLEEPING] "sleeping",
  [RUNNABLE] "runnable",
  [RUNNING]  "running ",
  [ZOMBIE]   "zombie  "
};

void
spin(int n)              // n = millions of iterations
{
  volatile int x = 0;
  for(int j = 0; j < n; j++)
    for(int i = 0; i < 1000000; i++)
      x += i;
}

// Start a CPU-bound child at priority prio. Returns the child's pid.
int
spawn(int prio, int work)
{
  setpriority(prio);                 // child inherits this
  uint born = uptime();              // arrival time
  int pid = fork();
  if(pid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }
  if(pid == 0){
    int rec[3];
    uint started = uptime();         // first time this child got the CPU
    printf("tick %d: pid %d (prio %d) STARTED, waited %d ticks\n",
           started, getpid(), prio, started - born);
    spin(work);
    uint done = uptime();
    printf("tick %d: pid %d (prio %d) FINISHED, turnaround %d ticks\n",
           done, getpid(), prio, done - born);
    rec[0] = prio;
    rec[1] = started - born;         // response time
    rec[2] = done - born;            // turnaround time
    write(fds[1], rec, sizeof(rec));
    exit(0);
  }
  setpriority(30);                   // parent goes back to top priority
  printf("tick %d: pid %d (prio %d) ARRIVED\n", born, pid, prio);
  return pid;
}

// Samples the low-priority process every SAMPLE ticks and prints how it ages.
void
monitor(int lowpid)
{
  struct pstat ps[NPROC];

  for(;;){
    int n = getprocs(ps);
    uint now = uptime();
    struct pstat *low = 0;
    int waiting = 0;

    for(int i = 0; i < n; i++){
      if(ps[i].pid == lowpid)
        low = &ps[i];
      else if(ps[i].state == RUNNABLE && ps[i].priority == HIGHPRIO)
        waiting++;
    }

    if(low == 0)
      exit(0);

    printf("tick %d: LOW %s priority %d age %d | %d high jobs waiting\n",
           now, states[low->state], low->priority,
           low->state == RUNNABLE ? now - low->readytime : 0,
           waiting);

    if(low->state == ZOMBIE)
      exit(0);

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
  printf("agetest: starting at tick %d\n", uptime());

  spawn(HIGHPRIO, HIGHWORK);               // keeps the CPU busy first
  lowpid = spawn(LOWPRIO, LOWWORK);        // the process that might starve

  monpid = fork();                         // monitor runs at priority 30
  if(monpid < 0){
    fprintf(2, "fork failed\n");
    exit(1);
  }
  if(monpid == 0){
    monitor(lowpid);
    exit(0);
  }

  for(i = 0; i < NHIGH; i++){              // stream of new high-priority jobs
    spawn(HIGHPRIO, HIGHWORK);
    pause(GAP);
  }

  for(i = 0; i < n; i++){
    read(fds[0], rec, sizeof(rec));
    if(rec[0] == LOWPRIO)
      printf("LOW  (prio %d): response %d, turnaround %d\n",
             LOWPRIO, rec[1], rec[2]);
    sumresp += rec[1];
    sumturn += rec[2];
  }

  kill(monpid);
  for(i = 0; i < n + 1; i++)               // children + monitor
    wait(0);

  printf("average response %d, average turnaround %d (%d procs)\n",
         sumresp / n, sumturn / n, n);
  exit(0);
}
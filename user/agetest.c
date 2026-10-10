#include "kernel/types.h"
#include "user/user.h"

#define NHIGH     20
#define GAP       15           // ticks between high-priority arrivals
#define HIGHWORK  20000000     // tune: about 20 ticks of CPU each
#define LOWWORK   60000000     // tune: about 60 ticks of CPU

int fds[2];

void
spin(int n)              // n = millions of iterations
{
  volatile int x = 0;
  for(int j = 0; j < n; j++)
    for(int i = 0; i < 1000000; i++)
      x += i;
}

void
spawn(int prio, int work)
{
    setpriority(prio);               // child inherits this
    uint born = uptime();
    int pid = fork();
    if(pid < 0){
        fprintf(2, "fork failed\n");
        exit(1);
    }
    if(pid == 0){
        int rec[3];
        rec[0] = prio;
        rec[1] = uptime() - born;      // response time: arrival -> first run
        spin(work);
        rec[2] = uptime() - born;      // turnaround time: arrival -> finish
        write(fds[1], rec, sizeof(rec));
        exit(0);
    }
    setpriority(30);                 // parent returns to top priority
}

int
main(void){
    int n = NHIGH + 1;
    int rec[3], i;
    int sumresp = 0, sumturn = 0;

    pipe(fds);
    setpriority(30);

    spawn(5, HIGHWORK);              // was 20
    spawn(1, LOWWORK);               // was 5
    for(i = 0; i < NHIGH; i++){
        spawn(5, HIGHWORK);            // was 20
        pause(GAP);
    }

    for(i = 0; i < n; i++){
        read(fds[0], rec, sizeof(rec));
        if(rec[0] == 1)
            printf("LOW  (prio 1): response %d, turnaround %d\n", rec[1], rec[2]);
        sumresp += rec[1];
        sumturn += rec[2];
    }
    for(i = 0; i < n; i++)
        wait(0);

    printf("average response %d, average turnaround %d (%d procs)\n",
            sumresp / n, sumturn / n, n);
    exit(0);
}
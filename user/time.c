#include "kernel/types.h"
#include "user/user.h"
#include "kernel/pstat.h"

int main(int argc, char *argv[]){
    if (argc < 2){
        fprintf(2, "Usage: time1 command[args...]\n");
        exit(1);
    }
    uint start = uptime();

    int pid = fork();

    if (0 > pid){
        fprintf(2, "time1: fork failed\n");
        exit(1);
    }

    if (pid == 0){
        exec(argv[1], &argv[1]);
        fprintf(2, "time1: exec %s failed\n", argv[1]);
        exit(1);
    }else{
        struct rusage ruse;
        int status;
        wait2(&status, &ruse);
        
        uint end = uptime();
        uint elapsed = end - start;
        uint cpu = ruse.cputime;
        uint percent = elapsed > 0 ? (cpu * 100) / elapsed : 0;
        printf("elapsed time: %d ticks, cpu time: %d ticks, %d%% CPU\n", elapsed, cpu, percent);
        exit(0);
    }
}
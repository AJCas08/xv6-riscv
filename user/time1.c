#include "kernel/types.h"
#include "user/user.h"

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
        wait(0);
        uint end = uptime();
        printf("Elapsed time: %d ticks\n", end - start);
        exit(0);
    }
}
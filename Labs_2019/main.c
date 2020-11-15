#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#define LIFETIME 5
#define ERR(source) (fprintf(stderr,"%s:%d\n",__FILE__,__LINE__),\
                     perror(source),kill(0,SIGKILL),\
                     exit(EXIT_FAILURE))

volatile sig_atomic_t i = 0;

void sigusr_handler(int sig) {
    i++;
}

int child_work() {
    signal(SIGUSR1, sigusr_handler);

    srand(getpid());
    int k = rand()%(LIFETIME+1);
    int seconds = 0;

    while(seconds < k) {
        sleep(1);
        seconds++;
        kill(0, SIGUSR1);
    }

    do {
        seconds = sleep(LIFETIME - k);
    } while (seconds > 0);

    fprintf(stdout, "[%d] K=%d\ti=%d\n", getpid(), k, i);

    unsigned int compound = 0;
    compound |= i;
    compound |= (k << 4);
    return compound;
}

void create_children(int n) {
    for (int i = 1; i <= n; i++) {
        int x;
        switch (fork()) {
            case 0:
                x = child_work();
                exit(x);
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
        }
    }
}

void parent_work() {
    while (1) {
        int status;
        pid_t p = waitpid(0, &status, WNOHANG);
        if(p > 0) {
            unsigned int exit_status = WEXITSTATUS(status);
            int I = (exit_status & 0x0F);
            int k = (exit_status & 0xF0) >> 4;
            fprintf(stdout, "(%d,%d,%d)", p, k, I);
        }
        if (p < 0 && errno == ECHILD)
            break;
    }
}

void usage(char *name) {
    fprintf(stderr, "USAGE: %s N F\n", name);
    fprintf(stderr, "N - number of children [1,100]\n");
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        usage(argv[0]);
    }

    int n = atoi(argv[1]);

    signal(SIGUSR1, SIG_IGN);

    create_children(n);
    parent_work();

    while(wait(NULL) > 0);
    fprintf(stdout, "\n");
}

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                    perror(source), kill(0, SIGKILL), \
                    exit(EXIT_FAILURE))


volatile sig_atomic_t last_signal = 0;

void setHandler(void (*f)(int), int sigNo) {

    // This structure specifies how to handle a signal

    struct sigaction act;

    // Copies '0' in each sizeof(struct sigaction) bytes in the structure above

    memset(&act, 0, sizeof(struct sigaction));

    // sa_handler field identifies the action to be associated with the specified signal
    // here we are setting it to f provided in the function

    act.sa_handler = f;

    // sigaction returns -1 if not successful, hence here we are handling an error

    if (-1 == sigaction(sigNo, &act, NULL)) {
        ERR("sigaction");
    }
}

// Function handling given signal

void sig_handler(int sig) {
    last_signal = sig;
}

// SIGCHLD signal is sent to a parent process sen child process stops or terminates.
// This function handles this signal.

void sigchld_handler(int sig) {

    pid_t pid;

    for (;;) {

        // Will return process ID for which we're waiting

        pid = waitpid(0, NULL, WNOHANG);

        // In case we can't get process status

        if (pid == 0) {
            return;
        }

        // ECHILD == no child processes

        if (pid <= 0) {
            if (errno == ECHILD) {
                return;
            }
            ERR("waitpid");
        }
    }
}

// Function defined for error printing

void usage(char * name) {
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

// Function assigning tasks to processes

void child_work() {

    // Providing seed to the random number generator

    int p = 30;
    srand(time(NULL) * getpid());

    // Generating pseudorandom number in [5, 10] range

    int t = 100 + rand() % (200);
    struct timespec time = {0, t * 10000};

    // Function sending p SIGUSR1 signals

    for (int i = 0; i < p; i++) {

        // Nanosleep handles delays

        nanosleep(&time, NULL);

        // Sending SIGUSR1 to parent and checking if everything went fine

        if (kill(getppid(), SIGUSR1)) {
            ERR("kill");
        } else {
            printf("*");
        }
    }

    printf("\n");

}

void create_children(int n) {
    while (n-- > 0) {
        switch(fork()) {
            case 0:
                child_work();
                fprintf(stdout, "[%d] terminating\n", getpid());
                exit(EXIT_SUCCESS);
                
            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
        }
    }
}

void parent_work() {

    while (1) {

        last_signal = 0;

        // Checking if last received signal is SIGUSR2

        // if (last_signal == SIGUSR2) {

            // Replaces current mask with oldMask, wait until signal from
            // oldMask shows up
            
            kill(0, SIGUSR2);
            return;

        // }
    }
}

int main(int argc, char ** argv) {

    int m;

    if (argc != 2) {
        usage(argv[0]);
    }

    m = atoi(argv[1]);

    // Setting handlers for signals
    
    setHandler(sig_handler, SIGUSR1);
    setHandler(SIG_IGN, SIGUSR2);
    // setHandler(sigchld_handler, SIGCHLD);

    create_children(m);
    parent_work();
    
    // while(wait(NULL) > 0);
    
    return EXIT_SUCCESS;
    
}

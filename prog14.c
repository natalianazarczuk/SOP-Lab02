#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <time.h>

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                    perror(source), kill(0, SIGKILL), \
                    exit(EXIT_FAILURE))

// Global variable used to exchange information in signal handling routing

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
    // sigaction examines and changes signal action

    if (-1 == sigaction(sigNo, &act, NULL)) {
        ERR("sigaction");
    }
}

// Function used to show which process received which signal, assigning last signal
// to a global variable

void sig_handler(int sig) {
    printf("[%d] received signal %d\n", getpid(), sig);
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

void child_work(int l) {

    int t, tt;

    // Setting seed for random function

    srand(getpid());

    // Getting random value in range [5, 10]

    t = rand() % 6 + 5;

    // Loop iterating l times

    while (l-- > 0) {

        // Process sleeps for random time generated above

        for (tt = t; tt > 0; tt = sleep(tt));

        // Checking task conditions and informing about termination

        if (last_signal == SIGUSR1) {
            printf("Success [%d]\n", getpid());
        } else {
            printf("Failed [%d]\n", getpid());
        }

        printf("[%d] Terminates \n", getpid());

    }
}

// Parent process that send sequentially SIGUSR1 and SIGUSR2 to all
// sub-processes in a loop with delays of k and p seconds

void parent_work(int k, int p, int l) {

    // Structures holding an interval broken down into seconds and nanoseconds

    struct timespec tk = {k, 0};
    struct timespec tp = {p, 0};

    // Sending SIGALRM to handle

    setHandler(sig_handler, SIGALRM);

    // Function generating SIGALRM signal after l * 10 seconds

    alarm(l * 10);

    // While alarm hasn't yet been invoked...

    while (last_signal != SIGALRM) {

        // Nanosleep handles delays

        nanosleep(&tk, NULL);

        // Sends signal SIGUSR1 to all sub-processes and checks correctness

        if (kill(0, SIGUSR1) < 0) {
            ERR("kill");
        }

        nanosleep(&tp, NULL);

        // Sends signal SIGUSR2 to all sub-processes and checks correctness

        if (kill(0, SIGUSR2) < 0) {
            ERR("kill");
        }
    }

    printf("[PARENT] Terminates \n");
}

void create_children(int n, int l) {

    // Creating n children

    while (n-- > 0) {

        // Checking if fork function has been successful

        switch(fork()) {

            case 0:
                setHandler(sig_handler, SIGUSR1);
                setHandler(sig_handler, SIGUSR2);
                child_work(l);
                exit(EXIT_SUCCESS);

            case -1:
                perror("Fork:");
                exit(EXIT_FAILURE);

        }
    }
}

// Error printing function

void usage(void) {

    fprintf(stderr, "USAGE: signals n k p l\n");
    fprintf(stderr,"n - number of children\n");
    fprintf(stderr, "k - Interval before SIGUSR1\n");
    fprintf(stderr, "p - Interval before SIGUSR2\n");
    fprintf(stderr, "l - lifetime of child in cycles\n");
    exit(EXIT_FAILURE);

}

int main(int argc, char ** argv) {

    int n, k, p, l;

    // Checking correctness of arguments

    if (argc != 5) {
        usage();
    }

    n = atoi(argv[1]);
    k = atoi(argv[2]);
    p = atoi(argv[3]);
    l = atoi(argv[4]);

    if (n <= 0 || k <= 0 || p <= 0 || l <= 0) {
        usage();
    }

    // Setting handlers for different signals

    setHandler(sigchld_handler, SIGCHLD);

    // SIG_IGN == makes signal ignored

    setHandler(SIG_IGN, SIGUSR1);
    setHandler(SIG_IGN, SIGUSR2);

    create_children(n, l);
    parent_work(k, p, l);
    
    // If the current process have no child processes wait(NULL) returns negative

    while (wait(NULL) > 0);
    return EXIT_SUCCESS;

}


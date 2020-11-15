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

void child_work(int m, int p) {

    int count = 0;

    // Structure holding an interval broken down into seconds and nanoseconds

    struct timespec t = {0, m * 10000};

    while (1) {

        // Function sending p SIGUSR1 signals

        for (int i = 0; i < p; i++) {

            // Nanosleep handles delays

            nanosleep(&t, NULL);

            // Sending SIGUSR1 to parent and checking if everything went fine

            if (kill(getppid(), SIGUSR1)) {
                ERR("kill");
            }
        }

        nanosleep(&t, NULL);

        // Sending SIGUSR2 to parent and checking if everything went fine

        if (kill(getppid(), SIGUSR2)) {
            ERR("kill");
        }

        count++;
        printf("[%d] sent %d SIGUSR2\n", getpid(), count);

    }
}

void parent_work(sigset_t oldMask) {

    int count = 0;

    while (1) {

        last_signal = 0;

        // Checking if last received signal is SIGUSR2

        while (last_signal != SIGUSR2) {

            // Replaces current mask with oldMask, wait until signal from
            // oldMask shows up

            sigsuspend(&oldMask);
        }

        count++;
        printf("[PARENT] received %d SIGUSR2\n", count);

    }
}

// Error printing function

void usage(char * name) {

    fprintf(stderr, "USAGE: %s m p\n", name);
    fprintf(stderr,"m - number of 1/1000 miliseconds between signals [1, 999], i.e. one milisecond maximum\n");
    fprintf(stderr, "p - after p SIGUSR1 send one SIGUSR2 [1, 999]\n");
    exit(EXIT_FAILURE);

}

int main(int argc, char ** argv) {

    int m, p;

    if (argc != 3) {
        usage(argv[0]);
    }

    m = atoi(argv[1]);
    p = atoi(argv[2]);

    if (m <= 0 || m > 999 || p <= 0 || p > 999) {
        usage(argv[0]);
    }

    // Setting handlers for signals

    setHandler(sigchld_handler, SIGCHLD);
    setHandler(sig_handler, SIGUSR1);
    setHandler(sig_handler, SIGUSR2);

    // Creating new signal sets

    sigset_t mask, oldMask;

    // Initializing and emptying mask set

    sigemptyset(&mask);

    // Adding elements to set

    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    // Changing blocked set to mask, saving old in oldMask
    // Argument SIG_BLOCK: The resulting set shall be the union of the current set and
    //                     the signal set pointed to by set.

    sigprocmask(SIG_BLOCK, &mask, &oldMask);

    pid_t pid;

    // Creating new process and checking correctness of operation

    if ((pid = fork()) < 0) {
        ERR("fork");
    }

    // If successful, send children to work, else parent goes to work

    if (0 == pid) {
        child_work(m, p);
    } else {
        parent_work(oldMask);
        while (wait(NULL) > 0);
    }

    // Argument SIG_UNBLOCK: The resulting set shall be the union of the current set and
    //                       the signal set pointed to by set.

    sigprocmask(SIG_UNBLOCK, &mask, NULL);
    return EXIT_SUCCESS;

}


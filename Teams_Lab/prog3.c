#define _GNU_SOURCE

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
    fprintf(stdout, "*");
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

void child_work(int number) {

    // Providing seed to the random number generator

    srand(time(NULL) * getpid());

    // Generating pseudorandom number in [5, 10] range

    int t = rand() % (number);
    struct timespec time = {0, t * 100 * 10000};

    // Function sending p SIGUSR1 signals

    printf("Ni: %d, C: %d\n", number, t);

    for (int i = 0; i < t; i++) {

        // Nanosleep handles delays

        nanosleep(&time, NULL);

        // Sending SIGUSR1 to parent and checking if everything went fine

        if (kill(getppid(), SIGUSR1)) {
            ERR("kill");
        }
    }
}

void create_children(char ** argv, int argc) {
    for (int i = 2; i < argc; i++) {
        switch(fork()) {
            case 0:
                child_work(atoi(argv[i]));
                // fprintf(stdout, "[%d] terminating\n", getpid());
                exit(EXIT_SUCCESS);

            case -1:
                perror("fork");
                exit(EXIT_FAILURE);
        }
    }
}

void parent_work(char * name) {

    int out;

    out = TEMP_FAILURE_RETRY(open(name, O_APPEND | O_CREAT | O_WRONLY, 0666));

    if (out < 0) {
        ERR("open");
    }

    while (1) {

        while (last_signal == SIGUSR1) {

            char buf[100] = {'\0'};

            for (int i = 0; i < 100; i++) {
                buf[i] = rand() % ('z' - 'a' + 1) + 'a';
            }

            TEMP_FAILURE_RETRY(write(out, buf, 100));
            last_signal = 0;

        }

        while (last_signal != SIGUSR1) {
            pid_t p = waitpid(0, NULL, WNOHANG);
            if (p < 0 && errno == ECHILD) {
                return;
            }
        }
    }

    TEMP_FAILURE_RETRY(close(out));

}

int main(int argc, char ** argv) {

    char * name;

    if (argc < 3) {
        usage(argv[0]);
    }

    name = argv[1];

    // Setting handlers for signals

    signal(SIGUSR1, sig_handler);
    // setHandler(SIG_IGN, SIGUSR2);
    signal(SIGCHLD, sigchld_handler);

    create_children(argv, argc);
    parent_work(name);

    while(wait(NULL) > 0);

    return EXIT_SUCCESS;

}

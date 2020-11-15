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

// Function defined for error printing

void usage(char * name) {
    fprintf(stderr, "USAGE: %s 0<n\n", name);
    exit(EXIT_FAILURE);
}

// Function assigning tasks to processes

void child_work(int i) {

    // Providing seed to the random number generator

    srand(time(NULL) * getpid());

    // Generating pseudorandom number in [5, 10] range

    int t = 5 + rand() % (10 - 5 + 1);

    // Assigning process to sleep for t seconds

    sleep(t);

    printf("PROCESS with pid %d terminates\n", getpid());
}

// Function creating new processes

void create_children(int n) {

    // Variable storing process ID

    pid_t s;

    for (n--; n >= 0; n--) {

        // Checking if process was successfully created
        // Fork returns negative number if not

        if ((s = fork()) < 0) {
            ERR("Fork:");
        }

        // If s == 0 (successful creation of a process

        if (!s) {
            child_work(n);
            exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char ** argv) {

    int n;

    // Checking if sufficient number of arguments is provided

    if (argc < 2) {
        usage(argv[0]);
    }

    // Converting needed argument to integer

    n = atoi(argv[1]);

    // Checking if integer is correct

    if (n <= 0) {
        usage(argv[0]);
    }

    create_children(n);

    // Parent process controls child processes

    while (n > 0) {

        sleep(3);
        pid_t pid;

        for (;;) {

            // Will return process ID for which we're waiting

            pid = waitpid(0, NULL, WNOHANG);

            if (pid > 0) {
                n--;
            }

            // In case we can't get process status

            if (0 == pid) {
                break;
            }
            
            // ECHILD == no child processes

            if (0 >= pid) {
                if (ECHILD == errno) {
                    break;
                }
                ERR("waitpid:");
            }
        }

        printf("PARENT: %d processes remain\n", n);

    }

    return EXIT_SUCCESS;
}

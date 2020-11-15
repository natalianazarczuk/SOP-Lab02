#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>

#define ERR(source) (fprintf(stderr, "%s:%d\n", __FILE__, __LINE__), \
                    perror(source), kill(0, SIGKILL), \
                    exit(EXIT_FAILURE))

// Global variable used to count signal numbers

volatile sig_atomic_t sig_count = 0;

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
    sig_count++;
}

void child_work(int m) {

    // Structure holding an interval broken down into seconds and nanoseconds

    struct timespec t = {0, m * 10000};

    // We are setting SIG_DFL default handler for SIGUSR1 signal

    setHandler(SIG_DFL, SIGUSR1);

    while (1) {

        // Nanosleep handles delays

        nanosleep(&t, NULL);

        // Sending SIGUSR1 to parent and checking if everything went fine

        if (kill(getppid(), SIGUSR1)) {
            ERR("kill");
        }
    }
}

// Function used for bulk reading information from file

ssize_t bulk_read (int fd, char * buf, size_t count) {

    ssize_t c;
    ssize_t len = 0;

    // We are using this macro to retry the operation in a loop until we're done

    do {

        // We check whether some bytes were read

        c = TEMP_FAILURE_RETRY(read(fd, buf, count));

        // If there is an error in macro...

        if (c < 0) {
            return c;
        }

        // When we encounter EOF (we finished reading file)...

        if (c == 0) {
            return len;
        }

        // We're counting how much information we've processed

        buf += c;
        len += c;
        count -= c;

    } while (count > 0);

    return len;
}

// Function used for bulk writing to file from buffer

ssize_t bulk_write(int fd, char * buf, size_t count) {

    size_t c;
    ssize_t len = 0;

    // We are using this macro to retry the operation in a loop until we're done

    do {

        // We check whether some bytes were written

        c = TEMP_FAILURE_RETRY(write(fd, buf, count));
        
        // Checking if there wasn't any error

        if (c < 0) {
            return c;
        }

        buf += c;
        len += c;
        count -= c;

    } while (count > 0);

    return len;
}

void parent_work(int b, int s, char * name) {

    int i, in, out;
    ssize_t count;
    char * buf = malloc(s);

    // Checking malloc correctness

    if (!buf) {
        ERR("malloc");
    }

    // Opens file "name" for write only, if not existent creates it, truncates the
    // length to 0 and the file offset shall be set to the end of the file prior
    // to each write, octal mode and checks if it was correct

    if ((out = open(name, O_WRONLY|O_CREAT|O_TRUNC|O_APPEND, 0777)) < 0) {
        ERR("open");
    }

    // Opens /dev/urandom location as read only and checks if it worked

    if ((in = open("/dev/urandom", O_RDONLY)) < 0) {
        ERR("open");
    }

    // b == amount of blocks of set size

    for (i = 0; i < b; i++) {

        // Function reads s bytes from input and puts them in buffer

        if ((count = bulk_read(in, buf, s)) < 0) {
            ERR("read");
        }

        // Writes count bytes from buffer to out

        if ((count = bulk_write(out, buf, count)) < 0) {
            ERR("write");
        }

        // Informing about operation by stderr

        if (TEMP_FAILURE_RETRY(fprintf(stderr, "Blocks %ld bytes transferred. Signals RX:%d\n", count, sig_count) < 0)) {
            ERR("fprintf");
        }
    }

    // Closing files, freeing memory

    if (TEMP_FAILURE_RETRY(close(in))) {
        ERR("close");
    }

    if (TEMP_FAILURE_RETRY(close(out))) {
        ERR("close");
    }

    free(buf);

    // Sending SIGUSR1 signal to processes

    if (kill(0, SIGUSR1)) {
        ERR("kill");
    }
}

// Error printing function

void usage(char * name) {

    fprintf(stderr, "USAGE: %s m b s \n", name);
    fprintf(stderr,"m - number of 1/1000 miliseconds between signals [1, 999], i.e. one milisecond maximum\n");
    fprintf(stderr, "b - number of blocks [1, 999]\n");
    fprintf(stderr, "s - size of blocks [1, 999] in MB\n");
    fprintf(stderr, "name of the output file\n");
    exit(EXIT_FAILURE);

}

int main(int argc, char ** argv) {

    int m, b, s;
    char * name;

    if (argc != 5) {
        usage(argv[0]);
    }

    m = atoi(argv[1]);
    b = atoi(argv[2]);
    s = atoi(argv[3]);
    name = argv[4];

    if (m <= 0 || m > 999 || b <= 0 || b > 999 || s <= 0 || s > 999) {
        usage(argv[0]);
    }

    // Setting signal handler for SIGUSR1 signal

    setHandler(sig_handler, SIGUSR1);

    pid_t pid;

    // Creating process, checking correctness

    if ((pid = fork()) < 0) {
        ERR("fork");
    }

    if (0 == pid) {
        child_work(m);
    } else {
        parent_work(b, s * 1024 * 1024, name);
        while(wait(NULL) > 0);
    }

    return EXIT_SUCCESS;
}

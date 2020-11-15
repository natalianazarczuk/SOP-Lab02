# SOP Lab 2 - reference material

This short documentation will explain what's in each task from Operating Systems on the MiNI Faculty.
Further explanation is avaiable in each task's file. 

# Task 13 (b is complete version)

**Creating processes**, waiting for their death, printing number of active processes every few seconds. To know:
- fork
- getpid
- wait / waitpid
- sleep

# Task 14

Creating subprocesses, **sending signals** to them, recognizing received signals. To know:
- signal
- sigaction
- nanosleep
- alarm
- memset
- kill

# Task 15

Creating a child process sending signals to parent, **receiving these signals (by waiting) and counting their number**. To know:
- sigsuspend
- getppid
- sigprocmask
- sigaddset
- sigemptyset

# Task 16

Receiving signals from children (like in Task 15). **Creating files using descriptors, reading random data from /dev/urandom and writing to the file**. To know:
- open
- close
- write
- urandom
- mknod

// File: scheduler.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 3 March 2026

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>
#include <unistd.h>

typedef struct {
    int processNum;
    int arrival;
    int burst;
    int priority;
    int remaining;
    int started;
    int finished;
    pid_t pid;
} PCB;

static PCB procs[10];
static int n = 0;

static volatile sig_atomic_t currentTime = 0;
static int running = -1;

/**************************************************
Method Name: fork_and_exec
Returns: void
Input: int idx
Precondition: idx is a valid index in procs[]
Task: Forks a child process and execs ./prime with args:
      <processNum> <priority>, then stores the child's PID.
 **************************************************/
static void fork_and_exec(int idx) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        char pnum[16], pri[16];
        snprintf(pnum, sizeof(pnum), "%d", procs[idx].processNum);
        snprintf(pri, sizeof(pri), "%d", procs[idx].priority);

        execl("./prime", "prime", pnum, pri, (char *)NULL);
        perror("execl");
        _exit(1);
    }

    procs[idx].pid = pid;
}

/**************************************************
Method Name: all_finished
Returns: int
Input: N/A
Precondition: procs[] has been populated, n > 0
Task: Returns 1 if all processes are finished, otherwise 0.
 **************************************************/
static int all_finished(void) {
    for (int i = 0; i < n; i++) {
        if (!procs[i].finished) return 0;
    }
    return 1;
}

/**************************************************
Method Name: pick_next_ready
Returns: int
Input: N/A
Precondition: currentTime reflects the scheduler's time in seconds
Task: Chooses the next ready process based on preemptive priority
 **************************************************/
static int pick_next_ready(void) {
    int next = -1;
    int bestPri = 1 << 30;

    for (int i = 0; i < n; i++) {
        if (procs[i].finished) continue;
        if (procs[i].arrival > (int)currentTime) continue;
        if (procs[i].remaining <= 0) continue;

        if (next == -1 ||
            procs[i].priority < procs[next].priority ||
            (procs[i].priority == procs[next].priority &&
             procs[i].arrival < procs[next].arrival))
        {
            next = i;
        }
    }
    return next;
}

/**************************************************
Method Name: schedule_one_tick
Returns: void
Input: N/A
Precondition: Called once per second (timer tick)
Task: Decrements remaining time for the currently running process.
  If it reaches 0, terminate it and mark finished. If all processes 
  finished, exit scheduler. Picks the best ready process by priority.
  If needed, preempts current process (SIGTSTP) and starts/resumes 
  the selected one (fork/exec or SIGCONT).
 **************************************************/
static void schedule_one_tick(void) {
    if (running != -1) {
        if (procs[running].remaining > 0) {
            procs[running].remaining--;
        }
        if (procs[running].remaining == 0 && !procs[running].finished) {
            printf("\nScheduler: Time Now: %d seconds\n", (int)currentTime);
            printf("Terminating Process %d (Pid %d)\n", procs[running].processNum, (int)procs[running].pid);
            fflush(stdout);
            kill(procs[running].pid, SIGTERM);
            procs[running].finished = 1;
            running = -1;
        }
    }

    if (all_finished()) {
        printf("\nScheduler: Time Now: %d seconds\n", (int)currentTime);
        fflush(stdout);
        exit(0);
    }

    // Choose the best ready process
    int next = pick_next_ready();
    if (next == -1) {
        return;
    }

    // Preempt/switch
    if (next != running) {
      printf("\nScheduler: Time Now: %d seconds\n", (int)currentTime);

      if (running != -1) {
          // If next hasn't started yet, fork it first so PID exists
          if (!procs[next].started) {
              fork_and_exec(next);
              procs[next].started = 1;
          }

          printf("Suspending Process %d (Pid %d) and Resuming Process %d (Pid %d)\n",
                 procs[running].processNum, (int)procs[running].pid,
                 procs[next].processNum, (int)procs[next].pid);
          fflush(stdout);

          kill(procs[running].pid, SIGTSTP);
          kill(procs[next].pid, SIGCONT);
      }
      else {
        if (!procs[next].started) {
            fork_and_exec(next);
            procs[next].started = 1;

            printf("Scheduling to Process %d (Pid %d)\n",
                   procs[next].processNum, (int)procs[next].pid);
            fflush(stdout);
        } else {
            printf("Resuming Process %d (Pid %d)\n",
                   procs[next].processNum, (int)procs[next].pid);
            fflush(stdout);
        }

          kill(procs[next].pid, SIGCONT);
      }

    running = next;
    }
}

/**************************************************
Method Name: timer_handler
Returns: void
Input: int signum
Precondition: Triggered by SIGALRM once per second
Task: Increments scheduler time and runs one scheduling tick.
 **************************************************/
static void timer_handler(int signum) {
    (void)signum;
    currentTime++;
    schedule_one_tick();
}

/**************************************************
Method Name: read_input
Returns: void
Input: const char *filename
Precondition: filename points to a readable text file where each line
  contains: processNum arrival burst priority
Task: Reads process definitions from file into procs[] and initializes
  PCB fields (remaining/started/finished/pid).
 **************************************************/
static void read_input(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }
    n = 0;
    while (n < 10) {
        PCB p;
        int got = fscanf(f, "%d %d %d %d",
                         &p.processNum, &p.arrival, &p.burst, &p.priority);
        if (got == EOF) break;
        if (got != 4) {
            fprintf(stderr, "Bad input line (expected 4 ints).\n");
            exit(1);
        }
        p.remaining = p.burst;
        p.started = 0;
        p.finished = 0;
        p.pid = -1;
        procs[n++] = p;
    }

    fclose(f);
    if (n == 0) {
        fprintf(stderr, "No processes found in input.\n");
        exit(1);
    }
}

/**************************************************
Method Name: main
Returns: int
Input: int argc, char **argv
Precondition: Program must be run as ./scheduler input.txt
Task: Initializes scheduler state, installs SIGALRM handler,
  starts a 1-second interval timer, and loops forever while
  scheduling happens on each timer tick.
 **************************************************/
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    read_input(argv[1]);
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = timer_handler;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval timer;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) != 0) {
        perror("setitimer");
        return 1;
    }

    while (1) { }
}

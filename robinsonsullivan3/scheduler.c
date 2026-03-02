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
    int priority;   // lower number = higher priority
    int remaining;
    int started;
    int finished;
    pid_t pid;
} PCB;

static PCB procs[10];
static int n = 0;

static volatile sig_atomic_t currentTime = 0;  // seconds
static int running = -1;                       // index in procs[], -1 if none

static void fork_and_exec(int idx) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    if (pid == 0) {
        // child -> exec prime with args: processNum priority
        char pnum[16], pri[16];
        snprintf(pnum, sizeof(pnum), "%d", procs[idx].processNum);
        snprintf(pri, sizeof(pri), "%d", procs[idx].priority);

        execl("./prime", "prime", pnum, pri, (char *)NULL);
        perror("execl");
        _exit(1);
    }

    procs[idx].pid = pid;
}

static int all_finished(void) {
    for (int i = 0; i < n; i++) {
        if (!procs[i].finished) return 0;
    }
    return 1;
}

static int pick_next_ready(void) {
    int next = -1;
    int bestPri = 1 << 30;

    for (int i = 0; i < n; i++) {
        if (procs[i].finished) continue;
        if (procs[i].arrival > (int)currentTime) continue;
        if (procs[i].remaining <= 0) continue;

        // Highest priority = lowest priority number
        if (procs[i].priority < bestPri) {
            bestPri = procs[i].priority;
            next = i;
        }
        // Tie-breaker by arrival time is naturally handled because input is
        // already in increasing arrival order; we keep the first one found.
    }
    return next;
}

static void schedule_one_tick(void) {
    // 1) Account for the last second of CPU time for whoever was running
    if (running != -1) {
        if (procs[running].remaining > 0) {
            procs[running].remaining--;
        }
        if (procs[running].remaining == 0 && !procs[running].finished) {
            printf("Scheduler: Time Now: %d second(s)\n", (int)currentTime);
            printf("Terminating Process %d (Pid %d)\n",
                   procs[running].processNum, (int)procs[running].pid);
            fflush(stdout);

            kill(procs[running].pid, SIGTERM);
            procs[running].finished = 1;
            running = -1;
        }
    }

    if (all_finished()) {
        printf("Scheduler: Time Now: %d second(s)\n", (int)currentTime);
        printf("All processes finished. Exiting scheduler.\n");
        fflush(stdout);
        exit(0);
    }

    // 2) Choose best ready process (highest priority = lowest number)
    int next = pick_next_ready();
    if (next == -1) {
        // nobody ready this second
        return;
    }

    // 3) If different from current running, preempt/switch
    if (next != running) {
        printf("Scheduler: Time Now: %d second(s)\n", (int)currentTime);

        // Suspend current, if any
        if (running != -1) {
            printf("Suspending Process %d (Pid %d)\n",
                   procs[running].processNum, (int)procs[running].pid);
            fflush(stdout);
            kill(procs[running].pid, SIGTSTP);
        }

        // Start or resume the chosen process
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
            kill(procs[next].pid, SIGCONT);
        }

        running = next;
    }
}

static void timer_handler(int signum) {
    (void)signum;
    currentTime++;
    schedule_one_tick();
}

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

    // Busy loop; scheduling happens in SIGALRM handler each second.
    while (1) { }
}

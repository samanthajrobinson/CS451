// File: srtfScheduler.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 18 February 2026

#include "timer.h"
#include "srtfScheduler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

#define MAX_PROCS 256

typedef struct {
    int procNum;        // Logical process number from input file
    int arrival;        // Arrival time (tick) when process becomes eligible
    int remaining;      // Remaining burst time (ticks) left to execute
    pid_t pid;          // OS PID of spawned child process (0 means not spawned yet)
    int finished;       // 1 if process completed, else 0
} Process;

static Process procs[MAX_PROCS];
static int nProcs = 0;

static int currentTime = 0;    // Global scheduler clock (ticks)
static int running = -1;       // Index of currently running process in procs[], -1 means none
static int completed = 0;      // Count of finished processes

/**************************************************
Method Name: load_input
Returns: void
Input: const char *path
Precondition: path points to a readable input file with a header line and then rows: procNum arrival burst
Task: Loads process definitions from the input file into the global procs[] array and initializes scheduler state.
**************************************************/
static void load_input(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    char line[256];

    // Ignore header line (first line). The project input format includes a header.
    if (!fgets(line, sizeof(line), f)) {
        fprintf(stderr, "Input file is empty\n");
        fclose(f);
        exit(1);
    }

    // Read each process definition line.
    while (fgets(line, sizeof(line), f)) {
        // Skip blank lines, Windows CRLF-only lines, and comment lines.
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;

        int p, a, b;
        // Expect exactly three integers: process number, arrival time, burst time.
        if (sscanf(line, "%d %d %d", &p, &a, &b) == 3) {
            if (nProcs >= MAX_PROCS) {
                fprintf(stderr, "Too many processes (max %d)\n", MAX_PROCS);
                fclose(f);
                exit(1);
            }

            // Initialize per-process scheduling fields.
            procs[nProcs].procNum = p;
            procs[nProcs].arrival = a;
            procs[nProcs].remaining = b; // remaining time starts as full burst time
            procs[nProcs].pid = 0;       // not spawned yet
            procs[nProcs].finished = 0;  // not completed
            nProcs++;
        }
        // If a line doesn't match the expected format, it is silently ignored.
        // (That matches the current behavior. If you want strict parsing, change it.)
    }

    fclose(f);

    if (nProcs == 0) {
        fprintf(stderr, "No processes loaded (check input format)\n");
        exit(1);
    }
}

/**************************************************
Method Name: choose_best_ready
Returns: int
Input: void
Precondition: procs[] and nProcs are initialized, currentTime reflects the scheduler tick.
Task: Selects the index of the ready process with the smallest remaining time (SRTF). Break ties by earlier arrival, then smaller procNum.
**************************************************/
static int choose_best_ready(void) {
    int best = -1;

    for (int i = 0; i < nProcs; i++) {
        // Ignore completed processes.
        if (procs[i].finished) continue;

        // Process is not ready until its arrival time.
        if (procs[i].arrival > currentTime) continue;

        // First eligible process becomes the baseline.
        if (best == -1) {
            best = i;
            continue;
        }

        // Primary SRTF rule: smallest remaining time wins.
        if (procs[i].remaining < procs[best].remaining) {
            best = i;
            continue;
        }

        // Tie-breakers to keep behavior deterministic.
        if (procs[i].remaining == procs[best].remaining) {
            // Earlier arrival time wins.
            if (procs[i].arrival < procs[best].arrival) {
                best = i;
                continue;
            }
            // If arrival also ties, smaller process number wins.
            if (procs[i].arrival == procs[best].arrival &&
                procs[i].procNum < procs[best].procNum) {
                best = i;
                continue;
            }
        }
    }

    return best; // -1 means no ready process exists at this time
}

/**************************************************
Method Name: spawn_child
Returns: void
Input: int idx
Precondition: idx is a valid index into procs[], and procs[idx].pid == 0 (child not spawned yet).
Task: Forks and execs the ./child program for the selected process, storing the spawned PID in procs[idx].pid.
**************************************************/
static void spawn_child(int idx) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process: exec the worker program.
        // Pass the process number as "-p <procNum>".
        char pstr[32];
        snprintf(pstr, sizeof(pstr), "%d", procs[idx].procNum);

        execlp("./child", "./child", "-p", pstr, (char *)NULL);

        // If execlp returns, it failed.
        perror("execlp");
        _exit(1); // Use _exit in child after fork to avoid flushing parent buffers twice.
    }

    // Parent process: record the child's PID so we can signal it later.
    procs[idx].pid = pid;
}

/**************************************************
Method Name: start_or_resume
Returns: void
Input: int idx
Precondition: idx is a valid index into procs[], procs[idx] is not finished, and currentTime is current scheduler tick.
Task: Ensures the process exists (spawn if needed) and then runs it by sending SIGCONT. Prints START/CONTINUE log output.
**************************************************/
static void start_or_resume(int idx) {
    if (procs[idx].pid == 0) {
        // First time this process is chosen, create the child process.
        spawn_child(idx);

        // Log that the process is starting for the first time.
        printf("t=%d START p=%d pid=%d rem=%d\n",
               currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
        fflush(stdout);
    } else {
        // Process already exists, so this is a resume after preemption.
        printf("t=%d CONTINUE p=%d pid=%d rem=%d\n",
               currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
        fflush(stdout);
    }

    // Let the process run (or keep running) by continuing it.
    kill(procs[idx].pid, SIGCONT);
}

/**************************************************
Method Name: preempt
Returns: void
Input: int idx
Precondition: idx is a valid index into procs[], procs[idx].pid != 0, and the process is currently running.
Task: Stops the running process using SIGTSTP to simulate preemption and prints a PREEMPT log line.
**************************************************/
static void preempt(int idx) {
    printf("t=%d PREEMPT p=%d pid=%d rem=%d\n",
           currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
    fflush(stdout);

    // SIGTSTP requests the process to stop (like Ctrl+Z), simulating a context switch out.
    kill(procs[idx].pid, SIGTSTP);
}

/**************************************************
Method Name: finish
Returns: void
Input: int idx
Precondition: idx is a valid index into procs[], procs[idx].pid != 0, and procs[idx].remaining <= 0.
Task: Terminates the child process, marks it finished in the scheduler tables, increments completion count, and prints a FINISH log line.
**************************************************/
static void finish(int idx) {
    printf("t=%d FINISH p=%d pid=%d\n",
           currentTime, procs[idx].procNum, procs[idx].pid);
    fflush(stdout);

    // End the child process now that its burst is complete.
    kill(procs[idx].pid, SIGTERM);

    // Update scheduler bookkeeping.
    procs[idx].finished = 1;
    completed++;
}

/**************************************************
Method Name: scheduler_tick
Returns: void
Input: void
Precondition: timer_start() has been called with scheduler_tick as its callback, and procs[] has been loaded.
Task: Advances the scheduler one tick: updates the running process remaining time, finishes it if done, selects the best ready process (SRTF), preempts if needed, and starts/resumes the chosen process.
**************************************************/
void scheduler_tick(void) {
    // Advance simulated time by 1 tick (called once per second by the timer).
    currentTime++;

    // If a process is currently running, charge it one unit of CPU time.
    if (running != -1 && !procs[running].finished) {
        procs[running].remaining--;

        // If it just completed, finalize it and clear the CPU.
        if (procs[running].remaining <= 0) {
            finish(running);
            running = -1;
        }
    }

    // If everything has finished, print and exit the scheduler.
    if (completed == nProcs) {
        printf("Complete!\n");
        fflush(stdout);
        exit(0);
    }

    // Pick the best ready process according to SRTF.
    int best = choose_best_ready();
    if (best == -1) {
        // No ready processes at this time, CPU stays idle.
        return;
    }

    // If the chosen process is already running, nothing to switch.
    // (This prints a CONTINUE each tick for the same running process, matching current behavior.)
    if (best == running) {
        printf("t=%d CONTINUE p=%d pid=%d rem=%d\n",
               currentTime,
               procs[running].procNum,
               procs[running].pid,
               procs[running].remaining);
        fflush(stdout);
        return;
    }

    // If a different process should run now, stop the current one (if any).
    if (running != -1) {
        preempt(running);
    }

    // Context switch in the new best process.
    running = best;
    start_or_resume(running);
}

/**************************************************
Method Name: main
Returns: int
Input: int argc, char **argv
Precondition: argv[1] is provided and is a valid input file path.
Task: Validates arguments, loads process input, starts the 1 Hz timer that drives scheduler_tick(), then waits indefinitely for timer signals.
**************************************************/
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    // Load process list from the input file into procs[].
    load_input(argv[1]);

    // Start periodic SIGALRM-based timer, calling scheduler_tick once per second.
    timer_start(scheduler_tick);

    // Sleep until signals arrive. scheduler_tick runs inside the SIGALRM handler chain.
    while (1) pause();

    return 0; // Unreachable, but kept for completeness.
}

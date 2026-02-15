// srtfScheduler.c  (bare-minimum preemptive SRTF scheduler)

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
    int procNum;
    int arrival;
    int remaining;   // burst remaining
    pid_t pid;       // 0 until forked
    int finished;    // 0/1
} Process;

static Process procs[MAX_PROCS];
static int nProcs = 0;

static int currentTime = 0;
static int running = -1;     // index into procs, -1 if none
static int completed = 0;

/* ----------------------------
   Input
   ---------------------------- */
static void load_input(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        perror("fopen");
        exit(1);
    }

    char line[256];

    // ignore header line
    if (!fgets(line, sizeof(line), f)) {
        fprintf(stderr, "Input file is empty\n");
        fclose(f);
        exit(1);
    }

    while (fgets(line, sizeof(line), f)) {
        // skip empty/comment-ish lines
        if (line[0] == '\n' || line[0] == '\r' || line[0] == '#') continue;

        int p, a, b;
        if (sscanf(line, "%d %d %d", &p, &a, &b) == 3) {
            if (nProcs >= MAX_PROCS) {
                fprintf(stderr, "Too many processes (max %d)\n", MAX_PROCS);
                fclose(f);
                exit(1);
            }
            procs[nProcs].procNum = p;
            procs[nProcs].arrival = a;
            procs[nProcs].remaining = b;
            procs[nProcs].pid = 0;
            procs[nProcs].finished = 0;
            nProcs++;
        }
        // if sscanf fails, ignore the line (bare minimum robustness)
    }

    fclose(f);

    if (nProcs == 0) {
        fprintf(stderr, "No processes loaded (check input format)\n");
        exit(1);
    }
}

/* ----------------------------
   Selection (SRTF + tie-breaks)
   ---------------------------- */
static int choose_best_ready(void) {
    int best = -1;

    for (int i = 0; i < nProcs; i++) {
        if (procs[i].finished) continue;
        if (procs[i].arrival > currentTime) continue; // not ready

        if (best == -1) {
            best = i;
            continue;
        }

        // smallest remaining time wins
        if (procs[i].remaining < procs[best].remaining) {
            best = i;
            continue;
        }

        if (procs[i].remaining == procs[best].remaining) {
            // tie 1: earlier arrival
            if (procs[i].arrival < procs[best].arrival) {
                best = i;
                continue;
            }
            // tie 2: smaller process number
            if (procs[i].arrival == procs[best].arrival &&
                procs[i].procNum < procs[best].procNum) {
                best = i;
                continue;
            }
        }
    }

    return best;
}

/* ----------------------------
   Child control
   ---------------------------- */
static void spawn_child(int idx) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        char pstr[32];
        snprintf(pstr, sizeof(pstr), "%d", procs[idx].procNum);
        execlp("./child", "./child", "-p", pstr, (char *)NULL);
        perror("execlp");
        _exit(1);
    }

    procs[idx].pid = pid;
}

static void start_or_resume(int idx) {
    if (procs[idx].pid == 0) {
        spawn_child(idx);
        printf("t=%d START p=%d pid=%d rem=%d\n",
               currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
        fflush(stdout);
    } else {
        printf("t=%d CONTINUE p=%d pid=%d rem=%d\n",
               currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
        fflush(stdout);
    }

    kill(procs[idx].pid, SIGCONT);
}

static void preempt(int idx) {
    printf("t=%d PREEMPT p=%d pid=%d rem=%d\n",
           currentTime, procs[idx].procNum, procs[idx].pid, procs[idx].remaining);
    fflush(stdout);
    kill(procs[idx].pid, SIGTSTP);
}

static void finish(int idx) {
    printf("t=%d FINISH p=%d pid=%d\n",
           currentTime, procs[idx].procNum, procs[idx].pid);
    fflush(stdout);

    kill(procs[idx].pid, SIGTERM);

    procs[idx].finished = 1;
    completed++;
}

/* ----------------------------
   Tick (called every 1 second)
   ---------------------------- */
void scheduler_tick(void) {
    currentTime++;

    // Run currently running process for 1 second
    if (running != -1 && !procs[running].finished) {
        procs[running].remaining--;

        if (procs[running].remaining <= 0) {
            finish(running);
            running = -1;
        }
    }

    // If all done
    if (completed == nProcs) {
        printf("Complete!\n");
        fflush(stdout);
        exit(0);
    }

    int best = choose_best_ready();
    if (best == -1) {
        return; // nothing ready yet
    }

    // 🔥 NEW SAFE CONTINUE CASE
    if (best == running) {
        // still the same process, explicitly print continue
        printf("t=%d CONTINUE p=%d pid=%d rem=%d\n",
               currentTime,
               procs[running].procNum,
               procs[running].pid,
               procs[running].remaining);
        fflush(stdout);
        return;
    }

    // Switch required
    if (running != -1) {
        preempt(running);
    }

    running = best;
    start_or_resume(running);
}



/* ----------------------------
   Main
   ---------------------------- */
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s input.txt\n", argv[0]);
        return 1;
    }

    load_input(argv[1]);

    // start 1-second timer ticks
    timer_start(scheduler_tick);

    // keep scheduler alive to receive SIGALRM
    while (1) pause();
    return 0;
}


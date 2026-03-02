// File: timer.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 18 February 2026

#include "timer.h"

#include <signal.h>
#include <sys/time.h>

static void (*tick_cb)(void) = 0; // Function pointer for the tick callback

static void alarm_handler(int sig) {
    (void)sig; // Don't use unneeded warning
    if (tick_cb) tick_cb(); // Calls tick callback
}

/**************************************************
Method Name: timer_start
Returns: void
Input: void (*on_tick)(void)
Precondition: on_tick is a function pointer
Task: Registers a SIGALRM handler, configures an interval timer,
 **************************************************/
void timer_start(void (*on_tick)(void)) {
    tick_cb = on_tick;

    // Install SIGALRM handler
    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    
    // SA_RESTART
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    // Timer: first tick in 1s, then every 1s
    struct itimerval t;
    t.it_value.tv_sec = 1;
    t.it_value.tv_usec = 0;
    t.it_interval.tv_sec = 1;
    t.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &t, NULL);
}


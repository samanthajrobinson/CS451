// File: timer.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 18 February 2026

#include "timer.h"

#include <signal.h>
#include <sys/time.h>

static void (*tick_cb)(void) = 0;

static void alarm_handler(int sig) {
    (void)sig;
    if (tick_cb) tick_cb();
}

void timer_start(void (*on_tick)(void)) {
    tick_cb = on_tick;

    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGALRM, &sa, NULL);

    struct itimerval t;
    t.it_value.tv_sec = 1;
    t.it_value.tv_usec = 0;
    t.it_interval.tv_sec = 1;
    t.it_interval.tv_usec = 0;

    setitimer(ITIMER_REAL, &t, NULL);
}


// File: prime.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 3 March 2026

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static int g_pnum = -1;
static int g_pri  = -1;

static unsigned long long current = 1234567890ULL;
static unsigned long long highestPrime = 0ULL;

static int isPrime(unsigned long long n) {
    if (n < 2ULL) return 0;
    if (n % 2ULL == 0ULL) return n == 2ULL;
    for (unsigned long long i = 3ULL; i * i <= n; i += 2ULL) {
        if (n % i == 0ULL) return 0;
    }
    return 1;
}

static unsigned long long nextPrime(unsigned long long x) {
    while (!isPrime(x)) x++;
    return x;
}

static void handle_tstp(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I am about to be suspended... "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
    raise(SIGSTOP);
}

static void handle_cont(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I just got resumed. "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
}

static void handle_term(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I completed my task and I am exiting. "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
    _exit(0);
}

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "prime usage: %s <processNum> <priority>\n", argv[0]);
        return 1;
    }

    g_pnum = atoi(argv[1]);
    g_pri  = atoi(argv[2]);

    current = 1234567890ULL + (unsigned long long)g_pnum * 100000ULL;

    // Ensure we have at least one real prime early so prints aren't 0.
    unsigned long long first = nextPrime(current);
    highestPrime = first;
    current = first + 1ULL;

    signal(SIGTSTP, handle_tstp);
    signal(SIGCONT, handle_cont);
    signal(SIGTERM, handle_term);

    printf("Process %d: My priority is %d, my PID is %d: I just got started.\n",
           g_pnum, g_pri, (int)getpid());
    printf("I am starting with the number %llu to find the next prime number.\n", current);
    fflush(stdout);

    while (1) {
        if (isPrime(current)) {
            highestPrime = current;
        }
        current++;
    }
}

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

/**************************************************
Method Name: isPrime
Returns: int
Input: unsigned long long n
Precondition: n is a positive integer
Task: Determines whether n is a prime number
 **************************************************/
static int isPrime(unsigned long long n) {
    if (n < 2ULL) return 0;
    for (unsigned long long i = 2ULL; i < n; i++) {
        if (n % i == 0ULL) return 0;
    }
    return 1;
}

/**************************************************
Method Name: handle_tstp
Returns: void
Input: int sig
Precondition: Triggered by SIGTSTP
Task: Prints suspend message and stops the process
 **************************************************/
static void handle_tstp(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I am about to be suspended... "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
    kill(getpid(), SIGSTOP);
}

/**************************************************
Method Name: handle_cont
Returns: void
Input: int sig
Precondition: Triggered by SIGCONT
Task: Prints resume message and continues execution
 **************************************************/
static void handle_cont(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I just got resumed. "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
}

/**************************************************
Method Name: handle_term
Returns: void
Input: int sig
Precondition: Triggered by SIGTERM
Task: Prints end message and ends the process
 **************************************************/
static void handle_term(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: I completed my task and I am exiting. "
           "Highest prime number I found is %llu.\n",
           g_pnum, g_pri, (int)getpid(), highestPrime);
    fflush(stdout);
    _exit(0);
}

/**************************************************
Method Name: main
Returns: int
Input: int argc, char **argv
Precondition: Program must be run as ./prime <processNum> <priority>
Task: Computes prime numbers continuously and responds to scheduler signals
 **************************************************/
int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "prime usage: %s <processNum> <priority>\n", argv[0]);
        return 1;
    }

    g_pnum = atoi(argv[1]);
    g_pri  = atoi(argv[2]);
    current = 1234567890ULL + (unsigned long long)g_pnum * 100000ULL;

    signal(SIGTSTP, handle_tstp);   // catchable: print, then SIGSTOP self
    signal(SIGCONT, handle_cont);
    signal(SIGTERM, handle_term);

    printf("Process %d: My priority is %d, my PID is %d: I just got started.\n", g_pnum, g_pri, (int)getpid());
    printf("I am starting with the number %llu to find the next prime number.\n", current);
    fflush(stdout);

    while (1) {
        if (isPrime(current)) {
            highestPrime = current;
        }
        current++;
    }
}

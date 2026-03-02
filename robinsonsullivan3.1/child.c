// File: child.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 3 March 2026

#include "child.h" 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

static int proc_num = -1;
static int priority = -1;   // NEW: store priority for Assignment 3
static unsigned long long highest_prime = 0;

/**************************************************
Method Name: is_prime
Returns: int
Input: unsigned long long n
Precondition: n is a positive integer
Task: Determines whether n is a prime number.
 **************************************************/
static int is_prime(unsigned long long n) {
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (unsigned long long d = 3; d * d <= n; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

/**************************************************
Method Name: on_tstp
Returns: void
Input: int sig
Precondition: Triggered by SIGTSTP
Task: Prints suspend message and stops the process
 **************************************************/
static void on_tstp(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: "
           "I am about to be suspended... Highest prime number I found is %llu.\n",
           proc_num, priority, getpid(), highest_prime);
    fflush(stdout);
    raise(SIGSTOP); // Stop the process
}

/**************************************************
Method Name: on_cont
Returns: void
Input: int sig
Precondition: Triggered by SIGCONT
Task: Prints resume message and continues execution
 **************************************************/
static void on_cont(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: "
           "I just got resumed.\nHighest prime number I found is %llu.\n",
           proc_num, priority, getpid(), highest_prime);
    fflush(stdout);
}

/**************************************************
Method Name: on_term
Returns: void
Input: int sig
Precondition: Triggered by SIGTERM
Task: Prints end message and ends the process
 **************************************************/
static void on_term(int sig) {
    (void)sig;
    printf("Process %d: My priority is %d, my PID is %d: "
           "I completed my task and I am exiting. "
           "Highest prime number I found is %llu.\n",
           proc_num, priority, getpid(), highest_prime);
    fflush(stdout);
    _exit(0);
}

/**************************************************
Method Name: rand_10_digit
Returns: unsigned long long
Input: N/A
Precondition: srand() is called
Task: Generates a random 10-digit starting number
 **************************************************/
static unsigned long long rand_10_digit(void) {
    unsigned long long a = (unsigned long long)rand();
    unsigned long long b = (unsigned long long)rand();
    unsigned long long c = (a << 31) ^ b;
    return 1000000000ULL + (c % 9000000000ULL); // [1,000,000,000 .. 9,999,999,999]
}

/**************************************************
Method Name: main
Returns: int
Input: int argc, char **argv
Precondition: Program must be run as 
              ./child -p <process_number> -r <priority>
Task: Complete Project
 **************************************************/
int main(int argc, char **argv) {

    // UPDATED: accept priority argument for Assignment 3
    if (argc != 5 ||
        strcmp(argv[1], "-p") != 0 ||
        strcmp(argv[3], "-r") != 0) {

        fprintf(stderr, "Usage: %s -p <process_number> -r <priority>\n", argv[0]);
        return 1;
    }

    proc_num = atoi(argv[2]);
    priority = atoi(argv[4]);

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    unsigned long long start = rand_10_digit();

    printf("Process %d: My priority is %d, my PID is %d: "
           "I just got started.\n"
           "I am starting with the number %llu to find the next prime number.\n",
           proc_num, priority, getpid(), start);
    fflush(stdout);

    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sa.sa_handler = on_tstp;
    sigaction(SIGTSTP, &sa, NULL);

    sa.sa_handler = on_cont;
    sigaction(SIGCONT, &sa, NULL);

    sa.sa_handler = on_term;
    sigaction(SIGTERM, &sa, NULL);

    unsigned long long x = start;
    while (1) {
        while (!is_prime(x)) x++;
        if (x > highest_prime) highest_prime = x;
        x++;
    }
}

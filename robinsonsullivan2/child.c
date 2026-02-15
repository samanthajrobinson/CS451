#include "child.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

static int proc_num = -1;
static unsigned long long highest_prime = 0;

static int is_prime(unsigned long long n) {
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (unsigned long long d = 3; d * d <= n; d += 2) {
        if (n % d == 0) return 0;
    }
    return 1;
}

static void on_tstp(int sig) {
    (void)sig;
    printf("CHILD SUSPEND p=%d pid=%d highest=%llu\n", proc_num, getpid(), highest_prime);
    fflush(stdout);
    raise(SIGSTOP); // actually stop
}

static void on_cont(int sig) {
    (void)sig;
    printf("CHILD RESUME p=%d pid=%d highest=%llu\n", proc_num, getpid(), highest_prime);
    fflush(stdout);
}

static void on_term(int sig) {
    (void)sig;
    printf("CHILD END p=%d pid=%d highest=%llu\n", proc_num, getpid(), highest_prime);
    fflush(stdout);
    _exit(0);
}

static unsigned long long rand_10_digit(void) {
    // build a larger random number than rand() alone
    unsigned long long a = (unsigned long long)rand();
    unsigned long long b = (unsigned long long)rand();
    unsigned long long c = (a << 31) ^ b;
    return 1000000000ULL + (c % 9000000000ULL); // [1,000,000,000 .. 9,999,999,999]
}

int main(int argc, char **argv) {
    if (argc != 3 || strcmp(argv[1], "-p") != 0) {
        fprintf(stderr, "Usage: %s -p <process_number>\n", argv[0]);
        return 1;
    }
    proc_num = atoi(argv[2]);

    srand((unsigned)time(NULL) ^ (unsigned)getpid());
    unsigned long long start = rand_10_digit();

    printf("CHILD START p=%d pid=%d rand=%llu\n", proc_num, getpid(), start);
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


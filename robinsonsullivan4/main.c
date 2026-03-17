// File: main.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 17 March 2026

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include "Party.h"

// Global configuration values set from command line
int numStudents = 0;
int numTaxis = 0;
int maxPartyTime = 0;

// Circular queue for waiting students
int waitingQueue[MAX_STUDENTS];
int front = 0;
int rear = 0;
int count = 0;

// Counters to track taxis and students
int totalStudentsTaken = 0;
int totalTripsCompleted = 0;
int totalTripsNeeded = 0;

// Thread arrays for students and taxis
pthread_t studentThreads[MAX_STUDENTS];
pthread_t taxiThreads[MAX_TAXIS];

// Semaphores for synchronization
sem_t queueMutex; // protects queue access
sem_t curb; // ensures one taxi at curb
sem_t studentsReady; // counts waiting students
sem_t rideDone[MAX_STUDENTS]; // signals student picked up

// Parse and validate command line 
int parseArguments(int argc, char *argv[]) {
    int option;

    while ((option = getopt(argc, argv, "s:t:m:")) != -1){
        switch (option)
        {
        case 's':
            numStudents = atoi(optarg);
            break;
        case 't':
            numTaxis = atoi(optarg);
            break;
        case 'm':
            maxPartyTime = atoi(optarg);
            break;
        default:
            return 0;
        }
    }

    // Validate student count
    if (numStudents <= 0 || numStudents > MAX_STUDENTS) {
        printf("Error: Number of students must be between 1 and 40.\n");
        return 0;
    }

    // Validate taxi count
    if (numTaxis <= 0 || numTaxis > MAX_TAXIS) {
        printf("Error: Number of taxis must be between 1 and 10.\n");
        return 0;
    }

    // Validate MAX party time
    if (maxPartyTime < 0 || maxPartyTime > 100) {
        printf("Error: Maximum party time must be between 0 and 100.\n");
        return 0;
    }

    // Make sure students can be grouped into taxis of 4
    if (numStudents % 4 != 0) {
        printf("Error: Number of students must be divisible by 4.\n");
        return 0;
    }

    return 1;
}

int main(int argc, char *argv[]) {
    int i;
    int studentIDs[MAX_STUDENTS];
    int taxiIDs[MAX_TAXIS];

    // Parse inputs or exit
    if (!parseArguments(argc, argv)) {
        printf("Usage: ./Party -s <students> -t <taxis> -m <maxPartyTime>\n");
        return 1;
    }

    // Compute how many taxi trips are needed
    totalTripsNeeded = numStudents / 4;

    // Initialize semaphores
    sem_init(&queueMutex, 0, 1);
    sem_init(&curb, 0, 1);
    sem_init(&studentsReady, 0, 0);

    // Initialize per-student completion semaphores
    for (i = 0; i < numStudents; i++) {
        sem_init(&rideDone[i], 0, 0);
    }

    // Create student threads
    for (i = 0; i < numStudents; i++) {
        studentIDs[i] = i;
        pthread_create(&studentThreads[i], NULL, studentFunction, &studentIDs[i]);
    }

    // Create taxi threads
    for (i = 0; i < numTaxis; i++) {
        taxiIDs[i] = i;
        pthread_create(&taxiThreads[i], NULL, taxiFunction, &taxiIDs[i]);
    }

    // Wait for students to finish
    for (i = 0; i < numStudents; i++) {
        pthread_join(studentThreads[i], NULL);
    }

    // Wait for taxis to finish
    for (i = 0; i < numTaxis; i++) {
        pthread_join(taxiThreads[i], NULL);
    }

    // Final completion message
    printf("All students have been taken home.\n");

    // Clean up semaphores
    sem_destroy(&queueMutex);
    sem_destroy(&curb);
    sem_destroy(&studentsReady);

    for (i = 0; i < numStudents; i++) {
        sem_destroy(&rideDone[i]);
    }

    return 0;
}

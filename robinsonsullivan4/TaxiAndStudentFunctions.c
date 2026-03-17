// File: TaxiAndStudentFunctions.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 17 March 2026

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "Party.h"

// Add a student to the queue
void enqueueStudent(int studentID) {
    waitingQueue[rear] = studentID;
    rear = (rear + 1) % MAX_STUDENTS;
    count++;
}

// Remove a student from the queue
int dequeueStudent(void) {
    int studentID = waitingQueue[front];
    front = (front + 1) % MAX_STUDENTS;
    count--;
    return studentID;
}

// Student thread
void *studentFunction(void *arg) {
    int studentID = *((int *)arg);
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)(studentID * 12345);
    int partyTime;

    // Student arrives at the party
    printf("Student %d: I am at the party. It is way more fun than I expected...\n", studentID);

    // Generate random party time
    if (maxPartyTime == 0) {
        partyTime = 0;
    } else {
        partyTime = rand_r(&seed) % (maxPartyTime + 1);
    }

    printf("My party time is: %d\n", partyTime);
    sleep(partyTime);

    // Student finishes and joins the taxi queue
    printf("Student %d: I am done partying and waiting for a taxi.\n", studentID);

    sem_wait(&queueMutex);
    enqueueStudent(studentID);
    sem_post(&queueMutex);

    // Signal that a student is ready
    sem_post(&studentsReady);

    // Wait until taxi signals pickup
    sem_wait(&rideDone[studentID]);

    pthread_exit(NULL);
}

// Taxi thread
void *taxiFunction(void *arg) {
    int taxiID = *((int *)arg);

    while (1) {
        int pickedUp[4];
        int i;
        int totalAfterTrip;

        // Wait to access the curb
        printf("Taxi %d: Waiting to arrive at the curb.\n", taxiID);
        sem_wait(&curb);

        // Check if all required trips are completed
        sem_wait(&queueMutex);
        if (totalTripsCompleted >= totalTripsNeeded) {
            sem_post(&queueMutex);
            sem_post(&curb);
            break;
        }
        sem_post(&queueMutex);

        // Taxi arrives and begins pickup
        printf("Taxi %d: I arrived at the curb.\n", taxiID);
        printf("Taxi %d: Waiting for students...\n", taxiID);

        // Pick up exactly 4 students
        for (i = 0; i < 4; i++) {
            sem_wait(&studentsReady);
            sem_wait(&queueMutex);
            pickedUp[i] = dequeueStudent();
            sem_post(&queueMutex);

            // Print current pickup state
            printf("Taxi %d: I have picked up %d student(s). Student IDs:", taxiID, i + 1);
            for (int j = 0; j <= i; j++) {
                printf(" %d", pickedUp[j]);
            }
            printf("\n");
        }

        // Update counters after trip
        sem_wait(&queueMutex);
        totalStudentsTaken += 4;
        totalTripsCompleted++;
        totalAfterTrip = totalStudentsTaken;
        sem_post(&queueMutex);

        // Taxi departs with 4 students
        printf("Taxi %d: Departing with 4 students. Total students picked up: %d\n", taxiID, totalAfterTrip);

        // Signal each student that their ride is complete
        for (i = 0; i < 4; i++) {
            sem_post(&rideDone[pickedUp[i]]);
        }

        // Release curb for next taxi
        sem_post(&curb);

        // Small delay to allow other taxis to run
        usleep(1000);
    }
    pthread_exit(NULL);
}

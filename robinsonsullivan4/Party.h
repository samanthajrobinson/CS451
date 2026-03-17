// File: Party.h
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 17 March 2026

#ifndef PARTY_H
#define PARTY_H

// File: Party.h
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 17 March 2026

#include <pthread.h>
#include <semaphore.h>

#define MAX_STUDENTS 40
#define MAX_TAXIS 10

extern int numStudents;
extern int numTaxis;
extern int maxPartyTime;

extern int waitingQueue[MAX_STUDENTS];
extern int front;
extern int rear;
extern int count;

extern int totalStudentsTaken;
extern int totalTripsCompleted;
extern int totalTripsNeeded;

extern pthread_t studentThreads[MAX_STUDENTS];
extern pthread_t taxiThreads[MAX_TAXIS];

extern sem_t queueMutex;
extern sem_t curb;
extern sem_t studentsReady;
extern sem_t rideDone[MAX_STUDENTS];

void *studentFunction(void *arg);
void *taxiFunction(void *arg);
void enqueueStudent(int studentID);
int dequeueStudent(void);
int parseArguments(int argc, char *argv[]);

#endif


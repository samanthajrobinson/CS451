// File: processInfo.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 28 January 2026

#include "processInfo.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// Option Selection
static pid_t selectedPid = 1;     // Default PID = 1
static int optState = 0;          // Default State
static int optTime = 0;           // Default Time
static int optMemory = 0;         // Default Memory Used
static int optCmdLine = 0;        // Default Command Input

/**************************************************
Method Name: parseOptions
Returns: void
Input: int argc, char *argv[]
Precondition: argv contains command line arguments
Task: Parses command line arguments and sets internal flags and selected PID based on provided options.
 **************************************************/
void parseOptions(int argc, char *argv[]) {
    int opt;

    // Walk through the command line options using getopt.
    while ((opt = getopt(argc, argv, "p:stvc")) != -1) {
        switch (opt) {
            case 'p':
                // Setter: sets the PID that the rest of the program will inspect.
                selectedPid = atoi(optarg);
                break;
            case 's':
                // Setter: enables "state" output.
                optState = 1;
                break;
            case 't':
                // Setter: enables "time" output.
                optTime = 1;
                break;
            case 'v':
                // Setter: enables "virtual memory" output.
                optMemory = 1;
                break;
            case 'c':
                // Setter: enables "command line" output.
                optCmdLine = 1;
                break;
            default:
                // Invalid option, print usage and exit.
                fprintf(stderr, "Usage: %s [-p pid] [-s] [-t] [-v] [-c]\n", argv[0]);
                exit(1);
        }
    }
}

// Getter: returns non-zero if any output flags were selected.
int optionsSelected(void) { return optState || optTime || optMemory || optCmdLine; }

// Getter: returns the PID currently selected for inspection.
pid_t getPid(void) { return selectedPid; }

// Getter: returns whether "state" output is enabled.
int showState(void) { return optState; }

// Getter: returns whether "time" output is enabled.
int showTime(void) { return optTime; }

// Getter: returns whether "memory" output is enabled.
int showMemory(void) { return optMemory; }

// Getter: returns whether "command line" output is enabled.
int showCmdLine(void) { return optCmdLine; }

/* -----------------------
   /proc parsing
   ----------------------- */

/**************************************************
Method Name: getState
Returns: char
Input: pid_t pid
Precondition: pid may or may not exist in /proc
Task: Reads /proc/[pid]/stat and returns the process state character (field 3 in that file).
 **************************************************/
char getState(pid_t pid) {
    char path[64], buffer[4096];
    FILE *fp;
    char *token;
    int field = 1;

    // Build path to /proc/[pid]/stat.
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    // Open the stat file, return '?' if it fails.
    fp = fopen(path, "r");
    if (!fp) return '?';

    // Read the single line from the file into buffer.
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Tokenize the line by spaces and grab the 3rd field (state).
    token = strtok(buffer, " ");
    while (token) {
        if (field == 3) return token[0];
        token = strtok(NULL, " ");
        field++;
    }

    // If parsing fails, return unknown.
    return '?';
}

/**************************************************
Method Name: getTime
Returns: long
Input: pid_t pid
Precondition: pid may or may not exist in /proc
Task: Reads /proc/[pid]/stat and returns total CPU time (utime + stime) in seconds.
 **************************************************/
long getTime(pid_t pid) {
    char path[64], buffer[4096];
    FILE *fp;
    char *token;
    int field = 1;
    long utime = 0, stime = 0;

    // Build path to /proc/[pid]/stat.
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);

    // Open the stat file, return 0 if it fails.
    fp = fopen(path, "r");
    if (!fp) return 0;

    // Read the single line from the file into buffer.
    fgets(buffer, sizeof(buffer), fp);
    fclose(fp);

    // Field 14 is utime and field 15 is stime (both in clock ticks).
    token = strtok(buffer, " ");
    while (token) {
        if (field == 14) {
            utime = atol(token);
        } else if (field == 15) {
            stime = atol(token);
            break;
        }
        token = strtok(NULL, " ");
        field++;
    }

    // Convert clock ticks to seconds.
    return (utime + stime) / sysconf(_SC_CLK_TCK);
}

/**************************************************
Method Name: getVMemory
Returns: long
Input: pid_t pid
Precondition: pid may or may not exist in /proc
Task: Reads /proc/[pid]/statm and returns the virtual memory size in pages (first field).
 **************************************************/
long getVMemory(pid_t pid) {
    char path[64];
    FILE *fp;
    long size = 0;

    // Build path to /proc/[pid]/statm.
    snprintf(path, sizeof(path), "/proc/%d/statm", pid);

    // Open the statm file, return 0 if it fails.
    fp = fopen(path, "r");
    if (!fp) return 0;

    // Read the first field (total program size) in pages.
    fscanf(fp, "%ld", &size);
    fclose(fp);

    return size;
}

/**************************************************
Method Name: getCmd
Returns: void
Input: pid_t pid, char *buffer, int size
Precondition: buffer points to writable memory of length size
Task: Reads /proc/[pid]/cmdline into buffer and replaces internal nulls with spaces so the result prints as a normal command line.
 **************************************************/
void getCmd(pid_t pid, char *buffer, int size) {
    char path[64];
    FILE *fp;
    int bytesRead, i;

    // Build path to /proc/[pid]/cmdline.
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    // Open cmdline file, return empty string if it fails.
    fp = fopen(path, "r");
    if (!fp) {
        buffer[0] = '\0';
        return;
    }

    // Read raw bytes (cmdline is null-separated arguments).
    bytesRead = fread(buffer, 1, size - 1, fp);
    fclose(fp);

    // Replace embedded null characters with spaces.
    for (i = 0; i < bytesRead - 1; i++) {
        if (buffer[i] == '\0') buffer[i] = ' ';
    }

    // Null-terminate to make it a valid C string.
    buffer[bytesRead] = '\0';
}

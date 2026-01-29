// File: main.c
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 28 January 2026

#include <stdio.h>
#include "processInfo.h"

int main(int argc, char *argv[]) {
    pid_t pid;
    char state;
    long time, memory;
    char cmdLine[1024];

    // Parse user option selections
    parseOptions(argc, argv);

    // Exit if no options are selected
    if (!optionsSelected()) return 0;

    pid = getPid(); // Get input process id
    printf("%d:", pid); // Print id

    // If state selected
    if (showState()) {
        state = getState(pid); // Get state
        printf(" %c", state); // Print state
    }

    // If time selected
    if (showTime()) {
        time = getTime(pid); // Get time (ticks)
        printf(" time=%02ld:%02ld:%02ld", time / 3600, (time % 3600) / 60, time % 60); // Print time in second format
    }

    // If memory selected
    if (showMemory()) {
        memory = getVMemory(pid); // Get memory
        printf(" sz=%ld", memory); // Print memory
    }

    // If command selected
    if (showCmdLine()) {
        getCmd(pid, cmdLine, sizeof(cmdLine)); // Get command that started process
        printf(" [%s]", cmdLine); // Print command
    }

    printf("\n"); // Enter new line
    return 0; // Exit program (no errors0
}

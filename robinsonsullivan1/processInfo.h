// File: processInfo.h
// Author: Samantha Robinson, Elizabeth Sullivan
// Date: 28 January 2026

#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <sys/types.h>

// Option handling of user choices
void parseOptions(int argc, char *argv[]);
int optionsSelected(void);
pid_t getPid(void);
int showState(void);
int showTime(void);
int showMemory(void);
int showCmdLine(void);
char getState(pid_t pid);
long getTime(pid_t pid);
long getVMemory(pid_t pid);
void getCmd(pid_t pid, char *buffer, int size);

#endif

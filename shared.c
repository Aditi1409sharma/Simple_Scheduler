#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "shared.h"

// Define a custom signal to notify child process to start execution
#define SIG_START_EXECUTION (SIGRTMIN + 1)

#ifndef SHARED_H
#define SHARED_H
// Data structure to represent a job or process
struct Process
{
    int pid;            // Process ID
    char command[1024]; // Command to execute
};
#endif
// Function to send a signal to a process by its PID
int sendSignalToProcess(int pid, int signal)
{
    if (kill(pid, signal) == -1)
    {
        perror("Failed to send signal to the process");
        return -1;
    }
    return 0;
}

// Signal handler for custom signals
void customSignalHandler(int signo)
{
    if (signo == SIG_START_EXECUTION)
    {
        // Handle the SIG_START_EXECUTION signal
        // Notify the child process to start execution
        // You can add your logic here
    }
}

// Initialize signal handling for custom signals

// Initialize signal handling for custom signals
void initializeSignalHandling()
{
    struct sigaction sa;
    sa.sa_handler = customSignalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);

    // Install the signal handler for custom signals
    if (sigaction(SIG_START_EXECUTION, &sa, NULL) == -1)
    {
        perror("Failed to install signal handler");
        exit(EXIT_FAILURE);
    }
}

// Define any other shared functions or data structures here

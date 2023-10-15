#ifndef SHARED_H
#define SHARED_H

// Define a custom signal to notify child process to start execution
#define SIG_START_EXECUTION (SIGRTMIN + 1)
#define SIG_STOP_EXECUTION (SIGRTMIN + 1)
// Data structure to represent a job or process
struct Process
{
    int pid;            // Process ID
    char command[1024]; // Command to execute
};

// Function to send a signal to a process by its PID
int sendSignalToProcess(int pid, int signal);

// Initialize signal handling for custom signals
void initializeSignalHandling();

#endif

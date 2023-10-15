#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <time.h>
#include "shared.h"

// The size of the shared memory segment
#define SHARED_MEMORY_SIZE 1024

// Function to manage the round-robin scheduling
void scheduleProcesses(int NCPU, int TSLICE, struct Process *processes, int numProcesses, int *shm_id)
{
    // Initialize the process queue with the provided processes
    int *processQueue = (int *)malloc(numProcesses * sizeof(int));
    for (int i = 0; i < numProcesses; i++)
    {
        processQueue[i] = i;
    }

    // Initialize the index to track the front of the queue
    int front = 0;

    while (numProcesses > 0)
    {
        int runningCount = 0;
        // Execute processes in round-robin fashion
        for (int i = 0; i < NCPU; i++)
        {
            if (processQueue[front] != -1)
            {
                int processIndex = processQueue[front];
                struct Process *currentProcess = &processes[processIndex];

                // Notify the child process to start execution
                sendSignalToProcess(currentProcess->pid, SIG_START_EXECUTION);
                printf("Scheduler: Process %d started execution.\n", currentProcess->pid);
                runningCount++;

                // Remove the process from the queue
                processQueue[front] = -1;
            }

            // Move the front to the next process in the queue
            front = (front + 1) % numProcesses;
        }

        // Sleep for TSLICE seconds to simulate time quantum expiration
        sleep(TSLICE);

        // Stop running processes
        for (int i = 0; i < NCPU; i++)
        {
            int processIndex = (front - runningCount + i + numProcesses) % numProcesses;
            if (processQueue[processIndex] != -1)
            {
                struct Process *currentProcess = &processes[processQueue[processIndex]];
                // Notify the child process to stop execution
                sendSignalToProcess(currentProcess->pid, SIG_STOP_EXECUTION);
                printf("Scheduler: Process %d stopped execution.\n", currentProcess->pid);
            }
        }

        // Rearrange the queue by adding processes back to the rear
        for (int i = 0; i < numProcesses; i++)
        {
            if (processQueue[i] == -1)
            {
                processQueue[i] = processQueue[front];
                processQueue[front] = -1;
                front = (front + 1) % numProcesses;
            }
        }
    }

    // Detach and remove the shared memory segment
    if (shmdt(processes) == -1)
    {
        perror("shmdt");
    }

    // Detach and remove the shared memory id
    if (shmctl(*shm_id, IPC_RMID, NULL) == -1)
    {
        perror("shmctl");
    }
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <NCPU> <TSLICE>\n", argv[0]);
        exit(1);
    }

    int NCPU = atoi(argv[1]);
    int TSLICE = atoi(argv[2]);

    key_t key = ftok("shared_memory_key", 65);
    int shm_id = shmget(key, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);

    if (shm_id == -1)
    {
        perror("shmget");
        exit(1);
    }

    // Attach the shared memory segment
    struct Process *processes = (struct Process *)shmat(shm_id, NULL, 0);

    if (processes == (void *)-1)
    {
        perror("shmat");
        exit(1);
    }

    int numProcesses = 0;

    while (1)
    {
        // Check for new processes in shared memory
        if (processes[numProcesses].pid != -1)
        {
            numProcesses++;
        }
        else
        {
            break;
        }
    }

    // Initialize the signal handler
    initializeSignalHandling();

    // Call the scheduling function
    scheduleProcesses(NCPU, TSLICE, processes, numProcesses, &shm_id);

    return 0;
}
